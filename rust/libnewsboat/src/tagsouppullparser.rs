use crate::logger::{self, Level};
use crate::utils;

/*
 * This method implements an "XML" pull parser. In reality, it's more liberal
 * than any XML pull parser, as it basically accepts everything that even only
 * remotely looks like XML. We use this parser for the HTML renderer.
 */

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub enum Event {
    StartDocument,
    EndDocument,
    StartTag,
    EndTag,
    Text,
}

type Attribute = (String, String);

pub struct TagSoupPullParser {
    /// characters of the source in reverse order
    chars: Vec<char>,
    attributes: Vec<Attribute>,
    text: String,
    current_event: Event,
}

impl TagSoupPullParser {
    pub fn new(source: String) -> Self {
        TagSoupPullParser {
            chars: source.chars().rev().collect(),
            attributes: Vec::new(),
            text: String::new(),
            current_event: Event::StartDocument,
        }
    }

    pub fn get_attribute_value(&self, name: &str) -> Option<String> {
        self.attributes
            .iter()
            .find(|(key, _)| key == name)
            .map(|(_, value)| value.clone())
    }

    pub fn get_text(&self) -> String {
        self.text.clone()
    }

    /// the next() method returns the next event by parsing the
    /// next element of the XML stream, depending on the current
    /// event.
    pub fn next(&mut self) -> Event {
        self.attributes.clear();
        self.text.clear();

        if self.chars.is_empty() {
            self.current_event = Event::EndDocument;
        }

        match self.current_event {
            Event::StartDocument | Event::StartTag | Event::EndTag => match self.chars.pop() {
                None => {
                    self.current_event = Event::EndDocument;
                }
                Some('<') => {
                    self.handle_tag();
                }
                Some(c) => {
                    self.handle_text(c);
                }
            },
            Event::Text => {
                self.handle_tag();
            }
            Event::EndDocument => {}
        }

        self.current_event
    }

    fn read_tag(&mut self) -> Option<String> {
        let mut s = String::new();
        while let Some(c) = self.chars.pop() {
            if c == '>' {
                return Some(s);
            }
            s.push(c);
        }
        None
    }

    fn handle_tag(&mut self) {
        if let Some(s) = self.read_tag() {
            self.parse_tag(&s);
            self.current_event = self.determine_tag_type();
        } else {
            self.current_event = Event::EndDocument;
        }
    }

    fn handle_text(&mut self, c: char) {
        self.text.push(c);
        while let Some(c) = self.chars.pop() {
            if c == '<' {
                break;
            }
            self.text.push(c);
        }
        self.text = decode_entities(&self.text);
        utils::remove_soft_hyphens(&mut self.text);
        self.current_event = Event::Text;
    }

    fn determine_tag_type(&mut self) -> Event {
        if self.text.starts_with('/') {
            self.text.remove(0);
            Event::EndTag
        } else {
            Event::StartTag
        }
    }

    fn parse_tag(&mut self, tagstr: &str) {
        let mut s = tagstr.trim();
        let mut pos = s.find(|c: char| c.is_whitespace());
        let mut count = 0;

        log!(
            Level::Debug,
            "parse_tag: parsing '{}', pos = {:?}",
            tagstr,
            pos
        );

        while !s.is_empty() {
            if count == 0 {
                // first token: tag name
                let end = pos.unwrap_or(s.len());
                self.text = s[..end].to_string();
                if self.text.ends_with('/') {
                    // a kludge for <br/>
                    self.text.pop();
                }
                log!(Level::Debug, "parse_tag: tag name = {}", self.text);
                s = &s[end..];
            } else {
                pos = s.find(|c| c == '=' || c == ' ');
                if let Some(idx) = pos {
                    let mut rest = &s[idx..];
                    log!(Level::Debug, "parse_tag: found = or space");
                    if rest.starts_with('=') {
                        rest = &rest[1..];
                        log!(Level::Debug, "parse_tag: found =");
                        if rest.starts_with('\'') || rest.starts_with('"') {
                            let quote = &rest[..1];
                            pos = rest[1..].find(quote).map(|i| i + idx + 3);
                            log!(
                                Level::Debug,
                                "parse_tag: finding ending quote, pos = {:?}",
                                pos
                            );
                        } else {
                            pos = rest.find(|c: char| c.is_whitespace()).map(|i| i + idx + 1);
                            log!(Level::Debug, "parse_tag: finding end of unquoted attribute");
                        }
                    }
                }

                let attr = if let Some(idx) = pos {
                    let attr = &s[..idx];
                    s = &s[idx..];
                    attr
                } else {
                    log!(
                        Level::Debug,
                        "parse_tag: found end of string, correcting end position"
                    );
                    let attr = s;
                    s = "";
                    attr
                };
                log!(
                    Level::Debug,
                    "parse_tag: extracted attribute is '{}', adding",
                    attr
                );
                self.add_attribute(attr);
            }
            s = s.trim_start();
            count += 1;
        }
    }

    fn add_attribute(&mut self, s: &str) {
        let mut s = s;
        if s.ends_with('/') {
            s = &s[..s.len() - 1];
        }
        if s.is_empty() {
            return;
        }
        let (attribname, attribvalue) = s.split_once('=').unwrap_or((s, s));
        self.attributes
            .push((attribname.to_lowercase(), decode_attribute(attribvalue)));
    }
}

fn parse_hex_prefix(s: &str) -> u32 {
    let mut result = 0;
    for c in s.chars() {
        if let Some(value) = c.to_digit(16) {
            result = (result << 4) | value;
        } else {
            break;
        }
    }
    result
}

fn decode_entity(s: &str) -> String {
    log!(
        Level::Debug,
        "TagSoupPullParser::decode_entity: decoding '{}'...",
        s
    );
    if s.len() > 1 && s.starts_with('#') {
        let wc = if s.starts_with("#x") {
            parse_hex_prefix(&s[2..])
        } else {
            utils::to_u(s[1..].to_string(), 0)
        };
        // convert some windows entities according to the spec
        // https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-end-state
        let wc = match wc {
            0x80 => 0x20AC, // EURO SIGN (€)
            0x82 => 0x201A, // SINGLE LOW-9 QUOTATION MARK (‚)
            0x83 => 0x0192, // LATIN SMALL LETTER F WITH HOOK (ƒ)
            0x84 => 0x201E, // DOUBLE LOW-9 QUOTATION MARK („)
            0x85 => 0x2026, // HORIZONTAL ELLIPSIS (…)
            0x86 => 0x2020, // DAGGER (†)
            0x87 => 0x2021, // DOUBLE DAGGER (‡)
            0x88 => 0x02C6, // MODIFIER LETTER CIRCUMFLEX ACCENT (ˆ)
            0x89 => 0x2030, // PER MILLE SIGN (‰)
            0x8A => 0x0160, // LATIN CAPITAL LETTER S WITH CARON (Š)
            0x8B => 0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK (‹)
            0x8C => 0x0152, // LATIN CAPITAL LIGATURE OE (Œ)
            0x8E => 0x017D, // LATIN CAPITAL LETTER Z WITH CARON (Ž)
            0x91 => 0x2018, // LEFT SINGLE QUOTATION MARK (‘)
            0x92 => 0x2019, // RIGHT SINGLE QUOTATION MARK (’)
            0x93 => 0x201C, // LEFT DOUBLE QUOTATION MARK (“)
            0x94 => 0x201D, // RIGHT DOUBLE QUOTATION MARK (”)
            0x95 => 0x2022, // BULLET (•)
            0x96 => 0x2013, // EN DASH (–)
            0x97 => 0x2014, // EM DASH (—)
            0x98 => 0x02DC, // SMALL TILDE (˜)
            0x99 => 0x2122, // TRADE MARK SIGN (™)
            0x9A => 0x0161, // LATIN SMALL LETTER S WITH CARON (š)
            0x9B => 0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK (›)
            0x9C => 0x0153, // LATIN SMALL LIGATURE OE (œ)
            0x9E => 0x017E, // LATIN SMALL LETTER Z WITH CARON (ž)
            0x9F => 0x0178, // LATIN CAPITAL LETTER Y WITH DIAERESIS (Ÿ)
            c => c,
        };

        log!(
            Level::Debug,
            "TagSoupPullParser::decode_entity: wc = {}",
            wc
        );

        if let Some(c) = char::from_u32(wc) {
            return c.to_string();
        }
    } else {
        for &(entity, value) in ENTITY_TABLE {
            if s == entity {
                if let Some(c) = char::from_u32(value) {
                    return c.to_string();
                } else {
                    return String::new();
                }
            }
        }
    }
    String::new()
}

fn decode_entities(s: &str) -> String {
    let mut s = s;
    let mut result = String::new();
    while let Some(ampersand_offset) = s.find('&') {
        let rest = &s[ampersand_offset + 1..];
        let semicolon_offset = match rest.find(';') {
            Some(offset) => offset,
            None => break,
        };
        result += &s[..ampersand_offset];
        let entity = &rest[..semicolon_offset];
        let encoded_entity = decode_entity(entity);
        if !encoded_entity.is_empty() {
            result += &encoded_entity;
            s = &rest[semicolon_offset + 1..];
        } else {
            result.push('&');
            s = rest;
        }
    }
    result += s;
    result
}

fn decode_attribute(s: &str) -> String {
    let mut s = s;
    if (s.starts_with('"') && s.ends_with('"')) || (s.starts_with('\'') && s.ends_with('\'')) {
        if !s.is_empty() {
            s = &s[1..];
        }
        if !s.is_empty() {
            s = &s[..s.len() - 1];
        }
    }
    decode_entities(s)
}

const ENTITY_TABLE: &[(&'static str, u32)] = &[
    /* semi-automatically generated from:
        - http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent
        - http://www.w3.org/TR/xhtml1/DTD/xhtml-special.ent
        - http://www.w3.org/TR/xhtml1/DTD/xhtml-symbol.ent
    */
    ("nbsp", ' ' as u32),
    ("iexcl", 161),
    ("cent", 162),
    ("pound", 163),
    ("curren", 164),
    ("yen", 165),
    ("brvbar", 166),
    ("sect", 167),
    ("uml", 168),
    ("copy", 169),
    ("ordf", 170),
    ("laquo", 171),
    ("not", 172),
    ("shy", 173),
    ("reg", 174),
    ("macr", 175),
    ("deg", 176),
    ("plusmn", 177),
    ("sup2", 178),
    ("sup3", 179),
    ("acute", 180),
    ("micro", 181),
    ("para", 182),
    ("middot", 183),
    ("cedil", 184),
    ("sup1", 185),
    ("ordm", 186),
    ("raquo", 187),
    ("frac14", 188),
    ("frac12", 189),
    ("frac34", 190),
    ("iquest", 191),
    ("Agrave", 192),
    ("Aacute", 193),
    ("Acirc", 194),
    ("Atilde", 195),
    ("Auml", 196),
    ("Aring", 197),
    ("AElig", 198),
    ("Ccedil", 199),
    ("Egrave", 200),
    ("Eacute", 201),
    ("Ecirc", 202),
    ("Euml", 203),
    ("Igrave", 204),
    ("Iacute", 205),
    ("Icirc", 206),
    ("Iuml", 207),
    ("ETH", 208),
    ("Ntilde", 209),
    ("Ograve", 210),
    ("Oacute", 211),
    ("Ocirc", 212),
    ("Otilde", 213),
    ("Ouml", 214),
    ("times", 215),
    ("Oslash", 216),
    ("Ugrave", 217),
    ("Uacute", 218),
    ("Ucirc", 219),
    ("Uuml", 220),
    ("Yacute", 221),
    ("THORN", 222),
    ("szlig", 223),
    ("agrave", 224),
    ("aacute", 225),
    ("acirc", 226),
    ("atilde", 227),
    ("auml", 228),
    ("aring", 229),
    ("aelig", 230),
    ("ccedil", 231),
    ("egrave", 232),
    ("eacute", 233),
    ("ecirc", 234),
    ("euml", 235),
    ("igrave", 236),
    ("iacute", 237),
    ("icirc", 238),
    ("iuml", 239),
    ("eth", 240),
    ("ntilde", 241),
    ("ograve", 242),
    ("oacute", 243),
    ("ocirc", 244),
    ("otilde", 245),
    ("ouml", 246),
    ("divide", 247),
    ("oslash", 248),
    ("ugrave", 249),
    ("uacute", 250),
    ("ucirc", 251),
    ("uuml", 252),
    ("yacute", 253),
    ("thorn", 254),
    ("yuml", 255),
    ("quot", 34),
    ("amp", 38),
    ("lt", 60),
    ("gt", 62),
    ("apos", 39),
    ("OElig", 338),
    ("oelig", 339),
    ("Scaron", 352),
    ("scaron", 353),
    ("Yuml", 376),
    ("circ", 710),
    ("tilde", 732),
    ("ensp", 8194),
    ("emsp", 8195),
    ("thinsp", 8201),
    ("zwnj", 8204),
    ("zwj", 8205),
    ("lrm", 8206),
    ("rlm", 8207),
    ("ndash", 8211),
    ("mdash", 8212),
    ("lsquo", 8216),
    ("rsquo", 8217),
    ("sbquo", 8218),
    ("ldquo", 8220),
    ("rdquo", 8221),
    ("bdquo", 8222),
    ("dagger", 8224),
    ("Dagger", 8225),
    ("permil", 8240),
    ("lsaquo", 8249),
    ("rsaquo", 8250),
    ("euro", 8364),
    ("fnof", 402),
    ("Alpha", 913),
    ("Beta", 914),
    ("Gamma", 915),
    ("Delta", 916),
    ("Epsilon", 917),
    ("Zeta", 918),
    ("Eta", 919),
    ("Theta", 920),
    ("Iota", 921),
    ("Kappa", 922),
    ("Lambda", 923),
    ("Mu", 924),
    ("Nu", 925),
    ("Xi", 926),
    ("Omicron", 927),
    ("Pi", 928),
    ("Rho", 929),
    ("Sigma", 931),
    ("Tau", 932),
    ("Upsilon", 933),
    ("Phi", 934),
    ("Chi", 935),
    ("Psi", 936),
    ("Omega", 937),
    ("alpha", 945),
    ("beta", 946),
    ("gamma", 947),
    ("delta", 948),
    ("epsilon", 949),
    ("zeta", 950),
    ("eta", 951),
    ("theta", 952),
    ("iota", 953),
    ("kappa", 954),
    ("lambda", 955),
    ("mu", 956),
    ("nu", 957),
    ("xi", 958),
    ("omicron", 959),
    ("pi", 960),
    ("rho", 961),
    ("sigmaf", 962),
    ("sigma", 963),
    ("tau", 964),
    ("upsilon", 965),
    ("phi", 966),
    ("chi", 967),
    ("psi", 968),
    ("omega", 969),
    ("thetasym", 977),
    ("upsih", 978),
    ("piv", 982),
    ("bull", 8226),
    ("hellip", 8230),
    ("prime", 8242),
    ("Prime", 8243),
    ("oline", 8254),
    ("frasl", 8260),
    ("weierp", 8472),
    ("image", 8465),
    ("real", 8476),
    ("trade", 8482),
    ("alefsym", 8501),
    ("larr", 8592),
    ("uarr", 8593),
    ("rarr", 8594),
    ("darr", 8595),
    ("harr", 8596),
    ("crarr", 8629),
    ("lArr", 8656),
    ("uArr", 8657),
    ("rArr", 8658),
    ("dArr", 8659),
    ("hArr", 8660),
    ("forall", 8704),
    ("part", 8706),
    ("exist", 8707),
    ("empty", 8709),
    ("nabla", 8711),
    ("isin", 8712),
    ("notin", 8713),
    ("ni", 8715),
    ("prod", 8719),
    ("sum", 8721),
    ("minus", 8722),
    ("lowast", 8727),
    ("radic", 8730),
    ("prop", 8733),
    ("infin", 8734),
    ("ang", 8736),
    ("and", 8743),
    ("or", 8744),
    ("cap", 8745),
    ("cup", 8746),
    ("int", 8747),
    ("there4", 8756),
    ("sim", 8764),
    ("cong", 8773),
    ("asymp", 8776),
    ("ne", 8800),
    ("equiv", 8801),
    ("le", 8804),
    ("ge", 8805),
    ("sub", 8834),
    ("sup", 8835),
    ("nsub", 8836),
    ("sube", 8838),
    ("supe", 8839),
    ("oplus", 8853),
    ("otimes", 8855),
    ("perp", 8869),
    ("sdot", 8901),
    ("lceil", 8968),
    ("rceil", 8969),
    ("lfloor", 8970),
    ("rfloor", 8971),
    ("lang", 9001),
    ("rang", 9002),
    ("loz", 9674),
    ("spades", 9824),
    ("clubs", 9827),
    ("hearts", 9829),
    ("diams", 9830),
];

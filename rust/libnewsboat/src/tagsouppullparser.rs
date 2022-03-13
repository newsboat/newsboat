use crate::utils::remove_soft_hyphens;

struct Attribute {
    name: String,
    value: String,
}

pub struct TagSoupPullParser {
    input: String,
    current_event: Event,
    text: String,
    attributes: Vec<Attribute>,
}

pub enum Event {
    StartDocument,
    EndDocument,
    StartTag,
    EndTag,
    Text,
}

impl TagSoupPullParser {
    pub fn new(input: &str) -> TagSoupPullParser {
        TagSoupPullParser {
            input: input.to_string(),
            current_event: Event::StartDocument,
            text: String::new(),
            attributes: vec![],
        }
    }

    pub fn next(self: &mut TagSoupPullParser) -> &Event {
        self.attributes.clear();
        self.text.clear();

        if self.input.is_empty() {
            self.current_event = Event::EndDocument;
        }

        match self.current_event {
            Event::StartDocument | Event::StartTag | Event::EndTag => {
                match self.input.chars().next() {
                    None => self.current_event = Event::EndDocument,
                    Some('<') => {
                        self.input = self.input[1..].to_string();
                        self.handle_tag();
                    }
                    Some(_) => self.handle_text(),
                }
            }
            Event::Text => self.handle_tag(),
            Event::EndDocument => {}
        }

        self.get_event_type()
    }

    pub fn get_attribute_value(self: &TagSoupPullParser, name: &str) -> Option<&String> {
        self.attributes
            .iter()
            .find(|attribute| attribute.name == name)
            .map(|attribute| &attribute.value)
    }

    pub fn get_event_type(self: &TagSoupPullParser) -> &Event {
        &self.current_event
    }

    pub fn get_text(self: &TagSoupPullParser) -> &str {
        &self.text
    }

    fn read_tag(self: &mut TagSoupPullParser) -> Option<String> {
        let (tag_content, remainder) = TagSoupPullParser::split_once(&self.input, '>');

        let tag_content = if remainder.is_some() {
            Some(tag_content.to_string())
        } else {
            None
        };

        self.input = remainder.unwrap_or("").to_string();
        tag_content
    }

    fn parse_tag(self: &mut TagSoupPullParser, tag_content: &str) {
        let whitespace = " \r\n\t";

        let mut remaining = tag_content;

        // Discard whitespace
        let first_non_whitespace = remaining.find(|c| !whitespace.contains(c));
        match first_non_whitespace {
            Some(pos) => remaining = &remaining[pos..],
            None => return,
        }

        // Find end of attribute name
        let first_whitespace = remaining.find(|c| whitespace.contains(c));
        match first_whitespace {
            Some(pos) => self.text = remaining[..pos].to_string(),
            None => {
                if let Some(remaining) = remaining.strip_suffix('/') {
                    self.text = remaining.to_string();
                } else {
                    self.text = remaining.to_string();
                }
                return;
            }
        }

        // Handle attributes
        while let Some(pos) = remaining.find(|c| !whitespace.contains(c)) {
            remaining = &remaining[pos..];

            let eq_or_ws_pos = remaining.find(|c| " =".contains(c));
            let attribute_end = match eq_or_ws_pos {
                Some(pos) => {
                    if remaining[pos..].starts_with('=') {
                        // TODO: Cleanup
                        match remaining[pos..].chars().nth(1) {
                            Some('\'') => {
                                pos + 3
                                    + remaining[pos + 2..]
                                        .find('\'')
                                        .unwrap_or_else(|| remaining[pos + 2..].len())
                            }
                            Some('"') => {
                                pos + 3
                                    + remaining[pos + 2..]
                                        .find('"')
                                        .unwrap_or_else(|| remaining[pos + 2..].len())
                            }
                            Some(_) => {
                                pos + remaining[pos..]
                                    .find(|c| whitespace.contains(c))
                                    .unwrap_or_else(|| remaining[pos..].len())
                            }
                            None => pos + 1,
                        }
                    } else {
                        pos
                    }
                }
                None => remaining.len(),
            };

            self.add_attribute(&remaining[..attribute_end]);
            remaining = &remaining[attribute_end..];
        }
    }

    fn decode_attribute(value: &str) -> String {
        let value = if (value.starts_with('\'') && value.ends_with('\''))
            || (value.starts_with('"') && value.ends_with('"'))
        {
            if value.len() == 1 {
                ""
            } else {
                &value[1..value.len() - 1]
            }
        } else {
            value
        };

        Self::decode_entities(value)
    }

    fn add_attribute(self: &mut TagSoupPullParser, attribute: &str) {
        let attribute = attribute.strip_prefix('/').unwrap_or(attribute);

        if attribute.is_empty() {
            return;
        }

        let (name, value) = Self::split_once(attribute, '=');
        let value = match value {
            Some(v) => v,
            None => name,
        };

        let value = Self::decode_attribute(value);

        self.attributes.push(Attribute {
            name: name.to_lowercase(),
            value,
        });
    }

    fn determine_tag_type(self: &mut TagSoupPullParser) -> Event {
        if self.text.starts_with('/') {
            self.text = self.text[1..].to_string();
            Event::EndTag
        } else {
            Event::StartTag
        }
    }

    fn handle_tag(self: &mut TagSoupPullParser) {
        if let Some(s) = self.read_tag() {
            self.parse_tag(&s);
            self.current_event = self.determine_tag_type();
        } else {
            self.current_event = Event::EndDocument;
        }
    }

    fn handle_text(self: &mut TagSoupPullParser) {
        let (text, remainder) = TagSoupPullParser::split_once(&self.input, '<');
        self.text.push_str(text);
        self.input = remainder.unwrap_or("").to_string();

        self.text = TagSoupPullParser::decode_entities(&self.text);

        remove_soft_hyphens(&mut self.text);

        self.current_event = Event::Text;
    }

    fn decode_entity(entity: &str) -> Option<char> {
        // spec: https://www.w3.org/TR/2017/WD-html52-20170228/syntax.html#character-references

        let codepoint = if let Some(number) = entity.strip_prefix('#') {
            #[allow(clippy::from_str_radix_10)]
            let specified_codepoint = if let Some(hexadecimal) = number.strip_prefix('x') {
                u32::from_str_radix(hexadecimal, 16).ok()
            } else {
                u32::from_str_radix(number, 10).ok()
            };
            specified_codepoint.map(TagSoupPullParser::map_common_windows_1252_codepoints)
        } else {
            TagSoupPullParser::codepoint_for_named_entity(entity)
        };

        match codepoint {
            Some(u) => char::from_u32(u),
            None => None,
        }
    }

    fn decode_entities(input: &str) -> String {
        let mut output = String::new();
        let mut current_input = input;
        loop {
            let (regular_text, remainder) = TagSoupPullParser::split_once(current_input, '&');
            output.push_str(regular_text);
            match remainder {
                Some(s) => {
                    let (entity, remainder) = TagSoupPullParser::split_once(s, ';');
                    match remainder {
                        Some(r) => {
                            let character = TagSoupPullParser::decode_entity(entity);
                            current_input = match character {
                                Some(c) => {
                                    output.push(c);
                                    // Continue with remainder after semicolon
                                    r
                                }
                                None => {
                                    output.push('&');
                                    // Continue with remainder after ampersand
                                    s
                                }
                            };
                        }
                        None => {
                            // Might not be necessary, just added to preserve behaviour from
                            // original C++ implementation. Only gets hit on an unterminated
                            // HTML entity.
                            output.push('&');
                            output.push_str(entity);
                            break;
                        }
                    }
                }
                None => break,
            }
        }
        output
    }

    fn split_once(input: &str, split_at: char) -> (&str, Option<&str>) {
        let mut x = input.splitn(2, split_at);
        let text_before = x.next().unwrap_or("");
        let remainder = x.next();
        (text_before, remainder)
    }

    fn map_common_windows_1252_codepoints(codepoint: u32) -> u32 {
        // convert some windows entities according to the spec
        // https://html.spec.whatwg.org/multipage/parsing.html#numeric-character-reference-end-state

        match codepoint {
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
            other => other,
        }
    }

    fn codepoint_for_named_entity(name: &str) -> Option<u32> {
        // Based on old C++ lookup table, which referenced the following pages:
        // - http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent
        // - http://www.w3.org/TR/xhtml1/DTD/xhtml-special.ent
        // - http://www.w3.org/TR/xhtml1/DTD/xhtml-symbol.ent

        match name {
            "nbsp" => Some(32),
            "iexcl" => Some(161),
            "cent" => Some(162),
            "pound" => Some(163),
            "curren" => Some(164),
            "yen" => Some(165),
            "brvbar" => Some(166),
            "sect" => Some(167),
            "uml" => Some(168),
            "copy" => Some(169),
            "ordf" => Some(170),
            "laquo" => Some(171),
            "not" => Some(172),
            "shy" => Some(173),
            "reg" => Some(174),
            "macr" => Some(175),
            "deg" => Some(176),
            "plusmn" => Some(177),
            "sup2" => Some(178),
            "sup3" => Some(179),
            "acute" => Some(180),
            "micro" => Some(181),
            "para" => Some(182),
            "middot" => Some(183),
            "cedil" => Some(184),
            "sup1" => Some(185),
            "ordm" => Some(186),
            "raquo" => Some(187),
            "frac14" => Some(188),
            "frac12" => Some(189),
            "frac34" => Some(190),
            "iquest" => Some(191),
            "Agrave" => Some(192),
            "Aacute" => Some(193),
            "Acirc" => Some(194),
            "Atilde" => Some(195),
            "Auml" => Some(196),
            "Aring" => Some(197),
            "AElig" => Some(198),
            "Ccedil" => Some(199),
            "Egrave" => Some(200),
            "Eacute" => Some(201),
            "Ecirc" => Some(202),
            "Euml" => Some(203),
            "Igrave" => Some(204),
            "Iacute" => Some(205),
            "Icirc" => Some(206),
            "Iuml" => Some(207),
            "ETH" => Some(208),
            "Ntilde" => Some(209),
            "Ograve" => Some(210),
            "Oacute" => Some(211),
            "Ocirc" => Some(212),
            "Otilde" => Some(213),
            "Ouml" => Some(214),
            "times" => Some(215),
            "Oslash" => Some(216),
            "Ugrave" => Some(217),
            "Uacute" => Some(218),
            "Ucirc" => Some(219),
            "Uuml" => Some(220),
            "Yacute" => Some(221),
            "THORN" => Some(222),
            "szlig" => Some(223),
            "agrave" => Some(224),
            "aacute" => Some(225),
            "acirc" => Some(226),
            "atilde" => Some(227),
            "auml" => Some(228),
            "aring" => Some(229),
            "aelig" => Some(230),
            "ccedil" => Some(231),
            "egrave" => Some(232),
            "eacute" => Some(233),
            "ecirc" => Some(234),
            "euml" => Some(235),
            "igrave" => Some(236),
            "iacute" => Some(237),
            "icirc" => Some(238),
            "iuml" => Some(239),
            "eth" => Some(240),
            "ntilde" => Some(241),
            "ograve" => Some(242),
            "oacute" => Some(243),
            "ocirc" => Some(244),
            "otilde" => Some(245),
            "ouml" => Some(246),
            "divide" => Some(247),
            "oslash" => Some(248),
            "ugrave" => Some(249),
            "uacute" => Some(250),
            "ucirc" => Some(251),
            "uuml" => Some(252),
            "yacute" => Some(253),
            "thorn" => Some(254),
            "yuml" => Some(255),
            "quot" => Some(34),
            "amp" => Some(38),
            "lt" => Some(60),
            "gt" => Some(62),
            "apos" => Some(39),
            "OElig" => Some(338),
            "oelig" => Some(339),
            "Scaron" => Some(352),
            "scaron" => Some(353),
            "Yuml" => Some(376),
            "circ" => Some(710),
            "tilde" => Some(732),
            "ensp" => Some(8194),
            "emsp" => Some(8195),
            "thinsp" => Some(8201),
            "zwnj" => Some(8204),
            "zwj" => Some(8205),
            "lrm" => Some(8206),
            "rlm" => Some(8207),
            "ndash" => Some(8211),
            "mdash" => Some(8212),
            "lsquo" => Some(8216),
            "rsquo" => Some(8217),
            "sbquo" => Some(8218),
            "ldquo" => Some(8220),
            "rdquo" => Some(8221),
            "bdquo" => Some(8222),
            "dagger" => Some(8224),
            "Dagger" => Some(8225),
            "permil" => Some(8240),
            "lsaquo" => Some(8249),
            "rsaquo" => Some(8250),
            "euro" => Some(8364),
            "fnof" => Some(402),
            "Alpha" => Some(913),
            "Beta" => Some(914),
            "Gamma" => Some(915),
            "Delta" => Some(916),
            "Epsilon" => Some(917),
            "Zeta" => Some(918),
            "Eta" => Some(919),
            "Theta" => Some(920),
            "Iota" => Some(921),
            "Kappa" => Some(922),
            "Lambda" => Some(923),
            "Mu" => Some(924),
            "Nu" => Some(925),
            "Xi" => Some(926),
            "Omicron" => Some(927),
            "Pi" => Some(928),
            "Rho" => Some(929),
            "Sigma" => Some(931),
            "Tau" => Some(932),
            "Upsilon" => Some(933),
            "Phi" => Some(934),
            "Chi" => Some(935),
            "Psi" => Some(936),
            "Omega" => Some(937),
            "alpha" => Some(945),
            "beta" => Some(946),
            "gamma" => Some(947),
            "delta" => Some(948),
            "epsilon" => Some(949),
            "zeta" => Some(950),
            "eta" => Some(951),
            "theta" => Some(952),
            "iota" => Some(953),
            "kappa" => Some(954),
            "lambda" => Some(955),
            "mu" => Some(956),
            "nu" => Some(957),
            "xi" => Some(958),
            "omicron" => Some(959),
            "pi" => Some(960),
            "rho" => Some(961),
            "sigmaf" => Some(962),
            "sigma" => Some(963),
            "tau" => Some(964),
            "upsilon" => Some(965),
            "phi" => Some(966),
            "chi" => Some(967),
            "psi" => Some(968),
            "omega" => Some(969),
            "thetasym" => Some(977),
            "upsih" => Some(978),
            "piv" => Some(982),
            "bull" => Some(8226),
            "hellip" => Some(8230),
            "prime" => Some(8242),
            "Prime" => Some(8243),
            "oline" => Some(8254),
            "frasl" => Some(8260),
            "weierp" => Some(8472),
            "image" => Some(8465),
            "real" => Some(8476),
            "trade" => Some(8482),
            "alefsym" => Some(8501),
            "larr" => Some(8592),
            "uarr" => Some(8593),
            "rarr" => Some(8594),
            "darr" => Some(8595),
            "harr" => Some(8596),
            "crarr" => Some(8629),
            "lArr" => Some(8656),
            "uArr" => Some(8657),
            "rArr" => Some(8658),
            "dArr" => Some(8659),
            "hArr" => Some(8660),
            "forall" => Some(8704),
            "part" => Some(8706),
            "exist" => Some(8707),
            "empty" => Some(8709),
            "nabla" => Some(8711),
            "isin" => Some(8712),
            "notin" => Some(8713),
            "ni" => Some(8715),
            "prod" => Some(8719),
            "sum" => Some(8721),
            "minus" => Some(8722),
            "lowast" => Some(8727),
            "radic" => Some(8730),
            "prop" => Some(8733),
            "infin" => Some(8734),
            "ang" => Some(8736),
            "and" => Some(8743),
            "or" => Some(8744),
            "cap" => Some(8745),
            "cup" => Some(8746),
            "int" => Some(8747),
            "there4" => Some(8756),
            "sim" => Some(8764),
            "cong" => Some(8773),
            "asymp" => Some(8776),
            "ne" => Some(8800),
            "equiv" => Some(8801),
            "le" => Some(8804),
            "ge" => Some(8805),
            "sub" => Some(8834),
            "sup" => Some(8835),
            "nsub" => Some(8836),
            "sube" => Some(8838),
            "supe" => Some(8839),
            "oplus" => Some(8853),
            "otimes" => Some(8855),
            "perp" => Some(8869),
            "sdot" => Some(8901),
            "lceil" => Some(8968),
            "rceil" => Some(8969),
            "lfloor" => Some(8970),
            "rfloor" => Some(8971),
            "lang" => Some(9001),
            "rang" => Some(9002),
            "loz" => Some(9674),
            "spades" => Some(9824),
            "clubs" => Some(9827),
            "hearts" => Some(9829),
            "diams" => Some(9830),
            _ => None,
        }
    }
}

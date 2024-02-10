use nom::branch::alt;
use nom::bytes::complete::tag;
use nom::character::complete::{alpha1, alphanumeric1, digit1, space0};
use nom::combinator::recognize;
use nom::multi::many0;
use nom::sequence::pair;
use nom::IResult;
use std::str;

/// Returns charset if it can be derived from the optional byte-order-mark
pub fn charset_from_bom(content: &[u8]) -> Option<&'static str> {
    match content {
        [0x00, 0x00, 0xFE, 0xFF, ..] => Some("UTF-32BE"),
        [0xFF, 0xFE, 0x00, 0x00, ..] => Some("UTF-32LE"),
        [0xFE, 0xFF, ..] => Some("UTF-16BE"),
        [0xFF, 0xFE, ..] => Some("UTF-16LE"),
        [0xEF, 0xBB, 0xBF, ..] => Some("UTF-8"),
        [0x2B, 0x2F, 0x76, ..] => Some("UTF-7"),
        _ => None,
    }
}

/// Returns charset if the encoding is specified in an XML declaration
///
/// Example XML declaration: `<?xml version="1.0" encoding="UTF-8"?>`
pub fn charset_from_xml_declaration(content: &[u8]) -> Option<String> {
    // Check for encodings by recognizing the first character(s) of `<?xml`:
    // https://www.w3.org/TR/REC-xml/#sec-guessing-no-ext-info
    let encoding_family = match content {
        [0x00, 0x00, 0x00, 0x3C, ..] => return Some("UTF-32BE".to_owned()),
        [0x3C, 0x00, 0x00, 0x00, ..] => return Some("UTF-32LE".to_owned()),
        [0x00, 0x3C, 0x00, 0x3F, ..] => return Some("UTF-16BE".to_owned()),
        [0x3C, 0x00, 0x3F, 0x00, ..] => return Some("UTF-16LE".to_owned()),
        [0x3C, 0x3F, 0x78, 0x6D, ..] => EncodingFamily::Ascii,
        [0x4C, 0x6F, 0xA7, 0x94, ..] => EncodingFamily::Ebcdic,
        _ => return None,
    };

    match encoding_family {
        EncodingFamily::Ascii => charset_from_ascii_xml_declaration(content),
        EncodingFamily::Ebcdic => None, // Unsupported for now
    }
}

enum EncodingFamily {
    Ascii,
    Ebcdic,
}

fn charset_from_ascii_xml_declaration(content: &[u8]) -> Option<String> {
    fn parse_version_info(input: &[u8]) -> IResult<&[u8], ()> {
        let (input, _) = space0(input)?;
        let (input, _) = tag(b"version=")(input)?;
        let (input, quote) = alt((tag(b"\""), tag(b"'")))(input)?;
        let (input, _) = tag(b"1.")(input)?;
        let (input, _) = digit1(input)?;
        let (input, _) = tag(quote)(input)?;
        Ok((input, ()))
    }

    fn parse_encoding_declaration(input: &[u8]) -> IResult<&[u8], &str> {
        let (input, _) = space0(input)?;
        let (input, _) = tag(b"encoding=")(input)?;
        let (input, quote) = alt((tag(b"\""), tag(b"'")))(input)?;
        let (input, encoding) = recognize(pair(
            alpha1,
            many0(alt((alphanumeric1, tag("_"), tag("-"), tag(".")))),
        ))(input)?;
        let (input, _) = tag(quote)(input)?;

        // This unwrap should be safe because the encoding can only consists of ASCII characters
        // a-z, A-Z, 0-9, '.', '_', and '-'. These ASCII characters are valid 1-byte UTF-8 sequences.
        let encoding = str::from_utf8(encoding).unwrap();

        Ok((input, encoding))
    }

    fn parse_xml_declaration(input: &[u8]) -> IResult<&[u8], String> {
        let (input, _) = tag(b"<?xml")(input)?;
        let (input, _) = parse_version_info(input)?;
        let (input, encoding) = parse_encoding_declaration(input)?;
        Ok((input, encoding.to_owned()))
    }

    parse_xml_declaration(content)
        .ok()
        .map(|(_, encoding)| encoding)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_charset_from_bom_on_empty_slice_none() {
        assert_eq!(charset_from_bom(&[]), None);
    }

    #[test]
    fn t_charset_from_bom_slice_without_bom_none() {
        assert_eq!(charset_from_bom("regular string".as_bytes()), None);
    }

    #[test]
    fn t_charset_from_bom_utf8_slice_with_bom_some() {
        assert_eq!(
            charset_from_bom("\u{feff}regular string".as_bytes()),
            Some("UTF-8")
        );
    }

    #[test]
    fn t_charset_from_bom_utf16_slice_with_bom_some() {
        let mut utf16_bytes_le = vec![];
        let mut utf16_bytes_be = vec![];
        for c in "\u{feff}regular string".encode_utf16() {
            let low = c as u8;
            let high = (c >> 8) as u8;
            utf16_bytes_le.push(low);
            utf16_bytes_le.push(high);
            utf16_bytes_be.push(high);
            utf16_bytes_be.push(low);
        }
        assert_eq!(charset_from_bom(&utf16_bytes_le), Some("UTF-16LE"));
        assert_eq!(charset_from_bom(&utf16_bytes_be), Some("UTF-16BE"));
    }

    #[test]
    fn t_charset_from_xml_declaration_empty_slice_none() {
        assert_eq!(charset_from_xml_declaration(&[]), None);
    }

    #[test]
    fn t_charset_from_xml_declaration_no_declaration_none() {
        assert_eq!(
            charset_from_xml_declaration(b"not an xml declaration"),
            None
        );
    }

    #[test]
    fn t_charset_from_xml_declaration_valid_encoding_extracted() {
        let input = br#"<?xml version="1.0" encoding="koi8-r"?>"#;
        assert_eq!(
            charset_from_xml_declaration(input),
            Some("koi8-r".to_owned())
        );
    }

    #[test]
    fn t_charset_from_xml_declaration_no_encoding_none() {
        let input = br#"<?xml version="1.0" ?>"#;
        assert_eq!(charset_from_xml_declaration(input), None);
    }

    #[test]
    fn t_charset_from_xml_declaration_invalid_encoding() {
        let input = br#"<?xml version="1.0" encoding="no spaces allowed in encoding" ?>"#;
        assert_eq!(charset_from_xml_declaration(input), None);
    }

    #[test]
    fn t_charset_from_xml_declaration_leading_space_fails_parsing() {
        let input = br#" <?xml version="1.0" encoding="koi8-r"?>"#;
        assert_eq!(charset_from_xml_declaration(input), None);
    }

    #[test]
    fn t_charset_from_xml_declaration_utf16() {
        let mut utf16_bytes_le = vec![];
        let mut utf16_bytes_be = vec![];
        for c in r#"<?xml version="1.0"?>"#.encode_utf16() {
            let low = c as u8;
            let high = (c >> 8) as u8;
            utf16_bytes_le.push(low);
            utf16_bytes_le.push(high);
            utf16_bytes_be.push(high);
            utf16_bytes_be.push(low);
        }
        assert_eq!(
            charset_from_xml_declaration(&utf16_bytes_le),
            Some("UTF-16LE".to_owned())
        );
        assert_eq!(
            charset_from_xml_declaration(&utf16_bytes_be),
            Some("UTF-16BE".to_owned())
        );
    }
}

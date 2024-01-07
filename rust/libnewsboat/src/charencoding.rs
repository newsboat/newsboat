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
}

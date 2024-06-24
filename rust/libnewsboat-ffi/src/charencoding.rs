use libnewsboat::charencoding;

#[cxx::bridge(namespace = "newsboat::charencoding::bridged")]
mod bridged {
    extern "Rust" {
        fn charset_from_bom(content: &[u8], output: &mut String) -> bool;
        fn charset_from_xml_declaration(content: &[u8], output: &mut String) -> bool;
    }
}

fn charset_from_bom(content: &[u8], output: &mut String) -> bool {
    match charencoding::charset_from_bom(content) {
        Some(charset) => {
            *output = charset.to_owned();
            true
        }
        None => false,
    }
}

fn charset_from_xml_declaration(content: &[u8], output: &mut String) -> bool {
    match charencoding::charset_from_xml_declaration(content) {
        Some(charset) => {
            *output = charset.to_owned();
            true
        }
        None => false,
    }
}

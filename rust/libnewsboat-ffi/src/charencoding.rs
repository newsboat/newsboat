use libNewsboat::charencoding;

#[cxx::bridge(namespace = "Newsboat::charencoding::bridged")]
mod bridged {
    extern "Rust" {
        fn charset_from_bom(content: &[u8], output: &mut String) -> bool;
        fn charset_from_xml_declaration(content: &[u8], output: &mut String) -> bool;
        fn charset_from_content_type_header(content: &[u8], output: &mut String) -> bool;
    }
}

// Temporarily ignore clippy lint until PR is merged:
// https://github.com/rust-lang/rust-clippy/pull/12756
#[allow(clippy::assigning_clones)]
fn charset_from_bom(content: &[u8], output: &mut String) -> bool {
    match charencoding::charset_from_bom(content) {
        Some(charset) => {
            *output = charset.to_owned();
            true
        }
        None => false,
    }
}

// Temporarily ignore clippy lint until PR is merged:
// https://github.com/rust-lang/rust-clippy/pull/12756
#[allow(clippy::assigning_clones)]
fn charset_from_xml_declaration(content: &[u8], output: &mut String) -> bool {
    match charencoding::charset_from_xml_declaration(content) {
        Some(charset) => {
            *output = charset.to_owned();
            true
        }
        None => false,
    }
}

// Temporarily ignore clippy lint until PR is merged:
// https://github.com/rust-lang/rust-clippy/pull/12756
#[allow(clippy::assigning_clones)]
fn charset_from_content_type_header(content: &[u8], output: &mut String) -> bool {
    match charencoding::charset_from_content_type_header(content) {
        Some(charset) => {
            *output = charset.to_owned();
            true
        }
        None => false,
    }
}

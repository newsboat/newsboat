use cxx::CxxString;
use libnewsboat::stflrichtext;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct StflRichText(stflrichtext::StflRichText);

#[cxx::bridge(namespace = "newsboat::stflrichtext::bridged")]
mod ffi {
    extern "Rust" {
        type StflRichText;

        fn from_plaintext(text: &CxxString) -> Box<StflRichText>;
        fn from_plaintext_with_style(text: &CxxString, style_tag: &str) -> Box<StflRichText>;
        fn from_quoted(text: &CxxString) -> Box<StflRichText>;
        fn copy(richtext: &StflRichText) -> Box<StflRichText>;
        fn append(richtext: &mut StflRichText, other: &StflRichText);

        fn highlight_searchphrase(
            richtext: &mut StflRichText,
            search: &str,
            case_insensitive: bool,
        );
        fn apply_style_tag(richtext: &mut StflRichText, tag: &str, start: usize, end: usize);
        fn plaintext(richtext: &StflRichText) -> &str;
        fn quoted(richtext: &StflRichText) -> String;
    }
}

fn from_plaintext(text: &CxxString) -> Box<StflRichText> {
    let text = text.to_string_lossy();
    let richtext = stflrichtext::StflRichText::from_plaintext(&text);
    Box::new(StflRichText(richtext))
}

fn from_plaintext_with_style(text: &CxxString, style_tag: &str) -> Box<StflRichText> {
    let text = text.to_string_lossy();
    let richtext = stflrichtext::StflRichText::from_plaintext_with_style(&text, style_tag);
    Box::new(StflRichText(richtext))
}

fn from_quoted(text: &CxxString) -> Box<StflRichText> {
    let text = text.to_string_lossy();
    let richtext = stflrichtext::StflRichText::from_quoted(&text);
    Box::new(StflRichText(richtext))
}

fn copy(richtext: &StflRichText) -> Box<StflRichText> {
    Box::new(StflRichText(richtext.0.clone()))
}

fn append(richtext: &mut StflRichText, other: &StflRichText) {
    richtext.0.append(&other.0);
}

fn highlight_searchphrase(richtext: &mut StflRichText, search: &str, case_insensitive: bool) {
    richtext.0.highlight_searchphrase(search, case_insensitive);
}

fn apply_style_tag(richtext: &mut StflRichText, tag: &str, start: usize, end: usize) {
    richtext.0.apply_style_tag(tag, start, end);
}

fn plaintext(richtext: &StflRichText) -> &str {
    richtext.0.plaintext()
}

fn quoted(richtext: &StflRichText) -> String {
    richtext.0.quoted()
}

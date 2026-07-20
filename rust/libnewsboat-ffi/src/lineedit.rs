use libnewsboat::lineedit;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct LineEdit(lineedit::LineEdit);

#[cxx::bridge(namespace = "newsboat::lineedit::bridged")]
mod bridged {
    extern "Rust" {
        type LineEdit;

        fn create() -> Box<LineEdit>;
        fn get_text(lineedit: &LineEdit) -> &str;
        fn set_text(lineedit: &mut LineEdit, text: String);
        fn get_cursor_location(lineedit: &LineEdit) -> usize;
        fn set_cursor_location(lineedit: &mut LineEdit, location: usize);
        fn insert_at_cursor(lineedit: &mut LineEdit, text: &str);
        fn handle_event(lineedit: &mut LineEdit, event: &str) -> bool;
    }
}

fn create() -> Box<LineEdit> {
    Box::new(LineEdit(lineedit::LineEdit::default()))
}

fn get_text(lineedit: &LineEdit) -> &str {
    lineedit.0.get_text()
}

fn set_text(lineedit: &mut LineEdit, text: String) {
    lineedit.0.set_text(text);
}

fn get_cursor_location(lineedit: &LineEdit) -> usize {
    lineedit.0.get_cursor_location()
}

fn set_cursor_location(lineedit: &mut LineEdit, location: usize) {
    lineedit.0.set_cursor_location(location);
}

fn insert_at_cursor(lineedit: &mut LineEdit, text: &str) {
    lineedit.0.insert_at_cursor(text);
}

fn handle_event(lineedit: &mut LineEdit, event: &str) -> bool {
    lineedit.0.handle_event(event)
}

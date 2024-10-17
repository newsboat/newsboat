use libnewsboat::tui;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct Tui(tui::Tui);
struct Form(tui::Form);

#[cxx::bridge(namespace = "newsboat::tui::bridged")]
mod bridged {
    extern "Rust" {
        type Tui;
        type Form;

        fn create_form() -> Box<Form>;

        fn create() -> Box<Tui>;
        fn reset(tui: &mut Tui);
        fn run(tui: &mut Tui, form: &mut Form, timeout: i32) -> String;
        fn get_variable(form: &mut Form, key: &str) -> String;
        fn set_variable(form: &mut Form, key: &str, value: &str);
        fn modify_form(form: &mut Form, name: &str, mode: &str, value: &str);
    }
}

fn create_form() -> Box<Form> {
    Box::new(Form(tui::Form::new()))
}

fn create() -> Box<Tui> {
    Box::new(Tui(tui::Tui::new()))
}

fn reset(tui: &mut Tui) {
    tui.0.reset();
}

fn run(tui: &mut Tui, form: &mut Form, timeout: i32) -> String {
    // TODO: Handle error?
    let event = tui.0.run(&mut form.0, timeout).unwrap();
    event.unwrap_or_default()
}

fn get_variable(form: &mut Form, key: &str) -> String {
    form.0.get_variable(key)
}

fn set_variable(form: &mut Form, key: &str, value: &str) {
    form.0.set_variable(key, value);
}

fn modify_form(form: &mut Form, name: &str, mode: &str, value: &str) {
    form.0.modify_form(name, mode, value);
}

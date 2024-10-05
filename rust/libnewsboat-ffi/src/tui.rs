use libnewsboat::tui;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct Tui(tui::Tui);

#[cxx::bridge(namespace = "newsboat::tui::bridged")]
mod bridged {
    extern "Rust" {
        type Tui;

        fn create() -> Box<Tui>;
        fn run(tui: &mut Tui);
    }
}

fn create() -> Box<Tui> {
    Box::new(Tui(tui::Tui::new()))
}

fn run(tui: &mut Tui) {
    // TODO: Handle error?
    let _ = tui.0.run();
}

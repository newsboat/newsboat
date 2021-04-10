use cxx::CxxString;
use libnewsboat::history;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct History(history::History);

#[cxx::bridge(namespace = "newsboat::history::bridged")]
mod bridged {
    extern "Rust" {
        type History;

        fn create() -> Box<History>;

        fn add_line(history: &mut History, line: &CxxString);
        fn previous_line(history: &mut History) -> String;
        fn next_line(history: &mut History) -> String;
        fn load_from_file(history: &mut History, file: &CxxString);
        fn save_to_file(history: &mut History, file: &CxxString, limit: usize);
    }
}

fn create() -> Box<History> {
    Box::new(History(history::History::new()))
}

fn add_line(history: &mut History, line: &CxxString) {
    let line = line.to_string_lossy().into_owned();
    history.0.add_line(line);
}

fn previous_line(history: &mut History) -> String {
    history.0.previous_line()
}

fn next_line(history: &mut History) -> String {
    history.0.next_line()
}

fn load_from_file(history: &mut History, file: &CxxString) {
    let file = file.to_string_lossy().into_owned();

    // Original C++ code did nothing if it failed to open file.
    // Returns a [must_use] type so safely ignoring the value.
    let _ = history.0.load_from_file(file);
}

fn save_to_file(history: &mut History, file: &CxxString, limit: usize) {
    let file = file.to_string_lossy().into_owned();

    // Original C++ code did nothing if it failed to open file.
    // Returns a [must_use] type so safely ignoring the value.
    let _ = history.0.save_to_file(file, limit);
}

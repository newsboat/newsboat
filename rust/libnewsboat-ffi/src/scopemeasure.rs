use libNewsboat::scopemeasure;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct ScopeMeasure(scopemeasure::ScopeMeasure);

#[cxx::bridge(namespace = "Newsboat::scopemeasure::bridged")]
mod bridged {
    extern "Rust" {
        type ScopeMeasure;

        fn create(scope_name: String) -> Box<ScopeMeasure>;
        fn stopover(obj: &ScopeMeasure, stopover_name: &str);
    }
}

fn create(scope_name: String) -> Box<ScopeMeasure> {
    Box::new(ScopeMeasure(scopemeasure::ScopeMeasure::new(scope_name)))
}

fn stopover(obj: &ScopeMeasure, stopover_name: &str) {
    obj.0.stopover(stopover_name);
}

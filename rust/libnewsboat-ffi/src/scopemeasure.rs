use libnewsboat::scopemeasure::ScopeMeasure;

#[cxx::bridge(namespace = "newsboat::scopemeasure::bridged")]
mod bridged {
    extern "Rust" {
        type ScopeMeasure;

        fn create(scope_name: String) -> Box<ScopeMeasure>;
        fn stopover(self: &ScopeMeasure, stopover_name: &str);
    }
}

fn create(scope_name: String) -> Box<ScopeMeasure> {
    Box::new(ScopeMeasure::new(scope_name))
}

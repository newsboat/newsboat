use libNewsboat::fmtstrformatter;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct FmtStrFormatter(fmtstrformatter::FmtStrFormatter);

#[cxx::bridge(namespace = "Newsboat::fmtstrformatter::bridged")]
mod bridged {
    extern "Rust" {
        type FmtStrFormatter;

        fn create() -> Box<FmtStrFormatter>;

        fn register_fmt(fmt: &mut FmtStrFormatter, key: u8, value: &str);
        fn do_format(fmt: &mut FmtStrFormatter, format: &str, width: u32) -> String;
    }
}

fn create() -> Box<FmtStrFormatter> {
    Box::new(FmtStrFormatter(fmtstrformatter::FmtStrFormatter::new()))
}

fn register_fmt(fmt: &mut FmtStrFormatter, key: u8, value: &str) {
    fmt.0.register_fmt(key as char, value.to_string());
}

fn do_format(fmt: &mut FmtStrFormatter, format: &str, width: u32) -> String {
    fmt.0.do_format(format, width)
}

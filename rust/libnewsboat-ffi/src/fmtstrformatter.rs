use cxx::CxxString;
use libnewsboat::fmtstrformatter;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct FmtStrFormatter(fmtstrformatter::FmtStrFormatter);

#[cxx::bridge(namespace = "newsboat::fmtstrformatter::bridged")]
mod bridged {
    extern "Rust" {
        type FmtStrFormatter;

        fn create() -> Box<FmtStrFormatter>;

        fn register_fmt(fmt: &mut FmtStrFormatter, key: u8, value: &CxxString);
        fn do_format(fmt: &mut FmtStrFormatter, format: &CxxString, width: u32) -> String;
    }
}

fn create() -> Box<FmtStrFormatter> {
    Box::new(FmtStrFormatter(fmtstrformatter::FmtStrFormatter::new()))
}

fn register_fmt(fmt: &mut FmtStrFormatter, key: u8, value: &CxxString) {
    let value = value.to_string_lossy().into_owned();

    fmt.0.register_fmt(key as char, value);
}

fn do_format(fmt: &mut FmtStrFormatter, format: &CxxString, width: u32) -> String {
    let format = format.to_string_lossy().into_owned();

    fmt.0.do_format(&format, width)
}

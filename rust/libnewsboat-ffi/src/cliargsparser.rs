use cxx::{ExternType, type_id};

use crate::filepath::PathBuf;
use libnewsboat::cliargsparser;
use std::ffi::OsString;
use std::os::unix::ffi::OsStringExt;
use std::pin::Pin;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct CliArgsParser(pub cliargsparser::CliArgsParser);

unsafe impl ExternType for CliArgsParser {
    type Id = type_id!("newsboat::cliargsparser::bridged::CliArgsParser");
    type Kind = cxx::kind::Opaque;
}

#[cxx::bridge(namespace = "newsboat::cliargsparser::bridged")]
mod bridged {
    #[namespace = "newsboat::filepath::bridged"]
    extern "C++" {
        include!("libnewsboat-ffi/src/filepath.rs.h");

        type PathBuf = crate::filepath::PathBuf;
    }

    /// Shared data structure between C++/Rust. We need to do this, because
    /// Vec<Vec<T>> is an unsupported CXX type.
    struct BytesVec {
        pub data: Vec<u8>,
    }

    extern "Rust" {
        type CliArgsParser;

        fn create(argv: Vec<BytesVec>) -> Box<CliArgsParser>;

        fn do_import(cliargsparser: &CliArgsParser) -> bool;
        fn do_export(cliargsparser: &CliArgsParser) -> bool;
        fn export_as_opml2(cliargsparser: &CliArgsParser) -> bool;
        fn do_vacuum(cliargsparser: &CliArgsParser) -> bool;
        fn do_cleanup(cliargsparser: &CliArgsParser) -> bool;
        fn do_show_version(cliargsparser: &CliArgsParser) -> u64;
        fn silent(cliargsparser: &CliArgsParser) -> bool;
        fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool;
        fn should_print_usage(cliargsparser: &CliArgsParser) -> bool;
        fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool;

        fn importfile(cliargsparser: &CliArgsParser, mut file: Pin<&mut PathBuf>);
        fn program_name(cliargsparser: &CliArgsParser) -> String;
        fn display_msg(cliargsparser: &CliArgsParser) -> String;

        fn return_code(cliargsparser: &CliArgsParser, value: &mut isize) -> bool;

        fn readinfo_import_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>)
            -> bool;
        fn readinfo_export_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>)
            -> bool;
        fn url_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn lock_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn cache_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn config_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn queue_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn search_history_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;
        fn cmdline_history_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>)
            -> bool;
        fn log_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool;

        fn cmds_to_execute(cliargsparser: &CliArgsParser) -> Vec<String>;

        fn log_level(cliargsparser: &CliArgsParser, level: &mut i8) -> bool;
    }
}

fn create(argv: Vec<bridged::BytesVec>) -> Box<CliArgsParser> {
    let os_str_argv: Vec<OsString> = argv
        .into_iter()
        .map(|arg_bytes| OsString::from_vec(arg_bytes.data))
        .collect();
    Box::new(CliArgsParser(cliargsparser::CliArgsParser::new(
        os_str_argv,
    )))
}

fn do_import(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.importfile.is_some()
}

fn do_export(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.do_export
}

fn export_as_opml2(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.export_as_opml2
}

fn do_vacuum(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.do_vacuum
}

fn do_cleanup(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.do_cleanup
}

fn do_show_version(cliargsparser: &CliArgsParser) -> u64 {
    cliargsparser.0.show_version as u64
}

fn silent(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.silent
}

fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.using_nonstandard_configs()
}

fn should_print_usage(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.should_print_usage
}

fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.0.refresh_on_start
}

fn importfile(cliargsparser: &CliArgsParser, mut output: Pin<&mut PathBuf>) {
    match &cliargsparser.0.importfile {
        Some(path) => output.0 = path.to_owned(),
        None => output.0.clear(),
    }
}

fn program_name(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.0.program_name.to_string()
}

fn display_msg(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.0.display_msg.to_string()
}

fn return_code(cliargsparser: &CliArgsParser, value: &mut isize) -> bool {
    match cliargsparser.0.return_code {
        Some(code) => {
            *value = code as isize;
            true
        }
        None => false,
    }
}

fn readinfo_import_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.readinfo_import_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn readinfo_export_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.readinfo_export_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn url_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.url_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn lock_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.lock_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn cache_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.cache_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn config_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.config_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn queue_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.queue_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn search_history_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.search_history_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn cmdline_history_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.cmdline_history_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn log_file(cliargsparser: &CliArgsParser, mut path: Pin<&mut PathBuf>) -> bool {
    match &cliargsparser.0.log_file {
        Some(p) => {
            path.0 = p.to_owned();
            true
        }
        None => false,
    }
}

fn cmds_to_execute(cliargsparser: &CliArgsParser) -> Vec<String> {
    cliargsparser.0.cmds_to_execute.to_owned()
}

fn log_level(cliargsparser: &CliArgsParser, level: &mut i8) -> bool {
    match cliargsparser.0.log_level {
        Some(l) => {
            *level = l as i8;
            true
        }
        None => false,
    }
}

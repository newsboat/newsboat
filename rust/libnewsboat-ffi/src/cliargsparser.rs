use libnewsboat::cliargsparser::CliArgsParser;

#[cxx::bridge(namespace = "newsboat::cliargsparser::bridged")]
mod bridged {
    extern "Rust" {
        type CliArgsParser;

        fn create(argv: Vec<String>) -> Box<CliArgsParser>;

        fn do_import(cliargsparser: &CliArgsParser) -> bool;
        fn do_export(cliargsparser: &CliArgsParser) -> bool;
        fn do_vacuum(cliargsparser: &CliArgsParser) -> bool;
        fn do_cleanup(cliargsparser: &CliArgsParser) -> bool;
        fn do_show_version(cliargsparser: &CliArgsParser) -> u64;
        fn silent(cliargsparser: &CliArgsParser) -> bool;
        fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool;
        fn should_print_usage(cliargsparser: &CliArgsParser) -> bool;
        fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool;

        fn importfile(cliargsparser: &CliArgsParser) -> String;
        fn program_name(cliargsparser: &CliArgsParser) -> String;
        fn display_msg(cliargsparser: &CliArgsParser) -> String;

        fn return_code(cliargsparser: &CliArgsParser, value: &mut isize) -> bool;

        fn readinfo_import_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn readinfo_export_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn url_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn lock_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn cache_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn config_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;
        fn log_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool;

        fn cmds_to_execute(cliargsparser: &CliArgsParser) -> Vec<String>;

        fn log_level(cliargsparser: &CliArgsParser, level: &mut i8) -> bool;
    }

    extern "C++" {
        // cxx uses `std::out_of_range`, but doesn't include the header that defines that
        // exception. So we do it for them.
        include!("stdexcept");
        // Also inject a header that defines ptrdiff_t. Note this is *not* a C++ header, because
        // cxx uses a non-C++ name of the type.
        include!("stddef.h");
    }
}

fn create(argv: Vec<String>) -> Box<CliArgsParser> {
    Box::new(CliArgsParser::new(argv))
}

fn do_import(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.importfile.is_some()
}

fn do_export(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_export
}

fn do_vacuum(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_vacuum
}

fn do_cleanup(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_cleanup
}

fn do_show_version(cliargsparser: &CliArgsParser) -> u64 {
    cliargsparser.show_version as u64
}

fn silent(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.silent
}

fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.using_nonstandard_configs()
}

fn should_print_usage(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.should_print_usage
}

fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.refresh_on_start
}

fn importfile(cliargsparser: &CliArgsParser) -> String {
    match &cliargsparser.importfile {
        Some(path) => path.to_string_lossy().to_string(),
        None => String::new(),
    }
}

fn program_name(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.program_name.to_string()
}

fn display_msg(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.display_msg.to_string()
}

fn return_code(cliargsparser: &CliArgsParser, value: &mut isize) -> bool {
    match cliargsparser.return_code {
        Some(code) => {
            *value = code as isize;
            true
        }
        None => false,
    }
}

fn readinfo_import_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.readinfo_import_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn readinfo_export_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.readinfo_export_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn url_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.url_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn lock_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.lock_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn cache_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.cache_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn config_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.config_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn log_file(cliargsparser: &CliArgsParser, path: &mut String) -> bool {
    match &cliargsparser.log_file {
        Some(p) => {
            *path = p.to_string_lossy().to_string();
            true
        }
        None => false,
    }
}

fn cmds_to_execute(cliargsparser: &CliArgsParser) -> Vec<String> {
    cliargsparser.cmds_to_execute.to_owned()
}

fn log_level(cliargsparser: &CliArgsParser, level: &mut i8) -> bool {
    match cliargsparser.log_level {
        Some(l) => {
            *level = l as i8;
            true
        }
        None => false,
    }
}

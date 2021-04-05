use libnewsboat::logger;

#[cxx::bridge(namespace = "newsboat::logger::bridged")]
mod bridged {
    // This has to be in sync with logger::Level in rust/libnewsboat/src/logger.rs
    enum Level {
        USERERROR = 1,
        CRITICAL,
        ERROR,
        WARN,
        INFO,
        DEBUG,
    }

    extern "Rust" {
        fn unset_loglevel();
        fn set_logfile(logfile: &str);
        fn get_loglevel() -> i64;
        fn set_loglevel(level: Level);
        fn log(level: Level, message: &str);
        fn set_user_error_logfile(user_error_logfile: &str);
    }
}

fn ffi_level_to_log_level(level: bridged::Level) -> logger::Level {
    match level {
        bridged::Level::USERERROR => logger::Level::UserError,
        bridged::Level::CRITICAL => logger::Level::Critical,
        bridged::Level::ERROR => logger::Level::Error,
        bridged::Level::WARN => logger::Level::Warn,
        bridged::Level::INFO => logger::Level::Info,
        bridged::Level::DEBUG => logger::Level::Debug,
        _ => panic!("Unknown log level"),
    }
}

fn unset_loglevel() {
    logger::get_instance().unset_loglevel();
}

fn set_logfile(logfile: &str) {
    logger::get_instance().set_logfile(logfile);
}

fn get_loglevel() -> i64 {
    logger::get_instance().get_loglevel() as i64
}

fn set_loglevel(level: bridged::Level) {
    let level = ffi_level_to_log_level(level);
    logger::get_instance().set_loglevel(level);
}

fn log(level: bridged::Level, message: &str) {
    let level = ffi_level_to_log_level(level);
    logger::get_instance().log(level, message);
}

fn set_user_error_logfile(user_error_logfile: &str) {
    logger::get_instance().set_user_error_logfile(user_error_logfile);
}

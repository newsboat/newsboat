use cxx::CxxString;
use libNewsboat::logger;

#[cxx::bridge(namespace = "Newsboat::logger")]
mod ffi {
    // This has to be in sync with logger::Level in rust/libNewsboat/src/logger.rs
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
        fn log_internal(level: Level, message: &CxxString);
        fn set_user_error_logfile(user_error_logfile: &str);
    }
}

fn ffi_level_to_log_level(level: ffi::Level) -> logger::Level {
    match level {
        ffi::Level::USERERROR => logger::Level::UserError,
        ffi::Level::CRITICAL => logger::Level::Critical,
        ffi::Level::ERROR => logger::Level::Error,
        ffi::Level::WARN => logger::Level::Warn,
        ffi::Level::INFO => logger::Level::Info,
        ffi::Level::DEBUG => logger::Level::Debug,
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

fn set_loglevel(level: ffi::Level) {
    let level = ffi_level_to_log_level(level);
    logger::get_instance().set_loglevel(level);
}

fn log_internal(level: ffi::Level, message: &CxxString) {
    let level = ffi_level_to_log_level(level);
    logger::get_instance().log_raw(level, message.as_bytes());
}

fn set_user_error_logfile(user_error_logfile: &str) {
    logger::get_instance().set_user_error_logfile(user_error_logfile);
}

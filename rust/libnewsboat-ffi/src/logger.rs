use libnewsboat::logger;

#[cxx::bridge(namespace = "newsboat::logger::bridged")]
mod bridged {
    extern "Rust" {
        fn unset_loglevel();
        fn set_logfile(logfile: &str);
        fn get_loglevel() -> i64;
        fn set_loglevel(level: i16);
        fn log(level: i16, message: &str);
        fn set_user_error_logfile(user_error_logfile: &str);
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

fn set_loglevel(level: i16) {
    let level = match level {
        1 => logger::Level::UserError,
        2 => logger::Level::Critical,
        3 => logger::Level::Error,
        4 => logger::Level::Warn,
        5 => logger::Level::Info,
        6 => logger::Level::Debug,
        _ => panic!("Unknown log level"),
    };
    logger::get_instance().set_loglevel(level);
}

fn log(level: i16, message: &str) {
    let level = match level {
        1 => logger::Level::UserError,
        2 => logger::Level::Critical,
        3 => logger::Level::Error,
        4 => logger::Level::Warn,
        5 => logger::Level::Info,
        6 => logger::Level::Debug,
        _ => panic!("Unknown log level"),
    };
    logger::get_instance().log(level, message);
}

fn set_user_error_logfile(user_error_logfile: &str) {
    logger::get_instance().set_user_error_logfile(user_error_logfile);
}

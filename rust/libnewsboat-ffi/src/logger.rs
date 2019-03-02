use abort_on_panic;
use libc::c_char;
use libnewsboat::logger;
use std::ffi::CStr;

#[no_mangle]
pub extern "C" fn rs_log(level: logger::Level, message: *const c_char) {
    abort_on_panic(|| {
        let message = unsafe { CStr::from_ptr(message) };
        logger::get_instance().log_raw(level, message.to_bytes());
    })
}

#[no_mangle]
pub extern "C" fn rs_set_loglevel(level: logger::Level) {
    abort_on_panic(|| {
        logger::get_instance().set_loglevel(level);
    })
}

#[no_mangle]
pub extern "C" fn rs_set_logfile(logfile: *const c_char) {
    abort_on_panic(|| {
        let logfile = unsafe { CStr::from_ptr(logfile) };
        let logfile = logfile
            .to_str()
            .expect("logfile path contained invalid UTF-8");
        logger::get_instance().set_logfile(logfile);
    })
}

#[no_mangle]
pub extern "C" fn rs_set_user_error_logfile(user_error_logfile: *const c_char) {
    abort_on_panic(|| {
        let user_error_logfile = unsafe { CStr::from_ptr(user_error_logfile) };
        let user_error_logfile = user_error_logfile
            .to_str()
            .expect("user_error_logfile path contained invalid UTF-8");
        logger::get_instance().set_user_error_logfile(user_error_logfile);
    })
}

#[no_mangle]
pub extern "C" fn rs_get_loglevel() -> u64 {
    abort_on_panic(|| logger::get_instance().get_loglevel() as u64)
}

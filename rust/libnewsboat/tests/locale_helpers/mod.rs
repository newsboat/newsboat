use libc::{LC_CTYPE, setlocale};
use std::env;
use std::ffi::CString;

pub fn set_locale(new_locale: &str) {
    unsafe {
        let c_new_locale = CString::new(new_locale).expect("New locale's name contains a NUL byte");
        let locale_set = setlocale(LC_CTYPE, c_new_locale.as_ptr());

        if locale_set.is_null() {
            panic!("Couldn't set locale {}; test skipped.", new_locale);
        }

        env::set_var("LC_CTYPE", new_locale);
    }
}

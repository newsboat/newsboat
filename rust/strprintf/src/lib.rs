pub mod specifiers_iterator;

/// A safe-ish wrapper around `libc::snprintf`.
#[macro_export]
macro_rules! fmt {
    ( $format:expr ) => {
        {
            let format: &str = $format;
            String::from(format)
        }
    };

    ( $format:expr, $( $arg:expr ),+ ) => {
        {
            let format: &str = $format;

            let mut result = String::new();

            use $crate::specifiers_iterator::SpecifiersIterator;
            let mut specifiers = SpecifiersIterator::from(format);

            $(
                let local_format = specifiers.next().unwrap_or("");
                // Might fail if `local_format` contains a null byte. TODO decide if this is a good
                // error-handling strategy, or we should report back to user, or panic, or what
                if let Ok(local_format_cstring) = std::ffi::CString::new(local_format) {
                    let buf_size = 1024usize;
                    let buffer: std::vec::Vec<u8> = std::vec::Vec::with_capacity(buf_size);
                    if let Ok(buffer) = std::ffi::CString::new(buffer) {
                        unsafe {
                            let buffer_ptr = buffer.into_raw();
                            // TODO: check how many bytes were written
                            let bytes_written = libc::snprintf(
                                buffer_ptr,
                                buf_size as libc::size_t,
                                local_format_cstring.as_ptr() as *const libc::c_char,
                                $arg);
                            let buffer = std::ffi::CString::from_raw(buffer_ptr);
                            let _bytes_written = bytes_written as usize;
                            if let Ok(formatted) = buffer.into_string() {
                                result.push_str(&formatted);
                            }
                        }
                    }
                }
            )+

            result
        }
    };
}

#[cfg(test)]
mod tests {
    #[test]
    fn returns_first_argument_if_it_is_the_only_one() {
        let input = String::from("Hello, world!");
        assert_eq!(fmt!(&input), input);
    }

    #[test]
    fn replaces_printf_format_with_text_representation_of_an_argument() {
        assert_eq!(fmt!("%i", 42), "42");
        assert_eq!(fmt!("%i", -13), "-13");
        assert_eq!(fmt!("%.3f", 3.1416), "3.142");
        assert_eq!(fmt!("%i %i", 100500, -191), "100500 -191");
    }
}

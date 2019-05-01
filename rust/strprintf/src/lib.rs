extern crate libc;

pub mod specifiers_iterator;
pub mod traits;

use std::ffi::CString;
use std::vec::Vec;
use traits::*;

// TODO: write an internal module doc explaining the interplay between traits, fmt_arg, and fmt!

/// Helper function for `fmt!`. Don't use directly.
pub fn fmt_arg<T>(format: &str, arg: T) -> Option<String>
where
    T: Printfable,
{
    // Might fail if `format` contains a null byte. TODO decide if this is a good
    // error-handling strategy, or we should report back to user, or panic, or what
    CString::new(format).ok().and_then(|local_format_cstring| {
        let buf_size = 1024usize;
        let mut buffer = Vec::<u8>::with_capacity(buf_size);
        // Filling the vector with ones because CString::new() doesn't want any zeroes in
        // there. The last byte is left unused, so that CString::new() can put a terminating
        // zero byte without triggering a re-allocation.
        buffer.resize(buf_size - 1, 1);
        CString::new(buffer).ok().and_then(|buffer| unsafe {
            let buffer_ptr = buffer.into_raw();
            // TODO: check how many bytes were written
            arg.to_c_repr_holder().and_then(|arg_c_repr_holder| {
                let bytes_written = libc::snprintf(
                    buffer_ptr,
                    buf_size as libc::size_t,
                    local_format_cstring.as_ptr() as *const libc::c_char,
                    arg_c_repr_holder.to_c_repr(),
                );
                let buffer = CString::from_raw(buffer_ptr);
                let _bytes_written = bytes_written as usize;
                buffer.into_string().ok()
            })
        })
    })
}

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
                if let Some(formatted_string) = $crate::fmt_arg(local_format, $arg) {
                    result.push_str(&formatted_string);
                }
            )+

            result
        }
    };
}

#[cfg(test)]
mod tests {
    extern crate libc;
    use std;

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

    #[test]
    fn formats_i32() {
        assert_eq!(fmt!("%i", 42i32), "42");
        assert_eq!(fmt!("%i", std::i32::MIN), "-2147483648");
        assert_eq!(fmt!("%i", std::i32::MAX), "2147483647");
    }

    #[test]
    fn formats_u32() {
        assert_eq!(fmt!("%u", 42u32), "42");
        assert_eq!(fmt!("%u", 0u32), "0");
        assert_eq!(fmt!("%u", std::u32::MAX), "4294967295");
    }

    #[test]
    fn formats_i64() {
        assert_eq!(fmt!("%li", 42i64), "42");
        assert_eq!(fmt!("%li", std::i32::MIN as i64 - 1), "-2147483649");
        assert_eq!(fmt!("%li", std::i32::MAX as i64 + 1), "2147483648");

        assert_eq!(fmt!("%lli", 42i64), "42");
        assert_eq!(fmt!("%lli", std::i64::MIN), "-9223372036854775808");
        assert_eq!(fmt!("%lli", std::i64::MAX), "9223372036854775807");
    }

    #[test]
    fn formats_u64() {
        assert_eq!(fmt!("%lu", 42u64), "42");
        assert_eq!(fmt!("%lu", 0u64), "0");
        assert_eq!(fmt!("%lu", std::u64::MAX), "18446744073709551615");

        assert_eq!(fmt!("%llu", 42u64), "42");
        assert_eq!(fmt!("%llu", 0u64), "0");
        assert_eq!(fmt!("%llu", std::u64::MAX), "18446744073709551615");
    }

    #[test]
    fn formats_void_ptr() {
        let x = 42i64;
        let ptr = &x as *const i64;
        assert_ne!(fmt!("%p", ptr as *const libc::c_void), "");
    }

    #[test]
    fn formats_null_ptr() {
        assert_eq!(fmt!("%p", std::ptr::null::<i32>()), "(nil)");
        assert_eq!(fmt!("%p", std::ptr::null::<u32>()), "(nil)");
        assert_eq!(fmt!("%p", std::ptr::null::<i64>()), "(nil)");
        assert_eq!(fmt!("%p", std::ptr::null::<u64>()), "(nil)");
        assert_eq!(fmt!("%p", std::ptr::null::<f32>()), "(nil)");
        assert_eq!(fmt!("%p", std::ptr::null::<f64>()), "(nil)");
    }

    #[test]
    fn formats_float() {
        let x = 42.0f32;
        assert_eq!(fmt!("%f", x), "42.000000");
        assert_eq!(fmt!("%.3f", x), "42.000");

        let y = 42e3f32;
        assert_eq!(fmt!("%e", y), "4.200000e+04");
        assert_eq!(fmt!("%.3e", y), "4.200e+04");

        assert_eq!(fmt!("%f", std::f32::INFINITY), "inf");
        assert_eq!(fmt!("%F", std::f32::INFINITY), "INF");

        assert_eq!(fmt!("%f", std::f32::NEG_INFINITY), "-inf");
        assert_eq!(fmt!("%F", std::f32::NEG_INFINITY), "-INF");

        assert_eq!(fmt!("%f", std::f32::NAN), "nan");
        assert_eq!(fmt!("%F", std::f32::NAN), "NAN");
    }

    #[test]
    fn formats_double() {
        let x = 42.0f64;
        assert_eq!(fmt!("%f", x), "42.000000");
        assert_eq!(fmt!("%.3f", x), "42.000");

        let y = 42e138f64;
        assert_eq!(fmt!("%e", y), "4.200000e+139");
        assert_eq!(fmt!("%.3e", y), "4.200e+139");

        assert_eq!(fmt!("%f", std::f64::INFINITY), "inf");
        assert_eq!(fmt!("%F", std::f64::INFINITY), "INF");

        assert_eq!(fmt!("%f", std::f64::NEG_INFINITY), "-inf");
        assert_eq!(fmt!("%F", std::f64::NEG_INFINITY), "-INF");

        assert_eq!(fmt!("%f", std::f64::NAN), "nan");
        assert_eq!(fmt!("%F", std::f64::NAN), "NAN");
    }

    #[test]
    fn formats_str_slice() {
        let input = "Hello, world!";
        assert_eq!(fmt!("%s", input), input);
    }

    /*
    // TODO: write the same test for str
    // TODO: write the same test for String
    TEST_CASE("strprintf::fmt() formats std::string", "[strprintf]")
    {
            const auto input = std::string("Hello, world!");
            REQUIRE(strprintf::fmt("%s", input) == input);
    }

    TEST_CASE("strprintf::fmt() works fine with 1MB format string", "[strprintf]")
    {
            const auto format = std::string(1024 * 1024, ' ');
            REQUIRE(strprintf::fmt(format) == format);
    }
    */
}

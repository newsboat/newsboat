// Problem statement for `strprintf` crate: provide a way to interpolate printf-style format
// strings using native Rust types. For example, it should be possible to format a string
// "%i %.2f %x" using values 42u32, 3.1415f64, and 255u8, and get a `std::string::String` "42 3.14
// ff".
//
// This is the same as strprintf module we already have in C++.
//
// The problem can be solved by wrapping `libc::snprintf`, which is what we do both here and in
// C++. However, our experience with C++ showed that we should constrain what types can be
// formatted. Otherwise, complex objects like `String` will be passed over FFI and lead to
// unexpected results (e.g. garbage strings).
//
// To achieve that, we provide a `Printfable` trait that's implemented only for types that our
// formatting macro accepts. Everything else will result in a compile-time error. See the docs in
// `trait` module for more on that.

use libc;

pub mod specifiers_iterator;
// Re-exporting so that macro can just import the whole crate and get everything it needs.
pub use crate::specifiers_iterator::SpecifiersIterator;

pub mod traits;

use crate::traits::*;
use std::ffi::{CStr, CString};
use std::mem;
use std::vec::Vec;

// Re-exporting platform-specific format specifiers.
#[cfg(target_pointer_width = "32")]
mod format_specifiers_32bit;
#[cfg(target_pointer_width = "32")]
pub use format_specifiers_32bit::*;

#[cfg(target_pointer_width = "64")]
mod format_specifiers_64bit;
#[cfg(target_pointer_width = "64")]
pub use crate::format_specifiers_64bit::*;

/// Helper function to `fmt!`. **Use it only through that macro!**
///
/// Returns a formatted string, or the size of the buffer that's necessary to hold the formatted
/// string.
#[doc(hidden)]
pub fn fmt_arg_with_buffer_size<T>(
    format_cstring: &CStr,
    arg_c_repr_holder: &T,
    buf_size: usize,
) -> Result<String, usize>
where
    T: CReprHolder,
{
    let mut buffer = Vec::<u8>::with_capacity(buf_size);
    let buffer_ptr = buffer.as_mut_ptr();
    unsafe {
        // We're passing the buffer to C, so let's make Rust forget about it for a while.
        mem::forget(buffer);

        let bytes_written = libc::snprintf(
            buffer_ptr as *mut libc::c_char,
            buf_size as libc::size_t,
            format_cstring.as_ptr() as *const libc::c_char,
            arg_c_repr_holder.to_c_repr(),
        ) as usize;
        if bytes_written >= buf_size {
            let _ = Vec::from_raw_parts(buffer_ptr, buf_size, buf_size);
            Err(bytes_written + 1)
        } else {
            let buffer = Vec::from_raw_parts(buffer_ptr, bytes_written, buf_size);
            Ok(String::from_utf8_lossy(&buffer).into_owned())
        }
    }
}

/// Helper function to `fmt!`. **Use it only through that macro!**
#[doc(hidden)]
pub fn fmt_arg<T>(format: &str, arg: T) -> Option<String>
where
    T: Printfable,
{
    // Returns None if `format` contains a null byte
    CString::new(format).ok().and_then(|local_format_cstring| {
        // Returns None if a holder couldn't be obtained - e.g. the value is a String that
        // contains a null byte
        arg.to_c_repr_holder().and_then(|arg_c_repr_holder| {
            match fmt_arg_with_buffer_size(&local_format_cstring, &arg_c_repr_holder, 1024) {
                Ok(formatted) => Some(formatted),
                Err(buf_size) => {
                    fmt_arg_with_buffer_size(&local_format_cstring, &arg_c_repr_holder, buf_size)
                        .ok()
                }
            }
        })
    })
}

/// A safe-ish wrapper around `libc::snprintf`.
///
/// It pairs each format specifier ("%i", "%.2f" etc.) with a value, and passes those to
/// `libc::snprinf`; the results are then concatenated.
///
/// If a pair couldn't be formatted, it's omitted from the output. This can happen if:
/// - a format string contains null bytes;
/// - the string value to be formatted contains null bytes;
/// - `libc::snprintf` failed to format things even when given large enough buffer to do so.
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

            use $crate::*;
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
    use libc;

    #[test]
    fn returns_first_argument_if_it_is_the_only_one() {
        let input = String::from("Hello, world!");
        assert_eq!(fmt!(&input), input);
    }

    #[test]
    fn replaces_printf_format_with_text_representation_of_an_argument() {
        assert_eq!(fmt!("%i", 42), "42");
        assert_eq!(fmt!("%i", -13), "-13");
        assert_eq!(fmt!("%.3f", 1.2468), "1.247");
        assert_eq!(fmt!("%i %i", 100500, -191), "100500 -191");
    }

    #[test]
    fn formats_i32() {
        assert_eq!(fmt!(&format!("%{}", PRIi32), 42i32), "42");
        assert_eq!(fmt!(&format!("%{}", PRId32), 42i32), "42");

        assert_eq!(fmt!(&format!("%{}", PRIi32), std::i32::MIN), "-2147483648");
        assert_eq!(fmt!(&format!("%{}", PRId32), std::i32::MIN), "-2147483648");

        assert_eq!(fmt!(&format!("%{}", PRIi32), std::i32::MAX), "2147483647");
        assert_eq!(fmt!(&format!("%{}", PRId32), std::i32::MAX), "2147483647");
    }

    #[test]
    fn formats_u32() {
        assert_eq!(fmt!(&format!("%{}", PRIu32), 42u32), "42");
        assert_eq!(fmt!(&format!("%{}", PRIu32), 0u32), "0");
        assert_eq!(fmt!(&format!("%{}", PRIu32), std::u32::MAX), "4294967295");
    }

    #[test]
    fn formats_i64() {
        assert_eq!(fmt!(&format!("%{}", PRIi64), 42i64), "42");
        assert_eq!(fmt!(&format!("%{}", PRId64), 42i64), "42");

        assert_eq!(
            fmt!(&format!("%{}", PRIi64), std::i64::MIN),
            "-9223372036854775808"
        );
        assert_eq!(
            fmt!(&format!("%{}", PRId64), std::i64::MIN),
            "-9223372036854775808"
        );

        assert_eq!(
            fmt!(&format!("%{}", PRIi64), std::i64::MAX),
            "9223372036854775807"
        );
        assert_eq!(
            fmt!(&format!("%{}", PRId64), std::i64::MAX),
            "9223372036854775807"
        );
    }

    #[test]
    fn formats_u64() {
        assert_eq!(fmt!(&format!("%{}", PRIu64), 42u64), "42");
        assert_eq!(fmt!(&format!("%{}", PRIu64), 0u64), "0");
        assert_eq!(
            fmt!(&format!("%{}", PRIu64), std::u64::MAX),
            "18446744073709551615"
        );
    }

    #[test]
    fn formats_pointers() {
        let x = 42i64;

        let x_ptr = &x as *const i64;
        let x_ptr_formatted = fmt!("%p", x_ptr);

        let x_ptr_void = x_ptr as *const libc::c_void;
        let x_ptr_void_formatted = fmt!("%p", x_ptr_void);
        assert!(!x_ptr_formatted.is_empty());
        assert_eq!(x_ptr_formatted, x_ptr_void_formatted);
    }

    #[test]
    fn formats_all_null_pointers_the_same() {
        let i32_ptr_fmt = fmt!("%p", std::ptr::null::<i32>());
        let u32_ptr_fmt = fmt!("%p", std::ptr::null::<u32>());
        let i64_ptr_fmt = fmt!("%p", std::ptr::null::<i64>());
        let u64_ptr_fmt = fmt!("%p", std::ptr::null::<u64>());
        let f32_ptr_fmt = fmt!("%p", std::ptr::null::<f32>());
        let f64_ptr_fmt = fmt!("%p", std::ptr::null::<f64>());
        assert!(!i32_ptr_fmt.is_empty());
        assert_eq!(i32_ptr_fmt, u32_ptr_fmt);
        assert_eq!(u32_ptr_fmt, i64_ptr_fmt);
        assert_eq!(i64_ptr_fmt, u64_ptr_fmt);
        assert_eq!(u64_ptr_fmt, f32_ptr_fmt);
        assert_eq!(f32_ptr_fmt, f64_ptr_fmt);
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

    #[test]
    fn formats_borrowed_string() {
        let input = String::from("Hello, world!");
        assert_eq!(fmt!("%s", &input), input);
    }

    #[test]
    fn formats_moved_string() {
        let input = String::from("Hello, world!");
        assert_eq!(fmt!("%s", input.clone()), input);
    }

    #[test]
    fn formats_2_megabyte_string() {
        let spacer = String::from(" ").repeat(1024 * 1024);
        let format = {
            let mut result = spacer.clone();
            result.push_str("%i");
            result.push_str(&spacer);
            result.push_str("%i");
            result
        };
        let expected = {
            let mut result = spacer.clone();
            result.push_str("42");
            result.push_str(&spacer);
            result.push_str("100500");
            result
        };
        assert_eq!(fmt!(&format, 42, 100500), expected);
    }
}

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

    #[test]
    fn formats_int_x86_64() {
        assert_eq!(fmt!("%i", 42i32), "42");

        let int_min = std::i32::MIN;
        assert_eq!(int_min, -2147483648i32);
        assert_eq!(fmt!("%i", int_min), "-2147483648");

        let int_max = std::i32::MAX;
        assert_eq!(int_max, 2147483647i32);
        assert_eq!(fmt!("%i", int_max), "2147483647");
    }

    #[test]
    fn formats_unsigned_int_x86_64() {
        assert_eq!(fmt!("%u", 42u32), "42");

        assert_eq!(fmt!("%u", 0u32), "0");

        let uint_max = std::u32::MAX;
        assert_eq!(uint_max, 4294967295u32);
        assert_eq!(fmt!("%u", uint_max), "4294967295");
    }

    #[test]
    fn formats_long_int_x86_64() {
        assert_eq!(fmt!("%li", 42i64), "42");

        let int_min = std::i32::MIN as i64;
        let long_min = std::i64::MIN;
        assert!(long_min < int_min);
        assert_eq!(int_min, -2147483648);
        assert_eq!(fmt!("%li", int_min - 1), "-2147483649");

        let int_max = std::i32::MAX as i64;
        let long_max = std::i64::MAX;
        assert!(long_max > int_max);
        assert_eq!(int_max, 2147483647);
        assert_eq!(fmt!("%li", int_max + 1), "2147483648");
    }

    #[test]
    fn formats_long_unsigned_int_x86_64() {
        assert_eq!(fmt!("%lu", 42u64), "42");

        assert_eq!(fmt!("%lu", 0u64), "0");

        let ulong_max = std::u64::MAX;
        assert_eq!(ulong_max, 18446744073709551615u64);
        assert_eq!(fmt!("%lu", ulong_max), "18446744073709551615");
    }

    #[test]
    fn formats_long_long_int_x86_64() {
        assert_eq!(fmt!("%lli", 42i64), "42");

        // This is one bigger than actual std::numeric_limits<long long int>::min()
        // on x86_64. The actual value can't be written in C++ as a literal because both GCC 8 and
        // Clang complain about it being too small to be represented with long long int.
        let input = -9223372036854775807i64;
        let llong_min = std::i64::MIN;
        assert!(llong_min <= input);
        assert_eq!(fmt!("%lli", input), "-9223372036854775807");

        let llong_max = std::i64::MAX;
        assert_eq!(llong_max, 9223372036854775807i64);
        assert_eq!(fmt!("%lli", llong_max), "9223372036854775807");
    }

    #[test]
    fn formats_unsigned_long_long_int_x86_64() {
        assert_eq!(fmt!("%llu", 42u64), "42");

        assert_eq!(fmt!("%llu", 0u64), "0");

        let ullong_max = std::u64::MAX;
        assert_eq!(ullong_max, 18446744073709551615u64);
        assert_eq!(fmt!("%llu", ullong_max), "18446744073709551615");
    }

    /*
    TEST_CASE("strprintf::fmt() formats void*", "[strprintf]")
    {
            const auto x = 42;
            REQUIRE_FALSE(strprintf::fmt("%p", reinterpret_cast<const void*>(&x)).empty());
    }

    TEST_CASE("strprintf::fmt() formats nullptr", "[strprintf]")
    {
            REQUIRE_FALSE(strprintf::fmt("%p", nullptr) == "(null)");
    }
    */

    #[test]
    fn formats_double() {
        let x = 42.0f64;
        assert_eq!(fmt!("%f", x), "42.000000");
        assert_eq!(fmt!("%.3f", x), "42.000");

        let y = 42e138f64;
        assert_eq!(fmt!("%e", y), "4.200000e+139");
        assert_eq!(fmt!("%.3e", y), "4.200e+139");
    }

    /*
    XXX: can't be easily ported since Rust doesn't let us pass f32 to a variadic function like
    snprintf.
    TEST_CASE("strprintf::fmt() formats float", "[strprintf]")
    {
            const auto x = 42.0f;
            REQUIRE(strprintf::fmt("%f", x) == "42.000000");
            REQUIRE(strprintf::fmt("%.3f", x) == "42.000");

            const auto y = 42e3f;
            REQUIRE(strprintf::fmt("%e", y) == "4.200000e+04");
            REQUIRE(strprintf::fmt("%.3e", y) == "4.200e+04");
    }

    // TODO: write the same test for str
    // TODO: write the same test for &str
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

// Allowing this on a module level, because:
// 1. this module will only ever contain these constants;
// 2. the names are copied from C, and should be kept verbatim, capitalization and all.
#![allow(non_upper_case_globals)]

/// `printf` format specifiers for 64-bit platforms.

/// `printf`'s format conversion specifier to output an `i32` (equivalent to `PRIi32`).
pub const PRId32: &str = "d";

/// `printf`'s format conversion specifier to output an `i32` (equivalent to `PRId32`).
pub const PRIi32: &str = "i";

/// `printf`'s format conversion specifier to output an `u32`.
pub const PRIu32: &str = "u";

/// `printf`'s format conversion specifier to output an `i64` (equivalent to `PRIi64`).
pub const PRId64: &str = "ld";

/// `printf`'s format conversion specifier to output an `i64` (equivalent to `PRId64`).
pub const PRIi64: &str = "li";

/// `printf`'s format conversion specifier to output an `u64`.
pub const PRIu64: &str = "lu";

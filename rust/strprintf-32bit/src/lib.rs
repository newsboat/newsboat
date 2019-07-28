/// `printf` format specifiers for 32-bit platforms.
pub mod format_specifiers {
    // Allowing this on a module level, because:
    // 1. this module will only ever contain these constants;
    // 2. the names are copied from C, and should be kept verbatim, capitalization and all.
    #![allow(non_upper_case_globals)]

    /// `printf`'s format conversion specifier to output an `i32` (equivalent to `PRIi32`).
    pub const PRId32: &'static str = "d";

    /// `printf`'s format conversion specifier to output an `i32` (equivalent to `PRId32`).
    pub const PRIi32: &'static str = "i";

    /// `printf`'s format conversion specifier to output an `u32`.
    pub const PRIu32: &'static str = "u";

    /// `printf`'s format conversion specifier to output an `i64` (equivalent to `PRIi64`).
    pub const PRId64: &'static str = "lld";

    /// `printf`'s format conversion specifier to output an `i64` (equivalent to `PRId64`).
    pub const PRIi64: &'static str = "lli";

    /// `printf`'s format conversion specifier to output an `u64`.
    pub const PRIu64: &'static str = "llu";
}

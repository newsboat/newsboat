//! Traits required by `fmt!` macro.
//!
//! `fmt!` needs to convert values from Rust types like `u64` and `String` to C types like
//! `uint64_t` and `const char*`.
//!
//! For integer types, this happens automatically, e.g. `u64` can be passed directly into
//! `libc::snprintf`.
//!
//! Floating-point types are a bit more complicated: `f32` (C's `float`) has to be converted to
//! `f64` (C's `double`) because that's how variadic FFI works. `f64` can be passed over FFI
//! without any problems.
//!
//! Strings are the most complicated of all: Rust represents them as an array of bytes, whereas
//! C represents them as a chunk of memory terminated by a null byte. As a result, converting
//! a `String` into `const char*` requires an allocation to temporarily store a string in
//! a C-compatible fashion. `snprintf` doesn't understand Rust's borrowing, so we also need a Rust
//! object to hold that memory while `snprintf` works with a pointer to it.
//!
//! To support all of this, `fmt!` uses a two-stage process:
//! 1. convert a Rust type into an intermediate "holder" that owns a C-compatible representation of
//!    that value (`Printfable` trait);
//! 2. borrow the C-compatible representation of the value from the "holder" (`CReprHolder` trait).
//!
//! For example, a "holder" for a `String` is `std::ffi::CString`. It can be borrowed to call its
//! `to_ptr` method that returns `*const libc::c_char`.
//!
//! For numeric types, the holder is the same as the type itself, and the values are simply copied.
//! For example, `u64`'s "holder" is `u64`, which is then borrowed to get a `u64` again.

use std::ffi::CString;

/// Trait of all types that can be formatted with `fmt!` macro.
///
/// See module-level documentation for an explanation of this.
pub trait Printfable {
    /// A "holder" type for this type.
    type Holder: CReprHolder;

    /// Try to convert the value into "holder" type.
    ///
    /// This can fail for some types, e.g. `String` might fail to convert to `CString` if it
    /// contains null bytes.
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder>;
}

impl Printfable for i32 {
    type Holder = i32;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self)
    }
}

impl Printfable for u32 {
    type Holder = u32;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self)
    }
}

impl Printfable for i64 {
    type Holder = i64;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self)
    }
}

impl Printfable for u64 {
    type Holder = u64;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self)
    }
}

/// `f32` can't be passed to variadic functions like `snprintf`, so it's converted into `f64`
impl Printfable for f32 {
    type Holder = f64;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self as f64)
    }
}

impl Printfable for f64 {
    type Holder = f64;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self)
    }
}

impl Printfable for &str {
    type Holder = CString;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        CString::new(self).ok()
    }
}

impl Printfable for &String {
    type Holder = CString;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        CString::new(self.as_str()).ok()
    }
}

impl Printfable for String {
    type Holder = CString;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        CString::new(self).ok()
    }
}

impl<T> Printfable for *const T {
    type Holder = *const libc::c_void;
    fn into_c_repr_holder(self) -> Option<<Self as Printfable>::Holder> {
        Some(self as *const libc::c_void)
    }
}

/// Trait of all types that hold a C representation of a value.
///
/// See module-level documentation for an explanation of this.
pub trait CReprHolder {
    type Output;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output;
}

impl CReprHolder for i32 {
    type Output = i32;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for u32 {
    type Output = u32;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for i64 {
    type Output = i64;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for u64 {
    type Output = u64;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

// No need to implement CReprHolder for f32, as this trait is only invoked on results of
// `Printfable::into_c_repr_holder`, and that converts f32 to f64

impl CReprHolder for f64 {
    type Output = f64;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for CString {
    type Output = *const libc::c_char;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        self.as_ptr()
    }
}

impl CReprHolder for *const libc::c_void {
    type Output = *const libc::c_void;
    fn to_c_repr(&self) -> <Self as CReprHolder>::Output {
        *self
    }
}

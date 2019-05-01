// TODO: write some docs for this

extern crate libc;

use std::ffi::CString;

pub trait Printfable {
    type Holder: CReprHolder;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder>;
}

impl Printfable for i32 {
    type Holder = i32;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self)
    }
}

impl Printfable for u32 {
    type Holder = u32;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self)
    }
}

impl Printfable for i64 {
    type Holder = i64;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self)
    }
}

impl Printfable for u64 {
    type Holder = u64;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self)
    }
}

/// `f32` can't be passed to variadic functions like `snprintf`, so it's converted into `f64`
impl Printfable for f32 {
    type Holder = f64;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self as f64)
    }
}

impl Printfable for f64 {
    type Holder = f64;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        Some(*self)
    }
}

impl<'a> Printfable for &'a str {
    type Holder = CString;
    fn to_c_repr_holder(self: &Self) -> Option<<Self as Printfable>::Holder> {
        CString::new(*self).ok()
    }
}

pub trait CReprHolder {
    type Output;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output;
}

impl CReprHolder for i32 {
    type Output = i32;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for u32 {
    type Output = u32;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for i64 {
    type Output = i64;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for u64 {
    type Output = u64;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        *self
    }
}

// No need to implement CReprHolder for f32, as this trait is only invoked on results of
// `Printfable::to_c_repr_holder`, and that converts f32 to f64

impl CReprHolder for f64 {
    type Output = f64;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        *self
    }
}

impl CReprHolder for CString {
    type Output = *const libc::c_char;
    fn to_c_repr(self: &Self) -> <Self as CReprHolder>::Output {
        self.as_ptr()
    }
}

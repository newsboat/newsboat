#[macro_use]
extern crate strprintf;

extern crate backtrace;
extern crate dirs;
extern crate nom;
extern crate once_cell;
extern crate xdg;
#[cfg(test)]
#[macro_use]
extern crate proptest;
extern crate clap;
extern crate gettextrs;
extern crate libc;

// This module must be declared before the others because it exports a `log!` macro that everyone
// else uses.
#[macro_use]
pub mod logger;

pub mod human_panic;
pub mod utils;

pub mod cliargsparser;
pub mod configpaths;
pub mod fmtstrformatter;
pub mod history;

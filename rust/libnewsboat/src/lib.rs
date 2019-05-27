#[macro_use]
extern crate strprintf;

extern crate backtrace;
#[macro_use]
extern crate nom;
extern crate dirs;
extern crate once_cell;
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
pub mod config;
pub mod fmtstrformatter;

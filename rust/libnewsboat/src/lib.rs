// This lint is nitpicky, I don't think it's really important how the literals are written.
#![allow(clippy::unreadable_literal)]

// This module must be declared before the others because it exports a `log!` macro that everyone
// else uses.
#[macro_use]
pub mod logger;

pub mod human_panic;
pub mod utils;

pub mod charencoding;
pub mod cliargsparser;
pub mod configpaths;
pub mod filepath;
pub mod filterparser;
pub mod fmtstrformatter;
pub mod fslock;
pub mod history;
pub mod keycombination;
pub mod keymap;
pub mod links;
pub mod matchable;
pub mod matcher;
pub mod matchererror;
pub mod scopemeasure;
pub mod stflrichtext;

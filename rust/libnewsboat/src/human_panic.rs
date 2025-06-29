//! Provides a panic hook with a user-friendly message.
//!
//! When Rust panics, the default message is pretty terse:
//!
//! ```text
//! thread '<unnamed>' panicked at 'Can't obtain a global logger', rust/libnewsboat/src/logger.rs:283:5
//! note: Run with `RUST_BACKTRACE=1` for a backtrace.
//! ```
//!
//! An end user probably won't know what to do with this, and even if they *did* re-run with
//! a backtrace enabled, the crash might not occur again. To fix that, this module provides a hook
//! that will print a more user-friendly message:
//!
//! ```text
//! Newsboat crashed; sorry about that. Please submit a report (below)
//! so we can investigate:
//!
//! - online: https://github.com/newsboat/newsboat/issues/new
//!
//!   You will need a GitHub account.
//!
//! - via email: newsboat@googlegroups.com
//!
//! We might have some follow-up questions, so please check your email
//! periodically. Thank you!
//!
//! The crash report:
//!
//! --->8----->8----->8----->8----->8----->8----->8----->8----->8----->8----->8---
//!
//! Newsboat version: 2.14.0
//! Couldn't determine the crash cause.
//! Message: Can't obtain the global logger
//! Crash location: rust/libnewsboat/src/logger.rs:283
//! stack backtrace:
//!    0:     0x559d490f8245 - backtrace::backtrace::libunwind::trace::ha74bb38523d900bf
//!                         at /home/minoru/.cargo/registry/src/github.com-1ecc6299db9ec823/backtrace-0.3.9/src/backtrace/libunwind.rs:53
//!                          - backtrace::backtrace::trace::h22649ed4442cef5c
//!                         at /home/minoru/.cargo/registry/src/github.com-1ecc6299db9ec823/backtrace-0.3.9/src/backtrace/mod.rs:42
//!    1:     0x559d490f2e3e - backtrace::capture::Backtrace::new_unresolved::h4fedf8cbb0cc5848
//!                         at /home/minoru/.cargo/registry/src/github.com-1ecc6299db9ec823/backtrace-0.3.9/src/capture.rs:88
//!    2:     0x559d490f2d9c - backtrace::capture::Backtrace::new::hd207511a8a11ca29
//!                         at /home/minoru/.cargo/registry/src/github.com-1ecc6299db9ec823/backtrace-0.3.9/src/capture.rs:63
//!    3:     0x559d48dcb00a - libnewsboat::human_panic::print_panic_msg::h212b04a7a3a5da8a
//!                         at rust/libnewsboat/src/human_panic.rs:59
//!    4:     0x559d48dca56c - libnewsboat::human_panic::setup::{{closure}}::hf29969d76d6a13df
//!                         at rust/libnewsboat/src/human_panic.rs:23
//!    5:     0x559d4911c593 - std::panicking::rust_panic_with_hook::hafa9461392bef12a
//!    6:     0x559d490c72bf - std::panicking::begin_panic::h76310ac575368f0c
//!                         at libstd/panicking.rs:411
//!    7:     0x559d48dcde7a - libnewsboat::logger::get_instance::h602edda973838b01
//!                         at rust/libnewsboat/src/logger.rs:283
//!    8:     0x559d48dc24f8 - rs_get_loglevel
//!                         at rust/libnewsboat-ffi/src/lib.rs:240
//!    9:     0x559d48a900a1 - _ZN8newsboat6Logger3logIJPcEEEvNS_5LevelERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpT_
//!                         at include/logger.h:29
//!   10:     0x559d48b0d2e4 - _ZN8newsboat10Controller3runERKNS_13CliArgsParserE
//!                         at src/controller.cpp:143
//!   11:     0x559d48a71b06 - main
//!                         at /home/minoru/src/newsboat/newsboat.cpp:188
//!   12:     0x7f251efc2b16 - __libc_start_main
//!   13:     0x559d48a6d399 - _start
//!   14:                0x0 - <unknown>
//!
//! --->8----->8----->8----->8----->8----->8----->8----->8----->8----->8----->8---
//! ```
//!
//! All you (the programmer) need to do is run this module's `setup()` somewhere towards the
//! beginning of the program.
use backtrace::Backtrace;
use std::io::{self, BufWriter, Write, stderr};
use std::panic;

use std::panic::PanicHookInfo;

/// Sets up a panic hook with a user-friendly message.
///
/// See module description for details.
pub fn setup() {
    if ::std::env::var("RUST_BACKTRACE").is_err() {
        panic::set_hook(Box::new(move |panic_info: &PanicHookInfo| {
            print_panic_msg(panic_info).expect("An error occurred while preparing a crash report");
        }));
    }
}

fn print_panic_msg(panic_info: &PanicHookInfo) -> io::Result<()> {
    // Locking the handle to make sure all the messages are printed out in one chunk.
    let stderr = stderr();
    let handle = stderr.lock();
    let mut stderr = BufWriter::new(handle);

    writeln!(
        &mut stderr,
        "Newsboat crashed; sorry about that. Please submit a report (below)\n\
         so we can investigate:\n\
         \n\
         - online: https://github.com/newsboat/newsboat/issues/new\n\
         \n  \
         You will need a GitHub account.\n\
         \n\
         - via email: newsboat@googlegroups.com\n\
         \n\
         We might have some follow-up questions, so please check your email\n\
         periodically. Thank you!\n\
         \n\
         The crash report:\n\
         \n\
         --->8----->8----->8----->8----->8----->8----->8----->8----->8----->8----->8---\n"
    )?;

    writeln!(
        &mut stderr,
        "Newsboat version: {}",
        env!("CARGO_PKG_VERSION")
    )?;
    writeln!(&mut stderr, "{}", get_error_message(panic_info))?;
    writeln!(&mut stderr, "{}", get_location(panic_info))?;

    writeln!(&mut stderr, "{:#?}", Backtrace::new())?;

    writeln!(
        &mut stderr,
        "\n--->8----->8----->8----->8----->8----->8----->8----->8----->8----->8----->8---"
    )?;

    Ok(())
}

fn get_error_message(panic_info: &PanicHookInfo) -> String {
    let payload = panic_info.payload().downcast_ref::<&str>();
    if let Some(payload) = payload {
        format!("Message: {}", &payload)
    } else {
        String::from("No error message")
    }
}

fn get_location(panic_info: &PanicHookInfo) -> String {
    match panic_info.location() {
        Some(location) => format!("Crash location: {}:{}", location.file(), location.line()),
        None => String::from("Crash location unknown"),
    }
}

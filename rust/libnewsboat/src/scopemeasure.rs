//! Measures time spent in a given scope, and writes it to the log.

use std::time::Instant;

use crate::logger::{self, Level};

/// Measures time spent in an enclosing scope, and writes it to the log.
///
/// Upon construction, this struct remembers current (monotonic) time. Before being dropped, it
/// will write a debug message to the log mentioning: 1) the name of the enclosing scope (as
/// provided to the constructor); 2) the time that elapsed between constructing and dropping the
/// object.
///
/// Calling `stopover()` will write a debug message to the log mentioning: 1) the name of the
/// enclosing scope; 2) the name of the stopover; 3) the time that elapsed between constructing the
/// object and calling `stopover()`.
pub struct ScopeMeasure {
    start_time: Instant,
    scope_name: String,
}

impl ScopeMeasure {
    /// Construct an object that will measure time spent in the scope named `scope_name`.
    pub fn new(scope_name: String) -> ScopeMeasure {
        ScopeMeasure {
            start_time: Instant::now(),
            scope_name,
        }
    }

    /// Write a message to the log mentioning the scope name, `stopover_name`, and the time elapsed
    /// since the object was constructed.
    pub fn stopover(&self, stopover_name: &str) {
        log!(
            Level::Debug,
            &format!(
                "ScopeMeasure: function `{}' (stop over `{stopover_name}') took {:.6} s so far",
                self.scope_name,
                self.start_time.elapsed().as_secs_f64()
            )
        );
    }
}

impl Drop for ScopeMeasure {
    fn drop(&mut self) {
        log!(
            Level::Debug,
            &format!(
                "ScopeMeasure: function `{}' took {:.6} s",
                self.scope_name,
                self.start_time.elapsed().as_secs_f64()
            )
        );
    }
}

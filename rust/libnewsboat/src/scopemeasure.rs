//! Measures time spent in a given scope, and writes it to the log.

use std::time::Instant;

use crate::{
    log,
    logger::{self, Level},
};

/// Measures time spent in an enclosing scope, and writes it to the log.
///
/// Upon construction, this struct remembers current (monotonic) time. Before being dropped, it
/// will write a message to the log mentioning: 1) the name of the enclosing scope (as provided to
/// the constructor); 2) the time that elapsed between constructing and dropping the object.
///
/// Calling `stopover()` will write a message to the log mentioning: 1) the name of the enclosing
/// scope; 2) the name of the stopover; 3) the time that elapsed between constructing the object
/// and calling `stopover()`.
pub struct ScopeMeasure {
    start_time: Instant,
    scope_name: String,
    log_level: Level,
}

impl ScopeMeasure {
    /// Construct an object that will measure time spent in the scope named `scope_name`. Log
    /// messages will be written at `log_level`.
    pub fn new(scope_name: String, log_level: Level) -> ScopeMeasure {
        ScopeMeasure {
            start_time: Instant::now(),
            scope_name,
            log_level,
        }
    }

    /// Write a message to the log mentioning the scope name, `stopover_name`, and the time elapsed
    /// since the object was constructed.
    pub fn stopover(&self, stopover_name: &str) {
        log!(
            self.log_level,
            &format!(
                "ScopeMeasure: function `{}' (stop over `{}') took {:.6} s so far",
                self.scope_name,
                stopover_name,
                self.start_time.elapsed().as_secs_f64()
            )
        );
    }
}

impl Drop for ScopeMeasure {
    fn drop(&mut self) {
        log!(
            self.log_level,
            &format!(
                "ScopeMeasure: function `{}' took {:.6} s",
                self.scope_name,
                self.start_time.elapsed().as_secs_f64()
            )
        );
    }
}

//! Keeps a record of what the program did.

extern crate chrono;

use self::chrono::offset::Local;
use std::fmt;
use std::fs::{File, OpenOptions};
use std::io::Write;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
/// "Importance levels" for log messages.
///
/// Each message is assigned a level. set_loglevel instructs Logger to only store messages at and
/// above a certain importance level, making the log file shorter and easier to understand.
pub enum Level {
    /// Not important at all, don't log.
    ///
    /// This level is purely for Logger's internal use, and shouldn't be passed to log(). That
    /// isn't enforced in any way, though.
    None,

    /// An error that can potentially be resolved by the user.
    ///
    /// This level should be used for configuration errors and the like.
    UserError,

    /// An error that prevents program from working at all.
    ///
    /// E.g. failure to open a cache file is a critical error, because we can't do anything without
    /// cache.
    Critical,

    /// An error that prevents some function from working.
    ///
    /// E.g. failure to find an item in the cache prevents Newsboat from displaying it, but other
    /// functions are still working, so overall it's not critical.
    Error,

    /// Warning about a potential problem.
    Warn,

    /// Useful information that can roughly explain what's going on.
    ///
    /// This level should be used with rought descriptions of stages that the program is going
    /// through. Fine details (indices, array sizes etc.) should be logged at Level::Debug instead.
    Info,

    /// May be useful to programmers when debugging.
    ///
    /// This level should be used for the most minutae details about program execution, like byte
    /// offsets, indices, sizes of internal data structures and so forth.
    Debug,
}

impl fmt::Display for Level {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Level::None => write!(f, "NONE"),
            Level::UserError => write!(f, "USERERROR"),
            Level::Critical => write!(f, "CRITICAL"),
            Level::Error => write!(f, "ERROR"),
            Level::Warn => write!(f, "WARNING"),
            Level::Info => write!(f, "INFO"),
            Level::Debug => write!(f, "DEBUG"),
        }
    }
}

/// Keeps a record of what the program did.
///
/// Each Logger object can write up to two logs.
///
/// One, general log, is created after the call to set_logfile(). set_loglevel() sets the logging
/// level, and from then on, any message at or above that level is written to the logfile.
///
/// Another, user-specific log, is created after the call to set_errorlogfile(). Only
/// Level::UserLevel messages are written to that one.
///
/// Each message in the log is time-stamped, and marked with its importance level.
///
/// This is meant to be a long-lived, shared object that exists for the duration of the program.
/// Users would call its `log` method to add messages to the log file, like this:
///
/// ```
/// // Create and configure the logger
/// let logger = Logger::new();
/// logger.set_logfile("/path/to/my/log.txt");
///
/// // Only log critical and configuration errors
/// logger.set_loglevel(Level::Critical);
///
/// // -- snip --
///
/// logger.log(Level::UserError, "Please specify a remote API to use");
///
/// // This one won't be logged because its importance level is too low
/// logger.log(Level::Debug, format!("feeds.len() == {}", 42));
/// ```
pub struct Logger {
    /// The file to which all messages at and above `loglevel` will be written.
    logfile: Option<File>,

    /// The file to which all Level::UserError messages will be written if `loglevel` is not
    /// Level::None.
    errorlogfile: Option<File>,

    /// Maximum "importance level" of the messages that will be written to the log.
    loglevel: Level,
}

impl Logger {
    /// Constructs an object that doesn't log anything.
    ///
    /// To make that Logger useful, you need to call set_logfile() and set_loglevel().
    pub fn new() -> Logger {
        Logger {
            logfile: None,
            errorlogfile: None,
            loglevel: Level::None,
        }
    }

    /// Specifies the file to which the messages will be written.
    ///
    /// The file will be created if it doesn't exist yet. It will be opened in the append mode, so
    /// its previous content will stay unchanged.
    ///
    /// Calling this closes previously opened logfile, if any.
    ///
    /// # Errors
    ///
    /// This can't fail, but if the file couldn't be created or opened, an error message will be
    /// printed to stderr.
    pub fn set_logfile(&mut self, filename: &str) {
        let file = OpenOptions::new()
            .create(true)
            .append(true)
            .open(filename);

        match file {
            Ok(file) => self.logfile = Some(file),
            Err(error) => eprintln!("Couldn't open `{}' as a logfile: {}", filename, error),
        }
    }

    /// Specifies the file to which all Level::UserError messages will be written.
    ///
    /// The messages will only be written if Logger's loglevel is not Level::None.
    ///
    /// The file will be created if it doesn't exist yet. It will be opened in the append mode, so
    /// its previous contents will stay unchanged.
    ///
    /// Calling this closes previously opened error-logfile, if any.
    ///
    /// # Errors
    ///
    /// This can't fail, but if the file couldn't be created or opened, an error message will be
    /// printed to stderr.
    pub fn set_errorlogfile(&mut self, filename: &str) {
        let file = OpenOptions::new()
            .create(true)
            .append(true)
            .open(filename);

        match file {
            Ok(file) => self.errorlogfile = Some(file),
            Err(error) => eprintln!("Couldn't open `{}' as a errorlogfile: {}", filename, error),
        }
    }

    /// Writes a message to a log.
    ///
    /// If `level` is lower than the logger's current level, the message won't be written. For
    /// example, if Logger's level is set to Level::Critical, then Level::Error won't be written.
    ///
    /// Level::UserError messages are written to two logs: the general one, and error-logfile. See
    /// Logger description for details.
    ///
    /// # Errors
    ///
    /// If the message couldn't be written for whatever reason, this function ignores the failure.
    /// Were you to check the return value of every log() call, you'd just stop writing logs.
    pub fn log(&mut self, level: Level, message: &str) {
        if level > self.loglevel {
            return;
        }

        let timestamp = Local::now().format("%Y-%m-%d %H:%M:%S");

        if let Some(ref mut logfile) = self.logfile {
            let line = format!("[{}] {}: {}\n", timestamp, level, message);

            // Ignoring the error since checking every log() call will be too bothersome.
            let _ = logfile.write_all(line.as_bytes());
        }

        if level == Level::UserError {
            if let Some(ref mut errorlogfile) = self.errorlogfile {
                let line = format!("[{}] {}\n", timestamp, message);

                // Ignoring the error since checking every log() call will be too bothersome.
                let _ = errorlogfile.write_all(line.as_bytes());
            }
        }
    }

    /// Sets maximum "importance level" of the messages that will be written to the log.
    ///
    /// For example, after the call to set_loglevel(Level::Error), only UserError, Critical, and
    /// Error messages will be written.
    ///
    /// If new level is Level::None, no logs will be written from now on. Otherwise, at least
    /// error-logfile will be written.
    ///
    /// Calling this doesn't close already opened logs.
    pub fn set_loglevel(&mut self, level: Level) {
        self.loglevel = level;
    }
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

    use super::*;
    
    use self::tempfile::TempDir;
    use self::chrono::{TimeZone, Duration};
    use std::io::{self, BufReader, BufRead};
    use std::path;

    fn setup_logger() -> io::Result<(TempDir, path::PathBuf, path::PathBuf, Logger)> {
        let tmp = TempDir::new()?;
        let logfile = {
            let mut logfile = tmp.path().to_owned();
            logfile.push("example.log");
            logfile
        };
        let error_logfile = {
            let mut error_logfile = tmp.path().to_owned();
            error_logfile.push("error-example.log");
            error_logfile
        };
        assert!(!error_logfile.exists());

        let mut logger = Logger::new();
        logger.set_logfile(logfile.to_str().unwrap());
        logger.set_errorlogfile(error_logfile.to_str().unwrap());

        Ok((tmp, logfile, error_logfile, logger))
    }

    fn parse_log_line(line: &str) -> Option<(&str, &str, &str)> {
        if !line.starts_with("[") {
            return None;
        }

        let timestamp_end = line.find(']').expect("Failed to find the end of the timestamp");
        // Timestamp starts with "[", we skip it by starting at 1 rather than 0.
        let timestamp = &line[1..timestamp_end];

        // "]" is an ASCII character, so it occupies one byte in UTF-8. As a result, we
        // can simply add one to get the next character's byte offset. And since the
        // next char should be a space, which is ASCII as well, we can just add 2.
        //
        // If timestamp is followed by something non-ASCII, indexing into &str will
        // panic and fail the test, which is good.
        if &line[timestamp_end+1..timestamp_end+2] != " " {
            return None;
        }

        let level_end =
            line[timestamp_end+2..]
            .find(':')
            .expect("Failed to find the end of the loglevel");
        let level = &line[timestamp_end+2..timestamp_end+2+level_end];

        // Message starts after ": " that follows the level, hence +2.
        let message = &line[timestamp_end+2+level_end+2..];

        Some((timestamp, level, message))
    }

    fn parse_errorlog_line(line: &str) -> Option<(&str, &str)> {
        if !line.starts_with("[") {
            return None;
        }

        let timestamp_end = line.find(']').expect("Failed to find the end of the timestamp");
        // Timestamp starts with "[", we skip it by starting at 1 rather than 0.
        let timestamp = &line[1..timestamp_end];

        // Message starts after " " that follows the timestamp, hence +2.
        let message = &line[timestamp_end+2..];

        Some((timestamp, message))
    }

    fn log_contains_n_lines(logfile: &path::Path, n: usize)  -> io::Result<()> {
        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        assert_eq!(reader.lines().count(), n);
        Ok(())
    }

    struct LogLinesCounter {
        messages: Vec<(Level, String)>,
        levels: Vec<Level>,
        expected_log_lines: Option<usize>,
        expected_errorlog_lines: Option<usize>,
    }

    impl LogLinesCounter {
        pub fn new() -> Self {
            LogLinesCounter {
                messages: vec![],
                levels: vec![],
                expected_log_lines: None,
                expected_errorlog_lines: None,
            }
        }

        pub fn with_messages(mut self, msgs: Vec<(Level, String)>) -> Self {
            self.messages = msgs;
            self
        }

        pub fn at_levels(mut self, levels: Vec<Level>) -> Self {
            self.levels = levels;
            self
        }

        pub fn expected_log_lines_count(mut self, n: usize) -> Self {
            self.expected_log_lines = Some(n);
            self
        }

        pub fn expected_errorlog_lines_count(mut self, n: usize) -> Self {
            self.expected_errorlog_lines = Some(n);
            self
        }

        pub fn test(&self) -> io::Result<()> {
            if self.expected_log_lines.is_none() && self.expected_errorlog_lines.is_none() {
                panic!("You failed to specify any assertions on LogLinesCounter");
            }

            for level in &self.levels {
                let (_tmp, logfile, error_logfile, mut logger) = setup_logger()?;
                logger.set_loglevel(*level);

                for (level, msg) in &self.messages {
                    logger.log(*level, &msg);
                }

                drop(logger);

                if let Some(count) = self.expected_log_lines {
                    log_contains_n_lines(&logfile, count)?;
                }

                if let Some(count) = self.expected_errorlog_lines {
                    log_contains_n_lines(&error_logfile, count)?;
                }
            };

            Ok(())
        }
    }

    #[test]
    fn t_set_logfile_creates_a_file() -> io::Result<()> {
        let (_tmp, logfile, _error_logfile, _logger) = setup_logger()?;

        assert!(logfile.exists());

        Ok(())
    }

    #[test]
    fn t_log_writes_message_to_the_file() -> io::Result<()> {
        let (_tmp, logfile, _error_logfile, mut logger) = setup_logger()?;

        let messages = vec![
            "Hello, world!",
            "I'm doing fine, how are you?",
            "Time to wrap up, see ya!",
        ];

        let start_time = Local::now();
        for msg in &messages {
            logger.log(Level::Debug, msg);
        }
        let finish_time = Local::now();

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(messages) {
            match line {
                Ok(line) => {
                    let (timestamp_str, _level, message) =
                        parse_log_line(&line)
                        .expect("Failed to split the log line into parts");

                    let timestamp =
                        Local::now()
                        .timezone()
                        .datetime_from_str(timestamp_str, "%Y-%m-%d %H:%M:%S")
                        .expect("Failed to parse the timestamp from the log file");
                    // `start_time` and `end_time` may have millisecond precision or better,
                    // whereas `timestamp` is limited to seconds. Therefore, we account for
                    // a situation where `start_time` is slightly bigger than `timestamp`.
                    assert!(timestamp - start_time > Duration::seconds(-1));
                    assert!(finish_time >= timestamp);

                    assert_eq!(message, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }

    #[test]
    fn t_different_loglevels_have_different_names() -> io::Result<()> {
        let (_tmp, logfile, _error_logfile, mut logger) = setup_logger()?;

        let levels = vec![
            (Level::UserError, "USERERROR"),
            (Level::Critical, "CRITICAL"),
            (Level::Error, "ERROR"),
            (Level::Warn, "WARNING"),
            (Level::Info, "INFO"),
            (Level::Debug, "DEBUG"),
        ];

        let msg = "Some test message";

        for (level, _level_str) in &levels {
            logger.log(*level, msg);
        }

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(levels.iter().map(|(_, l)| l)) {
            match line {
                Ok(line) => {
                    let (_timestamp_str, level, _message) =
                        parse_log_line(&line)
                        .expect("Failed to split the log line into parts");

                    assert_eq!(&level, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }

    #[test]
    fn t_if_curlevel_is_none_nothing_is_logged() -> io::Result<()> {
        let (_tmp, logfile, _error_logfile, mut logger) = setup_logger()?;

        logger.set_loglevel(Level::None);

        let levels = vec![
            (Level::UserError, "USERERROR"),
            (Level::Critical, "CRITICAL"),
            (Level::Error, "ERROR"),
            (Level::Warn, "WARNING"),
            (Level::Info, "INFO"),
            (Level::Debug, "DEBUG"),
        ];

        let msg = "Some test message";

        for (level, _level_str) in &levels {
            logger.log(*level, msg);
        }

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        log_contains_n_lines(&logfile, 0)?;

        Ok(())
    }

    #[test]
    fn t_user_errors_are_logged_at_all_curlevels_beside_none() -> io::Result<()> {
        let message = (Level::UserError, "hello".to_string());

        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(vec![Level::None])
            .expected_log_lines_count(0)
            .test()?;

        let levels = vec![
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(levels)
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_critial_msgs_are_logged_at_curlevels_starting_with_critical() -> io::Result<()> {
        let message = (Level::Critical, "hello".to_string());

        let nolog_levels = vec![
            Level::None,
            Level::UserError,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(nolog_levels)
            .expected_log_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(log_levels)
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_error_msgs_are_logged_at_curlevels_starting_with_error() -> io::Result<()> {
        let message = (Level::Error, "hello".to_string());

        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(nolog_levels)
            .expected_log_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(log_levels)
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_warning_msgs_are_logged_at_curlevels_starting_with_warning() -> io::Result<()> {
        let message = (Level::Warn, "hello".to_string());

        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(nolog_levels)
            .expected_log_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(log_levels)
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_info_msgs_are_logged_at_curlevels_starting_with_info() -> io::Result<()> {
        let message = (Level::Info, "hello".to_string());

        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(nolog_levels)
            .expected_log_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(log_levels)
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_debug_msgs_are_logged_only_at_curlevel_debug() -> io::Result<()> {
        let message = (Level::Debug, "hello".to_string());

        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(nolog_levels)
            .expected_log_lines_count(0)
            .test()?;

        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(vec![Level::Debug])
            .expected_log_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_set_errorlogfile_creates_a_file() -> io::Result<()> {
        let (_tmp, _logfile, error_logfile, _logger) = setup_logger()?;
        assert!(error_logfile.exists());

        Ok(())
    }

    #[test]
    fn t_writes_to_errorlog_at_usererror_and_above() -> io::Result<()> {
        let message = (Level::UserError, "hello".to_string());

        LogLinesCounter::new()
            .with_messages(vec![message.clone()])
            .at_levels(vec![Level::None])
            .expected_errorlog_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(vec![message])
            .at_levels(log_levels)
            .expected_errorlog_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_only_usererrors_are_written_to_errorlog() -> io::Result<()> {
        let messages = vec![
            (Level::UserError, "hello".to_string()),
            (Level::Critical, "this shouldn't be written to error-log".to_string()),
        ];

        let nolog_levels = vec![
            Level::None,
        ];
        LogLinesCounter::new()
            .with_messages(messages.clone())
            .at_levels(nolog_levels)
            .expected_errorlog_lines_count(0)
            .test()?;

        let log_levels = vec![
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];
        LogLinesCounter::new()
            .with_messages(messages)
            .at_levels(log_levels)
            .expected_errorlog_lines_count(1)
            .test()?;

        Ok(())
    }

    #[test]
    fn t_log_writes_message_to_the_errorlogfile() -> io::Result<()> {
        let (_tmp, _logfile, error_logfile, mut logger) = setup_logger()?;

        let messages = vec![
            "Hello, world!",
            "I'm doing fine, how are you?",
            "Time to wrap up, see ya!",
        ];

        let start_time = Local::now();
        for msg in &messages {
            logger.log(Level::UserError, msg);
        }
        let finish_time = Local::now();

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(error_logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(messages) {
            match line {
                Ok(line) => {
                    let (timestamp_str, message) =
                        parse_errorlog_line(&line)
                        .expect("Failed to split the error log line into parts");

                    let timestamp =
                        Local::now()
                        .timezone()
                        .datetime_from_str(timestamp_str, "%Y-%m-%d %H:%M:%S")
                        .expect("Failed to parse the timestamp from the error log file");
                    // `start_time` and `end_time` may have millisecond precision or better,
                    // whereas `timestamp` is limited to seconds. Therefore, we account for
                    // a situation where `start_time` is slightly bigger than `timestamp`.
                    assert!(timestamp - start_time > Duration::seconds(-1));
                    assert!(finish_time >= timestamp);

                    assert_eq!(message, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }
}

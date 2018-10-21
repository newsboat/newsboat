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
    /// This level should be used with rough descriptions of stages that the program is going
    /// through. Fine details (indices, array sizes etc.) should be logged at Level::Debug instead.
    Info,

    /// May be useful to programmers when debugging.
    ///
    /// This level should be used for the most minutiae details about program execution, like byte
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
    logfile: Option<File>,

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
            loglevel: Level::None,
        }
    }

    /// Specifies the file to which the messages will be written.
    ///
    /// The file will be created if it doesn't exist yet. It will be opened in the append mode, so
    /// its previous content will stay unchanged.
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

    /// Writes a message to a log.
    ///
    /// If `level` is lower than the logger's current level, the message won't be written. For
    /// example, if Logger's level is set to Level::Critical, then Level::Error won't be written.
    ///
    /// # Errors
    ///
    /// If the message couldn't be written for whatever reason, this function ignores the failure.
    /// Were you to check the return value of every log() call, you'd just stop writing logs.
    pub fn log(&mut self, level: Level, message: &str) {
        if level > self.loglevel {
            return;
        }

        if let Some(ref mut logfile) = self.logfile {
            let timestamp = Local::now().format("%Y-%m-%d %H:%M:%S");
            let line = format!("[{}] {}: {}\n", timestamp, level, message);

            // Ignoring the error since checking every log() call will be too bothersome.
            let _ = logfile.write_all(line.as_bytes());
        }
    }

    /// Sets maximum "importance level" of the messages that will be written to the log.
    ///
    /// For example, after the call to set_loglevel(Level::Error), only UserError, Critical, and
    /// Error messages will be written.
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

    fn setup_logger() -> io::Result<(TempDir, path::PathBuf, Logger)> {
        let tmp = TempDir::new()?;
        let logfile = {
            let mut logfile = tmp.path().to_owned();
            logfile.push("example.log");
            logfile
        };
        assert!(!logfile.exists());

        let mut logger = Logger::new();
        logger.set_logfile(logfile.to_str().unwrap());

        Ok((tmp, logfile, logger))
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

    fn log_contains_n_lines(logfile: &path::Path, n: usize)  -> io::Result<()> {
        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        assert_eq!(reader.lines().count(), n);
        Ok(())
    }

    #[test]
    fn t_set_logfile() -> io::Result<()> {
        let (_tmp, logfile, _logger) = setup_logger()?;

        assert!(logfile.exists());

        Ok(())
    }

    #[test]
    fn t_log_writes_message_to_the_file() -> io::Result<()> {
        let (_tmp, logfile, mut logger) = setup_logger()?;

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
        let (_tmp, logfile, mut logger) = setup_logger()?;

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
        let (_tmp, logfile, mut logger) = setup_logger()?;

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
        {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(Level::None);

            logger.log(Level::UserError, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let levels = vec![
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];

        for level in levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::UserError, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }

    #[test]
    fn t_critial_msgs_are_logged_at_curlevels_starting_with_critical() -> io::Result<()> {
        let nolog_levels = vec![
            Level::None,
            Level::UserError,
        ];

        for level in nolog_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Critical, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let log_levels = vec![
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];

        for level in log_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Critical, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }

    #[test]
    fn t_error_msgs_are_logged_at_curlevels_starting_with_error() -> io::Result<()> {
        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
        ];

        for level in nolog_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Error, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let log_levels = vec![
            Level::Error,
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];

        for level in log_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Error, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }

    #[test]
    fn t_warning_msgs_are_logged_at_curlevels_starting_with_warning() -> io::Result<()> {
        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
        ];

        for level in nolog_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Warn, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let log_levels = vec![
            Level::Warn,
            Level::Info,
            Level::Debug,
        ];

        for level in log_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Warn, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }

    #[test]
    fn t_info_msgs_are_logged_at_curlevels_starting_with_info() -> io::Result<()> {
        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
        ];

        for level in nolog_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Info, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let log_levels = vec![
            Level::Info,
            Level::Debug,
        ];

        for level in log_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Info, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }

    #[test]
    fn t_debug_msgs_are_logged_only_at_curlevel_debug() -> io::Result<()> {
        let nolog_levels = vec![
            Level::None,
            Level::UserError,
            Level::Critical,
            Level::Error,
            Level::Warn,
            Level::Info,
        ];

        for level in nolog_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Debug, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 0)?;
        }

        let log_levels = vec![
            Level::Debug,
        ];

        for level in log_levels {
            let (_tmp, logfile, mut logger) = setup_logger()?;
            logger.set_loglevel(level);

            logger.log(Level::Debug, "hello");

            drop(logger);

            log_contains_n_lines(&logfile, 1)?;
        };

        Ok(())
    }
}

use libnewsboat::{
    log,
    logger::{self, Level, get_instance},
};
use std::fs::File;
use std::io::{BufRead, BufReader, Result};
use std::path;
use tempfile::TempDir;

fn log_contains_n_lines(logfile: &path::Path, n: usize) -> Result<()> {
    let file = File::open(logfile)?;
    let reader = BufReader::new(file);
    assert_eq!(reader.lines().count(), n);
    Ok(())
}

#[test]
fn t_log_macro_writes_messages_to_the_log() {
    let tmp = TempDir::new().unwrap();
    let logfile = {
        let mut logfile = tmp.path().to_owned();
        logfile.push("example.log");
        logfile
    };

    get_instance().set_logfile(logfile.to_str().unwrap());
    get_instance().set_loglevel(Level::Debug);

    log!(Level::Debug, "Greetings");
    log!(Level::UserError, "Please set some settings");
    log!(Level::Error, "Answer invalid: {}", 41);

    log_contains_n_lines(&logfile, 3).unwrap();
}

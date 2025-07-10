use libNewsboat::{
    logger::{self, Level},
    scopemeasure::ScopeMeasure,
};
use std::fs::File;
use std::io::{BufRead, BufReader, Result};
use std::path::Path;
use tempfile::TempDir;

fn file_lines_count(logfile: &Path) -> Result<usize> {
    let file = File::open(logfile)?;
    let reader = BufReader::new(file);
    Ok(reader.lines().count())
}

#[test]
fn droppping_a_scopemeasure_writes_a_line_to_the_log() {
    let tmp = TempDir::new().unwrap();
    let logfile = {
        let mut logfile = tmp.path().to_owned();
        logfile.push("example.log");
        logfile
    };

    {
        logger::get_instance().set_logfile(logfile.to_str().unwrap());
        logger::get_instance().set_loglevel(Level::Debug);
        let _sm = ScopeMeasure::new(String::from("test"));
    }

    assert_eq!(file_lines_count(&logfile).unwrap(), 1);
}

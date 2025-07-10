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
fn stopover_adds_an_extra_line_to_the_log_upon_each_call() {
    for calls in &[1, 2, 5] {
        let tmp = TempDir::new().unwrap();
        let logfile = {
            let mut logfile = tmp.path().to_owned();
            logfile.push("example.log");
            logfile
        };

        {
            logger::get_instance().set_logfile(logfile.to_str().unwrap());
            logger::get_instance().set_loglevel(Level::Debug);
            let sm = ScopeMeasure::new(String::from("test"));

            for i in 0..*calls {
                sm.stopover(&format!("stopover No.{i}"));
            }
        }

        // One line for each call to stopover(), plus one more for the call to drop()
        assert_eq!(file_lines_count(&logfile).unwrap(), calls + 1usize);
    }
}

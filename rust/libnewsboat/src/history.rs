use std::fs::OpenOptions;
use std::io::{self, BufRead, BufReader, BufWriter, Write};
use std::path::Path;

#[derive(Default)]
pub struct History {
    idx: usize,
    lines: Vec<String>,
}

impl History {
    pub fn new() -> History {
        History {
            idx: 0,
            lines: Vec::new(),
        }
    }
    pub fn add_line(&mut self, line: String) {
        // When a line is added, we need to do so and reset the index so that the next
        // previous_line()/next_line() operations start from the beginning again.
        if !line.is_empty() {
            self.lines.insert(0, line)
        }
        self.idx = 0;
    }
    pub fn next_line(&mut self) -> String {
        match self.idx {
            0 => String::new(),
            _ => {
                self.idx -= 1;
                self.lines[self.idx].clone()
            }
        }
    }
    pub fn previous_line(&mut self) -> String {
        match self.lines.len() {
            0 => String::new(),
            len if self.idx < len => {
                let line = self.lines[self.idx].clone();
                self.idx += 1;
                line
            }
            _ => self.lines[self.idx - 1].clone(),
        }
    }
    pub fn load_from_file<P: AsRef<Path>>(&mut self, path: P) -> io::Result<()> {
        let file = OpenOptions::new().read(true).open(path)?;
        let lines: Vec<String> = BufReader::new(file)
            .lines()
            .collect::<Result<Vec<_>, _>>()?;
        lines.into_iter().for_each(|ln| self.add_line(ln));
        Ok(())
    }
    pub fn save_to_file<P: AsRef<Path>>(&self, path: P, limit: usize) -> io::Result<()> {
        if limit == 0 || self.lines.is_empty() {
            return Ok(());
        }

        let file = OpenOptions::new()
            .write(true)
            .create(true)
            .truncate(true)
            .open(path)?;
        let mut f = BufWriter::new(file);

        self.lines
            .iter()
            .rev()
            .take(limit)
            .map(|ln| writeln!(f, "{}", ln).map(|_| ()))
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::TempDir;

    #[test]
    fn t_empty_history_returns_nothing() {
        let mut h = History::new();
        assert_eq!(h.previous_line(), "");
        assert_eq!(h.previous_line(), "");
        assert_eq!(h.next_line(), "");
        assert_eq!(h.next_line(), "");
    }

    #[test]
    fn t_one_line_in_history() {
        let mut h = History::new();
        h.add_line("testline".to_string());
        assert_eq!(h.previous_line(), "testline");
        assert_eq!(h.previous_line(), "testline");
        assert_eq!(h.next_line(), "testline");
        assert_eq!(h.next_line(), "");
    }

    #[test]
    fn t_multiple_lines_in_history() {
        let mut h = History::new();
        h.add_line("testline".to_string());
        h.add_line("foobar".to_string());
        assert_eq!(h.previous_line(), "foobar");
        assert_eq!(h.previous_line(), "testline");
        assert_eq!(h.next_line(), "testline");
        assert_eq!(h.previous_line(), "testline");
        assert_eq!(h.next_line(), "testline");
        assert_eq!(h.next_line(), "foobar");
        assert_eq!(h.next_line(), "");
        assert_eq!(h.next_line(), "");
    }

    #[test]
    fn t_save_and_load_history() {
        let tmp_dir = TempDir::new().unwrap();
        let file_path = tmp_dir.path().join("history.cmdline");

        let mut h = History::new();
        h.add_line("testline".to_string());
        h.add_line("foobar".to_string());

        // no file is created/opened if limit is zero
        h.save_to_file(file_path.clone(), 0).unwrap();
        assert!(!file_path.exists());

        // lines are written to file
        h.save_to_file(file_path.clone(), 10).unwrap();
        assert!(file_path.exists());

        // lines are loaded to new History
        let mut loaded_h = History::new();
        loaded_h.load_from_file(file_path).unwrap();
        assert_eq!(loaded_h.previous_line(), "foobar");
        assert_eq!(loaded_h.previous_line(), "testline");
        assert_eq!(loaded_h.next_line(), "testline");
        assert_eq!(loaded_h.next_line(), "foobar");
        assert_eq!(loaded_h.next_line(), "");
    }
}

use ratatui::DefaultTerminal;
use std::io;

pub struct Tui {
    terminal: Option<DefaultTerminal>,
}

impl Tui {
    pub fn new() -> Self {
        Tui { terminal: None }
    }

    pub fn run(&mut self) -> io::Result<()> {
        if self.terminal.is_none() {
            let mut terminal = ratatui::init();
            terminal.clear()?;
            self.terminal = Some(terminal);
        }
        Ok(())
    }
}

impl Drop for Tui {
    fn drop(&mut self) {
        if self.terminal.is_some() {
            self.terminal = None;
            ratatui::restore();
        }
    }
}

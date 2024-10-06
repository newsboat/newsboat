use ratatui::{style::Stylize, widgets::Paragraph, DefaultTerminal};
use std::io;

pub struct Tui {
    terminal: Option<DefaultTerminal>,
    title: String,
}

impl Tui {
    pub fn new() -> Self {
        Tui {
            terminal: None,
            title: String::new(),
        }
    }

    pub fn run(&mut self) -> io::Result<()> {
        if self.terminal.is_none() {
            let mut terminal = ratatui::init();
            terminal.clear()?;
            self.terminal = Some(terminal);
        }
        let terminal = self.terminal.as_mut().unwrap();
        terminal.draw(|frame| {
            let title = Paragraph::new(self.title.as_str())
                .white()
                .on_blue();
            frame.render_widget(title, frame.area());
        })?;
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

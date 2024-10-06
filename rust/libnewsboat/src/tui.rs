use ratatui::{
    crossterm::event::{self, poll, Event, KeyCode, KeyEventKind},
    layout::{Constraint, Direction, Layout},
    style::Stylize,
    text::Line,
    widgets::Paragraph,
    DefaultTerminal,
};
use std::{io, time::Duration};

pub struct Tui {
    terminal: Option<DefaultTerminal>,
    title: String,
    message: String,
}

impl Tui {
    pub fn new() -> Self {
        Tui {
            terminal: None,
            title: String::new(),
            message: String::new(),
        }
    }

    fn draw(&mut self) -> io::Result<()> {
        if self.terminal.is_none() {
            let mut terminal = ratatui::init();
            terminal.clear()?;
            self.terminal = Some(terminal);
        }
        let terminal = self.terminal.as_mut().unwrap();
        terminal.draw(|frame| {
            let chunks = Layout::default()
                .direction(Direction::Vertical)
                .constraints([
                    Constraint::Length(1),
                    Constraint::Fill(1),
                    Constraint::Length(1),
                ])
                .split(frame.area());
            let title = Paragraph::new(self.title.as_str()).white().on_blue();
            let message = Line::from(self.message.as_str()).white().on_black();
            frame.render_widget(title, chunks[0]);
            frame.render_widget(message, chunks[2]);
        })?;
        Ok(())
    }

    fn get_event(timeout: Option<u32>) -> io::Result<Option<String>> {
        let read_event = match timeout {
            None => poll(Duration::from_secs(0))?,
            Some(0) => true,
            Some(timeout) => poll(Duration::from_millis(timeout.into()))?,
        };
        if read_event {
            let event = match event::read()? {
                Event::Key(key_event) if key_event.kind == KeyEventKind::Press => {
                    match key_event.code {
                        KeyCode::Char(c) => Some(c.to_string()),
                        // TODO: Handle all keys which were handled by ncurses/stfl
                        _ => None,
                    }
                }
                _ => None,
            };
            Ok(event)
        } else {
            Ok(None)
        }
    }

    // TODO: Get implementation in line with description
    /// Draws UI and waits for next event (keypress, resize, etc.)
    ///
    ///
    pub fn run(&mut self, timeout: i32) -> io::Result<Option<String>> {
        let event = match timeout {
            -1 => {
                self.draw()?;
                None
            }
            -2 => Self::get_event(None)?,
            -3 => None, // TODO: Check if any rendering or size-determining is necessary
            timeout => {
                self.draw()?;
                Self::get_event(Some(timeout.abs() as u32))?
            }
        };
        Ok(event)
    }

    pub fn set_variable(&mut self, key: &str, value: &str) {
        match key {
            "head" => {
                self.title = value.into();
            }
            "msg" => {
                self.message = value.into();
            }
            _ => {
                // TODO: Handle other variables
                //self.message = key.into();
            }
        };
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

use nom::{
    bytes::complete::{tag, take_until},
    combinator::eof,
    multi::many0,
    IResult,
};
use ratatui::{
    crossterm::event::{self, poll, Event, KeyCode, KeyEventKind},
    layout::{Constraint, Direction, Layout},
    style::{Style, Stylize},
    text::Line,
    widgets::{List, ListItem, ListState, Paragraph},
    DefaultTerminal,
};
use std::{io, time::Duration};

pub struct Tui {
    terminal: Option<DefaultTerminal>,
    title: String,
    help: String,
    message: String,
    list_items: Vec<String>,
    list_state: ListState,
}

impl Tui {
    pub fn new() -> Self {
        Tui {
            terminal: None,
            title: String::new(),
            help: String::new(),
            message: String::new(),
            list_items: vec![],
            list_state: ListState::default(),
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
                    Constraint::Length(1),
                ])
                .split(frame.area());
            let title = Paragraph::new(self.title.as_str()).white().on_blue();
            let help = Line::from(self.help.as_str()).white().on_black();
            let message = Line::from(self.message.as_str()).white().on_black();

            let items: Vec<_> = self
                .list_items
                .iter()
                .map(|item| ListItem::from(item.as_str()))
                .collect();
            let list = List::new(items).highlight_style(Style::new().white().on_blue().bold());

            frame.render_widget(title, chunks[0]);
            frame.render_stateful_widget(list, chunks[1], &mut self.list_state);
            frame.render_widget(help, chunks[2]);
            frame.render_widget(message, chunks[3]);
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

    fn parse_stfl_listitem(input: &str) -> IResult<&str, &str> {
        let (input, _) = tag("{listitem text:\"")(input)?;
        // TODO: Handle escaped double quotes
        let (input, item) = take_until("\"")(input)?;
        let (input, _) = tag("\"}")(input)?;
        Ok((input, item))
    }

    fn parse_stfl_list(input: &str) -> IResult<&str, Vec<&str>> {
        let (input, _) = tag("{list")(input)?;
        let (input, items) = many0(Self::parse_stfl_listitem)(input)?;
        let (input, _) = tag("}")(input)?;
        let (input, _) = eof(input)?;
        Ok((input, items))
    }

    fn replace_list(&mut self, value: &str) {
        // TODO: Avoid unwrap
        let (_remainder, items) = Self::parse_stfl_list(value).unwrap();
        self.list_items = items.into_iter().map(|s| s.into()).collect();
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

    pub fn get_variable(&mut self, key: &str) -> &str {
        match key {
            "feeds:w" => "80", // TODO: Return actual width
            "feeds:h" => "10", // TODO: Return actual height
            _ => {
                self.message = format!("unhandled get: {}", key);
                ""
            }
        }
    }

    pub fn set_variable(&mut self, key: &str, value: &str) {
        match key {
            "head" => {
                self.title = value.into();
            }
            "help" => {
                self.help = value.into();
            }
            "msg" => {
                self.message = value.into();
            }
            "feeds_pos" => {
                // TODO: Handle non-numeric value?
                self.list_state.select(value.parse().ok());
            }
            "feeds_offset" => {
                // TODO: Handle
            }
            _ => {
                // TODO: Handle other variables
                self.message = format!("unhandled set: {}", key);
            }
        };
    }

    pub fn modify_form(&mut self, name: &str, mode: &str, value: &str) {
        match (name, mode) {
            ("feeds", "replace_inner") => self.replace_list(value),
            ("feeds", "replace") => (), // TODO: Handle (update style?)
            _ => self.message = format!("unhandled modify_form: {} {} {}", name, mode, value),
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

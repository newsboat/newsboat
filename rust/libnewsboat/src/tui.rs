use ratatui::{
    layout::{Constraint, Direction, Layout},
    style::{Style, Stylize},
    text::{Line, Span},
    widgets::{List, ListItem, ListState, Paragraph},
    DefaultTerminal,
};
use std::io;

mod input;
mod stfl;

pub struct Form {
    title: String,
    help: String,
    message: String,
    list_items: Vec<String>,
    list_state: ListState,
    list_viewport_dimensions: (u16, u16),
    text_percent: String,
    text_percent_width: u16,
}

impl Form {
    pub fn new() -> Self {
        Self {
            title: String::new(),
            help: String::new(),
            message: String::new(),
            list_items: vec![],
            list_state: ListState::default(),
            list_viewport_dimensions: (0, 0),
            text_percent: String::new(),
            text_percent_width: 0,
        }
    }

    fn replace_list(&mut self, value: &str) {
        // TODO: Avoid unwrap
        let (_remainder, items) = stfl::parse_list(value).unwrap();
        self.list_items = items;
    }

    pub fn get_variable(&mut self, key: &str) -> String {
        match key {
            "msg" => self.message.to_owned(),
            "feeds:w" | "items:w" | "urls:w" | "dls:w" | "taglist:w" => {
                self.list_viewport_dimensions.0.to_string()
            }
            "feeds:h" | "items:h" | "urls:h" | "dls:h" | "taglist:h" => {
                self.list_viewport_dimensions.1.to_string()
            }
            "article:w" | "helptext:w" => self.list_viewport_dimensions.0.to_string(),
            "article:h" | "helptext:h" => self.list_viewport_dimensions.1.to_string(),
            "title:w" => self.list_viewport_dimensions.0.to_string(),
            "article_offset" | "helptext_offset" => self.list_state.offset().to_string(),
            _ => {
                self.message = format!("unhandled get: {}", key);
                String::new()
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
            "feeds_pos" | "items_pos" | "urls_pos" | "dls_pos" | "taglist_pos" => {
                // TODO: Handle non-numeric value?
                self.list_state.select(value.parse().ok());
            }
            "feeds_offset" | "items_offset" | "urls_offset" | "dls_offset" | "taglist_offset" => {
                *self.list_state.offset_mut() = value.parse().unwrap();
            }
            "percent" => {
                self.text_percent = value.into();
            }
            "article_offset" | "helptext_offset" => {
                *self.list_state.offset_mut() = value.parse().unwrap();
            }
            "percentwidth" => {
                self.text_percent_width = value.parse().unwrap();
            }
            "highlight" => {
                // TODO: Handle (seems related to HelpFormAction
            }
            "hint-description"
            | "hint-separator"
            | "hint-keys-delimiter"
            | "hint-key"
            | "title"
            | "info"
            | "color_underline"
            | "color_bold"
            | "article" => {
                // TODO: Handle color change
            }
            _ => {
                // TODO: Handle other variables
                self.message = format!("unhandled set: {} - {}", key, value);
            }
        };
    }

    pub fn modify_form(&mut self, name: &str, mode: &str, value: &str) {
        match (name, mode) {
            ("feeds" | "items" | "urls" | "dls" | "taglist", "replace_inner") => {
                self.replace_list(value)
            }
            ("feeds" | "items" | "urls" | "dls" | "taglist", "replace") => (), // TODO: Handle (update style?)
            ("article" | "helptext", "replace_inner") => self.replace_list(value),
            ("article" | "helptext", "replace") => (), // TODO: Handle (update style?)
            _ => self.message = format!("unhandled modify_form: {} {} {}", name, mode, value),
        };
    }
}

pub struct Tui {
    terminal: Option<DefaultTerminal>,
}

impl Tui {
    pub fn new() -> Self {
        Tui { terminal: None }
    }

    fn draw(&mut self, form: &mut Form) -> io::Result<()> {
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
            let title = Paragraph::new(form.title.as_str()).white().on_blue();
            let help = Line::from(form.help.as_str()).white().on_black();
            let message = Line::from(form.message.as_str()).white().on_black();

            let items: Vec<_> = form
                .list_items
                .iter()
                .map(|item| ListItem::from(item.as_str()))
                .collect();
            let list = List::new(items).highlight_style(Style::new().white().on_blue().bold());

            frame.render_widget(title, chunks[0]);
            frame.render_stateful_widget(list, chunks[1], &mut form.list_state);
            frame.render_widget(message, chunks[3]);

            if form.text_percent.is_empty() {
                frame.render_widget(help, chunks[2]);
            } else {
                let parts = Layout::default()
                    .direction(Direction::Horizontal)
                    .constraints([
                        Constraint::Fill(1),
                        Constraint::Length(form.text_percent_width),
                    ])
                    .split(chunks[2]);

                let percent_widget = Span::from(form.text_percent.as_str());

                frame.render_widget(help, parts[0]);
                frame.render_widget(percent_widget, parts[1]);
            }

            form.list_viewport_dimensions = (chunks[1].width, chunks[1].height);
        })?;
        Ok(())
    }

    // TODO: Get implementation in line with description
    /// Draws UI and waits for next event (keypress, resize, etc.)
    ///
    ///
    pub fn run(&mut self, form: &mut Form, timeout: i32) -> io::Result<Option<String>> {
        let event = match timeout {
            -1 => {
                self.draw(form)?;
                None
            }
            -2 => input::get_event(None)?,
            -3 => {
                // TODO: Check if we can get rid of this draw (only updating widget dimensions instead)
                self.draw(form)?;
                None
            }
            timeout => {
                self.draw(form)?;
                input::get_event(Some(timeout.abs() as u32))?
            }
        };
        Ok(event)
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

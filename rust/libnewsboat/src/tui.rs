use ratatui::prelude::Rect;
use ratatui::{
    layout::{Constraint, Direction, Layout},
    style::{Color, Style, Stylize},
    text::{Line, Span},
    widgets::{List, ListItem},
    DefaultTerminal, Frame,
};
use std::{collections::HashMap, io, str::FromStr};
use style::StyleIdentifier;

use crate::stflrichtext::StflRichText;

mod input;
mod stfl;
mod style;

pub struct Form {
    title: String,
    help: StflRichText,
    message: String,
    list_items: Vec<StflRichText>,
    list_offset: usize,
    list_focus: Option<usize>,
    list_viewport_dimensions: (u16, u16),
    text_percent: String,
    text_percent_width: u16,
    styles: HashMap<StyleIdentifier, Style>,
}

impl Default for Form {
    fn default() -> Self {
        Self::new()
    }
}

impl Form {
    pub fn new() -> Self {
        Self {
            title: String::new(),
            help: StflRichText::default(),
            message: String::new(),
            list_items: vec![],
            list_offset: 0,
            list_focus: None,
            list_viewport_dimensions: (0, 0),
            text_percent: String::new(),
            text_percent_width: 0,
            styles: [
                (
                    StyleIdentifier::Title,
                    Style::new().yellow().on_blue().bold(),
                ),
                (
                    StyleIdentifier::Info,
                    Style::new().yellow().on_blue().bold(),
                ),
                (
                    StyleIdentifier::HintKey,
                    Style::new().yellow().on_blue().bold(),
                ),
                (
                    StyleIdentifier::HintKeysDelimiter,
                    Style::new().white().on_blue(),
                ),
                (
                    StyleIdentifier::HintSeparator,
                    Style::new().white().on_blue().bold(),
                ),
                (
                    StyleIdentifier::HintDescription,
                    Style::new().white().on_blue(),
                ),
            ]
            .into(),
        }
    }

    fn replace_list_content(&mut self, value: &str) {
        // TODO: Avoid unwrap
        let (_remainder, items) = stfl::parse_list(value).unwrap();
        let items = items.iter().map(|s| StflRichText::from_quoted(s)).collect();
        self.list_items = items;
    }

    fn replace_list(&mut self, value: &str) {
        // TODO: Avoid unwrap
        let (_remainder, named_styles) = stfl::parse_list_replacement(value).unwrap();
        for (name, style) in named_styles {
            let style = Self::convert_style(style);
            let style_identifier = StyleIdentifier::from_list_replace(name);
            if let Some(style_identifier) = style_identifier {
                self.styles.insert(style_identifier, style);
            } else {
                // TODO: Add style support
                self.message = format!("Unknown replace_list style name: {name}");
            }
        }
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
            "article_offset" | "helptext_offset" => self.list_offset.to_string(),
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
                self.help = StflRichText::from_quoted(value);
            }
            "msg" => {
                self.message = value.into();
            }
            "feeds_pos" | "items_pos" | "urls_pos" | "dls_pos" | "taglist_pos" => {
                // TODO: Handle non-numeric value?
                self.list_focus = value.parse().ok();
            }
            "feeds_offset" | "items_offset" | "urls_offset" | "dls_offset" | "taglist_offset" => {
                self.list_offset = value.parse().unwrap();
            }
            "percent" => {
                self.text_percent = value.into();
            }
            "article_offset" | "helptext_offset" => {
                self.list_offset = value.parse().unwrap();
            }
            "percentwidth" => {
                self.text_percent_width = value.parse().unwrap();
            }
            "highlight" => {
                // TODO: Handle (seems related to HelpFormAction
            }
            _ => {
                let style_identifier = StyleIdentifier::from_set_var(key);
                if let Some(style_identifier) = style_identifier {
                    let (_, style) = stfl::parse_style(value).unwrap();
                    let style = Self::convert_style(style);
                    self.styles.insert(style_identifier, style);
                } else {
                    // TODO: Handle other variables
                    self.message = format!("unhandled set: {} - {}", key, value);
                }
            }
        };
    }

    pub fn modify_form(&mut self, name: &str, mode: &str, value: &str) {
        match (name, mode) {
            ("feeds" | "items" | "urls" | "dls" | "taglist", "replace_inner") => {
                self.replace_list_content(value)
            }
            ("feeds" | "items" | "urls" | "dls" | "taglist", "replace") => self.replace_list(value),
            ("article" | "helptext", "replace_inner") => self.replace_list_content(value),
            ("article" | "helptext", "replace") => (), // TODO: Handle (update style?)
            _ => self.message = format!("unhandled modify_form: {} {} {}", name, mode, value),
        };
    }

    fn convert_style(stfl_style: stfl::Style) -> Style {
        let fg = Color::from_str(stfl_style.fg).unwrap_or(Color::White);
        let bg = Color::from_str(stfl_style.bg).unwrap_or(Color::Black);
        let mut style = Style::new().fg(fg).bg(bg);
        for &attribute in &stfl_style.attributes {
            match attribute {
                "standout" => {
                    todo!("Find out what this did in stfl and reproduce it with ratatui")
                }
                "underline" => style = style.underlined(),
                "reverse" => style = style.reversed(),
                // TODO: Check if stfl used a slow or a fast blink
                "blink" => style = style.slow_blink(),
                "dim" => style = style.dim(),
                "bold" => style = style.bold(),
                "protect" => {
                    todo!("Find out what this did in stfl and reproduce it with ratatui")
                }
                "invis" => {
                    todo!("Find out what this did in stfl and reproduce it with ratatui")
                }
                _ => todo!("Unhandled attribute: {attribute}, stfl style: {stfl_style:?}"),
            };
        }
        style
    }
}

pub struct Tui {
    terminal: Option<DefaultTerminal>,
}

impl Default for Tui {
    fn default() -> Self {
        Self::new()
    }
}

impl Tui {
    pub fn new() -> Self {
        Tui { terminal: None }
    }

    fn draw(&mut self, form: &mut Form) -> io::Result<()> {
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
            let fallback_style = Style::new().fg(Color::White).bg(Color::Black);
            let title_style = form
                .styles
                .get(&StyleIdentifier::Title)
                .unwrap_or(&fallback_style);
            let title = Line::styled(form.title.as_str(), title_style.to_owned());

            let message = Line::from(form.message.as_str()).white().on_black();

            let items: Vec<_> = form
                .list_items
                .iter()
                .enumerate()
                .map(|(i, item)| (form.list_focus == Some(i), item))
                .skip(form.list_offset)
                .take(chunks[1].height.into())
                .map(|(has_focus, item)| Self::style_line(item, has_focus, &form.styles))
                .map(ListItem::from)
                .collect();
            let list = List::new(items).highlight_style(Style::new().white().on_blue().bold());

            frame.render_widget(title, chunks[0]);
            frame.render_widget(list, chunks[1]);
            frame.render_widget(message, chunks[3]);

            if form.text_percent.is_empty() {
                Self::render_help(frame, &form.help, &form.styles, chunks[2]);
            } else {
                let parts = Layout::default()
                    .direction(Direction::Horizontal)
                    .constraints([
                        Constraint::Fill(1),
                        Constraint::Length(form.text_percent_width),
                    ])
                    .split(chunks[2]);

                let percent_widget = Span::from(form.text_percent.as_str());

                Self::render_help(frame, &form.help, &form.styles, parts[0]);
                frame.render_widget(percent_widget, parts[1]);
            }
        })?;
        Ok(())
    }

    fn style_line<'a>(
        richtext: &'a StflRichText,
        has_focus: bool,
        styles: &HashMap<StyleIdentifier, Style>,
    ) -> Line<'a> {
        let text = richtext.plaintext();
        let locs = richtext.style_switch_points().iter();
        let fallback = if has_focus {
            Style::new().yellow().on_blue().bold()
        } else {
            Style::new().white().on_black()
        };
        // TODO: Check if this also makes sense for text views (article, help)
        let default = if has_focus {
            styles.get(&StyleIdentifier::ListFocus).unwrap_or(&fallback)
        } else {
            styles
                .get(&StyleIdentifier::ListNormal)
                .unwrap_or(&fallback)
        };
        let mut pos = 0;
        let mut style = default;
        let mut line = Line::styled("", default.to_owned());
        for (&loc, style_name) in locs {
            if loc > pos {
                line.push_span(Span::styled(&text[pos..loc], style.to_owned()));
                pos = loc;
            }
            // TODO: Take `has_focus` into account
            let style_identifier = StyleIdentifier::from_stye_tag(style_name);
            style = style_identifier
                .and_then(|s| styles.get(&s))
                .unwrap_or(default);
        }
        if pos < text.len() {
            line.push_span(Span::styled(&text[pos..], style.to_owned()));
        }
        line
    }

    fn render_help(
        frame: &mut Frame,
        richtext: &StflRichText,
        styles: &HashMap<StyleIdentifier, Style>,
        area: Rect,
    ) {
        let text = richtext.plaintext();
        let locs = richtext.style_switch_points().iter();
        let fallback = Style::new().fg(Color::White).bg(Color::Black);
        let default = styles.get(&StyleIdentifier::Info).unwrap_or(&fallback);
        let mut pos = 0;
        let mut style = default;
        let mut line = Line::styled("", default.to_owned());
        for (&loc, style_name) in locs {
            if loc > pos {
                line.push_span(Span::styled(&text[pos..loc], style.to_owned()));
                pos = loc;
            }
            let style_identifier = StyleIdentifier::from_stye_tag(style_name);
            style = style_identifier
                .and_then(|s| styles.get(&s))
                .unwrap_or(default);
        }
        if pos < text.len() {
            line.push_span(Span::styled(&text[pos..], style.to_owned()));
        }
        frame.render_widget(line, area);
    }

    // TODO: Get implementation in line with description
    /// Draws UI and waits for next event (keypress, resize, etc.)
    ///
    ///
    pub fn run(&mut self, form: &mut Form, timeout: i32) -> io::Result<Option<String>> {
        if self.terminal.is_none() {
            let mut terminal = ratatui::init();
            terminal.clear()?;
            self.terminal = Some(terminal);
        }
        let event = match timeout {
            -1 => {
                self.draw(form)?;
                None
            }
            -2 => input::get_event(None)?,
            -3 => {
                // TODO: Check if this has changed by the time a PR is created
                // Should be save because we initialize it at the start of this method
                let terminal = self.terminal.as_mut().unwrap();
                // TODO: See if we can cache this value until a ^L or RESIZE event
                let size = terminal.size()?;
                // TODO: Check if all forms have exactly the same number of none-list lines
                // (assuming 3 lines: [title, help, message])
                form.list_viewport_dimensions = (size.width, size.height - 3);
                None
            }
            timeout => {
                let event = input::get_event(None)?;
                if event.is_some() {
                    event
                } else {
                    self.draw(form)?;
                    input::get_event(Some(timeout.unsigned_abs()))?
                }
            }
        };
        Ok(event)
    }

    pub fn reset(&mut self) {
        if self.terminal.is_some() {
            self.terminal = None;
            ratatui::restore();
        }
    }
}

impl Drop for Tui {
    fn drop(&mut self) {
        self.reset();
    }
}

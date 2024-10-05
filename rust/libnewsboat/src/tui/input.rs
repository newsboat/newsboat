use ratatui::crossterm::event::{self, poll, Event, KeyCode, KeyEventKind, KeyModifiers};
use std::{io, time::Duration};

pub fn get_event(timeout: Option<u32>) -> io::Result<Option<String>> {
    let read_event = match timeout {
        None => poll(Duration::from_secs(0))?,
        Some(0) => true,
        Some(timeout) => poll(Duration::from_millis(timeout.into()))?,
    };
    if read_event {
        let event = match event::read()? {
            Event::Key(key_event)
                if key_event.kind == KeyEventKind::Press
                    && !key_event.modifiers.contains(KeyModifiers::CONTROL) =>
            {
                match key_event.code {
                    KeyCode::Char(c) => Some(c.to_string()),
                    KeyCode::Enter => Some("ENTER".into()),
                    KeyCode::Backspace => Some("BACKSPACE".into()),
                    KeyCode::Left => Some("LEFT".into()),
                    KeyCode::Right => Some("RIGHT".into()),
                    KeyCode::Up => Some("UP".into()),
                    KeyCode::Down => Some("DOWN".into()),
                    KeyCode::PageUp => Some("PPAGE".into()),
                    KeyCode::PageDown => Some("NPAGE".into()),
                    KeyCode::Home => Some("HOME".into()),
                    KeyCode::End => Some("END".into()),
                    KeyCode::Esc => Some("ESC".into()),
                    KeyCode::Tab => Some("TAB".into()),
                    KeyCode::F(n) => Some(format!("F{}", n)),
                    // TODO: Handle all keys which were handled by ncurses/stfl
                    _ => None,
                }
            }
            Event::Key(key_event)
                if key_event.kind == KeyEventKind::Press
                    && key_event.modifiers.contains(KeyModifiers::CONTROL) =>
            {
                match key_event.code {
                    KeyCode::Char(c) if c.is_ascii() && c.is_alphabetic() => {
                        Some(format!("^{}", c.to_ascii_uppercase()))
                    }
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

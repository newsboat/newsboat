#[derive(Default)]
pub struct LineEdit {
    text: String,
    // Location in unicode codepoints
    cursor: usize,
}

impl LineEdit {
    pub fn get_text(&self) -> &str {
        &self.text
    }

    pub fn set_text(&mut self, text: String) {
        self.text = text;
        self.cursor = self.cursor.clamp(0, self.text.chars().count());
    }

    pub fn get_cursor_location(&self) -> usize {
        self.cursor
    }

    pub fn set_cursor_location(&mut self, location: usize) {
        self.cursor = location.clamp(0, self.text.chars().count());
    }

    pub fn insert_at_cursor(&mut self, text: &str) {
        let chars = self.text.chars();
        let before: String = chars.clone().take(self.cursor).collect();
        let after: String = chars.skip(self.cursor).collect();
        self.text = before + text + &after;
        self.cursor += text.chars().count();
    }

    fn remove_nth_char(&mut self, index: usize) {
        let chars = self.text.chars();
        let before: String = chars.clone().take(index).collect();
        let after: String = chars.skip(index + 1).collect();
        self.text = before + &after;
    }

    // Returns true if event is handled
    pub fn handle_event(&mut self, event: &str) -> bool {
        match event {
            "LEFT" => {
                if self.cursor >= 1 {
                    self.cursor -= 1;
                }
                true
            }
            "RIGHT" => {
                if self.cursor < self.text.chars().count() {
                    self.cursor += 1;
                }
                true
            }
            "HOME" | "^A" => {
                self.cursor = 0;
                true
            }
            "END" | "^E" => {
                self.cursor = self.text.chars().count();
                true
            }
            "DC" => {
                if self.cursor < self.text.chars().count() {
                    self.remove_nth_char(self.cursor);
                }
                true
            }
            "BACKSPACE" => {
                if self.cursor >= 1 {
                    self.remove_nth_char(self.cursor - 1);
                    self.cursor -= 1;
                }
                true
            }
            _ => false,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::LineEdit;

    #[test]
    fn t_set_text_clamps_cursor_to_valid_location() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("many words make long sentence".to_owned());
        lineedit.set_cursor_location(10);

        lineedit.set_text("short".to_owned());
        assert_eq!(lineedit.get_cursor_location(), 5);

        lineedit.set_text("longer".to_owned());
        assert_eq!(lineedit.get_cursor_location(), 5);

        lineedit.set_text("ß大".to_owned());
        assert_eq!(lineedit.get_cursor_location(), 2);

        lineedit.set_text("".to_owned());
        assert_eq!(lineedit.get_cursor_location(), 0);
    }

    #[test]
    fn t_set_cursor_location_clamps_to_valid_location() {
        let mut lineedit = LineEdit::default();

        lineedit.set_cursor_location(10);
        assert_eq!(lineedit.get_cursor_location(), 0);

        lineedit.set_text("ß大".to_owned());
        lineedit.set_cursor_location(10);
        assert_eq!(lineedit.get_cursor_location(), 2);

        lineedit.set_text("many words make long sentence".to_owned());
        lineedit.set_cursor_location(10);
        assert_eq!(lineedit.get_cursor_location(), 10);
    }

    #[test]
    fn t_handle_event_left() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("abc".to_owned());
        lineedit.set_cursor_location(2);

        lineedit.handle_event("LEFT");
        assert_eq!(lineedit.get_cursor_location(), 1);

        lineedit.handle_event("LEFT");
        assert_eq!(lineedit.get_cursor_location(), 0);

        lineedit.handle_event("LEFT");
        assert_eq!(lineedit.get_cursor_location(), 0);
    }

    #[test]
    fn t_handle_event_right() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("大ßc".to_owned());
        lineedit.set_cursor_location(1);

        lineedit.handle_event("RIGHT");
        assert_eq!(lineedit.get_cursor_location(), 2);

        lineedit.handle_event("RIGHT");
        assert_eq!(lineedit.get_cursor_location(), 3);

        lineedit.handle_event("RIGHT");
        assert_eq!(lineedit.get_cursor_location(), 3);
    }

    #[test]
    fn t_handle_event_home_ctrl_a() {
        for event in &["HOME", "^A"] {
            let mut lineedit = LineEdit::default();
            lineedit.set_text("abc".to_owned());
            lineedit.set_cursor_location(2);

            lineedit.handle_event(event);
            assert_eq!(lineedit.get_cursor_location(), 0);
        }
    }

    #[test]
    fn t_handle_event_end_ctrl_e() {
        for event in &["END", "^E"] {
            let mut lineedit = LineEdit::default();
            lineedit.set_text("大ßc".to_owned());
            lineedit.set_cursor_location(1);

            lineedit.handle_event(event);
            assert_eq!(lineedit.get_cursor_location(), 3);
        }
    }

    #[test]
    fn t_handle_event_delete() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("大ßc".to_owned());
        lineedit.set_cursor_location(1);

        lineedit.handle_event("DC");
        assert_eq!(lineedit.get_text(), "大c");
        assert_eq!(lineedit.get_cursor_location(), 1);

        lineedit.handle_event("DC");
        assert_eq!(lineedit.get_text(), "大");
        assert_eq!(lineedit.get_cursor_location(), 1);

        lineedit.handle_event("DC");
        assert_eq!(lineedit.get_text(), "大");
        assert_eq!(lineedit.get_cursor_location(), 1);
    }

    #[test]
    fn t_handle_event_backspace() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("大ßc".to_owned());
        lineedit.set_cursor_location(2);

        lineedit.handle_event("BACKSPACE");
        assert_eq!(lineedit.get_text(), "大c");
        assert_eq!(lineedit.get_cursor_location(), 1);

        lineedit.handle_event("BACKSPACE");
        assert_eq!(lineedit.get_text(), "c");
        assert_eq!(lineedit.get_cursor_location(), 0);

        lineedit.handle_event("BACKSPACE");
        assert_eq!(lineedit.get_text(), "c");
        assert_eq!(lineedit.get_cursor_location(), 0);
    }

    #[test]
    fn t_insert_at_cursor() {
        let mut lineedit = LineEdit::default();
        lineedit.set_text("大c".to_owned());
        lineedit.set_cursor_location(1);

        lineedit.insert_at_cursor("ß");
        assert_eq!(lineedit.get_text(), "大ßc");
        assert_eq!(lineedit.get_cursor_location(), 2);

        lineedit.insert_at_cursor("more text");
        assert_eq!(lineedit.get_text(), "大ßmore textc");
        assert_eq!(lineedit.get_cursor_location(), 11);
    }
}

use std::collections::BTreeMap;

/// Quote a string for use with stfl by replacing all occurences of "<" with "<>"
fn quote_for_stfl(string: &str) -> String {
    string.replace('<', "<>")
}

#[derive(Clone)]
pub struct StflRichText {
    text: String,
    style_tags: BTreeMap<usize, String>,
}

impl StflRichText {
    pub fn from_plaintext(text: &str) -> Self {
        let quoted_text = quote_for_stfl(text);
        Self::from_quoted(&quoted_text)
    }

    pub fn from_plaintext_with_style(text: &str, style_tag: &str) -> Self {
        let quoted_text = quote_for_stfl(text);
        let mut richtext = Self::from_quoted(&quoted_text);
        if !text.is_empty() {
            richtext.apply_style_tag(style_tag, 0, text.len());
        }
        richtext
    }

    pub fn from_quoted(text: &str) -> Self {
        let (text, style_tags) = Self::extract_style_tags(text);
        Self { text, style_tags }
    }

    pub fn plaintext(&self) -> &str {
        &self.text
    }

    pub fn quoted(&self) -> String {
        self.insert_style_tags()
    }

    pub fn apply_style_tag(&mut self, tag: &str, start: usize, end: usize) {
        self.merge_style_tag(tag, start, end);
    }

    pub fn highlight_searchphrase(&mut self, search: &str, case_insensitive: bool) {
        let literal_pattern = regex::escape(search);

        if let Ok(re) = regex::RegexBuilder::new(&literal_pattern)
            .case_insensitive(case_insensitive)
            .build()
        {
            let text = self.text.clone();
            for found in re.find_iter(&text) {
                self.apply_style_tag("<hl>", found.start(), found.end());
            }
        }
    }

    pub fn append(&mut self, other: &Self) {
        let offset = self.text.len();
        self.text += &other.text;
        for (&pos, tag) in &other.style_tags {
            self.style_tags.insert(offset + pos, tag.to_owned());
        }
    }

    fn extract_style_tags(text: &str) -> (String, BTreeMap<usize, String>) {
        let mut result = text.to_owned();
        let mut tags = BTreeMap::new();

        let mut pos = 0;
        while pos < result.len() {
            let remainder = &result[pos..];
            let tag_start = remainder.find(['<', '>']);
            let Some(tag_start) = tag_start else {
                break;
            };
            if remainder[tag_start..].starts_with('>') {
                // Keep unmatched '>' (stfl way of encoding a literal '>')
                pos += tag_start + 1;
                continue;
            }
            let remainder_in_tag = &remainder[tag_start + 1..];
            let tag_end = remainder_in_tag.find(['<', '>']).map(|i| i + tag_start + 1);
            let Some(tag_end) = tag_end else {
                break;
            };
            if remainder[tag_end..].starts_with('<') {
                // First '<' bracket is unmatched, ignoring it
                pos += tag_start + 1;
                continue;
            }
            if tag_end - tag_start == 1 {
                // Convert "<>" into "<" (stfl way of encoding a literal '<')
                result.remove(pos + tag_end);
                pos += tag_start + 1;
                continue;
            }
            tags.insert(
                pos + tag_start,
                remainder[tag_start..tag_end + 1].to_owned(),
            );
            result.replace_range(pos + tag_start..pos + tag_end + 1, "");
            pos += tag_start;
        }

        (result, tags)
    }

    fn merge_style_tag(&mut self, tag: &str, start: usize, end: usize) {
        if end <= start {
            return;
        }

        // Find the latest tag occurring before `end`.
        // It is important that looping executes in ascending order of location.
        let mut latest_tag = "</>";
        for (&pos, tag) in self.style_tags.iter() {
            if pos > end {
                break;
            }
            latest_tag = tag;
        }
        let previous_tag = latest_tag.to_owned();
        self.style_tags.insert(start, tag.into());
        self.style_tags.insert(end, previous_tag);

        // Remove any old tags between the start and end marker
        self.style_tags.retain(|&k, _v| k <= start || k >= end);
    }

    fn insert_style_tags(&self) -> String {
        let mut result = self.text.clone();
        let mut tags = self.style_tags.clone();

        // Expand "<" into "<>" (reverse of what happened in extract_style_tags()
        let mut pos = 0;
        while pos < result.len() {
            let bracket = result[pos..].find('<').map(|i| i + pos);
            let Some(bracket) = bracket else {
                break;
            };
            pos = bracket + 1;
            // Add to strings in the `tags` map so we don't have to shift all the positions in that map
            // (would be necessary if inserting directly into `str`)
            // NOTE: There can already be a tag at this position (e.i. immediately after the `<`),
            // so we must not overwrite the entry.
            tags.entry(pos).or_default().insert(0, '>');
        }

        // Insert tags right-to-left (starting at the end of the string)
        // This avoids recalculation of tag insertion points due to tags shifting all positions to
        // the right
        for (&pos, tag) in tags.iter().rev() {
            result.insert_str(pos, tag);
        }

        result
    }
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use super::StflRichText;
    use super::quote_for_stfl;

    #[test]
    fn t_quote_for_stfl() {
        assert_eq!(&quote_for_stfl("<"), "<>");
        assert_eq!(&quote_for_stfl("<<><><><"), "<><>><>><>><>");
        assert_eq!(&quote_for_stfl("test"), "test");
    }

    #[test]
    fn t_from_plaintext_with_style() {
        assert_eq!(
            StflRichText::from_plaintext_with_style("", "<abc>").quoted(),
            ""
        );
        assert_eq!(
            StflRichText::from_plaintext_with_style(" ", "<space>").quoted(),
            "<space> </>"
        );
        assert_eq!(
            StflRichText::from_plaintext_with_style("ENTER", "<key>").quoted(),
            "<key>ENTER</>"
        );
        assert_eq!(
            StflRichText::from_plaintext_with_style("<", "<bracket>").quoted(),
            "<bracket><></>"
        );
    }

    #[test]
    fn t_left_angle_bracket_immediately_before_a_tag() {
        // Regression test for https://github.com/newsboat/newsboat/issues/3007

        let plain = "<";
        let quoted = format!("<hl>{}</>", quote_for_stfl(plain));
        let tags = BTreeMap::from_iter([(0, "<hl>".to_string()), (1, "</>".to_string())]);

        let actual = StflRichText::from_quoted(&quoted);

        assert_eq!(actual.style_tags, tags);
        assert_eq!(actual.plaintext(), plain);
        assert_eq!(actual.quoted(), quoted);
    }

    #[test]
    fn t_append_plaintext() {
        let mut text = StflRichText::from_plaintext("hello");
        text.append(&StflRichText::from_plaintext(" world"));
        assert_eq!(text.quoted(), "hello world");

        let mut text_with_angle_brackets = StflRichText::from_plaintext("abc<");
        text_with_angle_brackets.append(&StflRichText::from_plaintext("def"));
        assert_eq!(text_with_angle_brackets.quoted(), "abc<>def");
    }

    #[test]
    fn t_append_plaintext_to_richtext() {
        let mut text = StflRichText::from_quoted("<abc>hello</>");
        text.append(&StflRichText::from_plaintext(" world"));
        assert_eq!(text.quoted(), "<abc>hello</> world");
    }

    #[test]
    fn t_append_richtext() {
        let mut text = StflRichText::from_quoted("<abc>first</>");
        text.append(&StflRichText::from_quoted("<def>second</>"));
        assert_eq!(text.quoted(), "<abc>first<def>second</>");
    }

    proptest::proptest! {
        #[test]
        fn t_from_plaintext_does_not_crash_on_any_utf8_inputs(ref input in "\\PC*") {
            // Result explicitly ignored because we just want to make sure this call doesn't panic.
            let _ = StflRichText::from_plaintext(input);
        }

        #[test]
        fn t_from_quoted_does_not_crash_on_any_utf8_inputs(ref input in "\\PC*") {
            // Result explicitly ignored because we just want to make sure this call doesn't panic.
            let _ = StflRichText::from_quoted(input);
        }
    }
}

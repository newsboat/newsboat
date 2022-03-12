use crate::utils::remove_soft_hyphens;

struct Attribute {
    name: String,
    value: String,
}

pub struct TagSoupPullParser {
    input: String,
    current_event: Event,
    text: String,
    attributes: Vec<Attribute>,
}

pub enum Event {
    StartDocument,
    EndDocument,
    StartTag,
    EndTag,
    Text,
}

impl TagSoupPullParser {
    pub fn new(input: &str) -> TagSoupPullParser {
        TagSoupPullParser {
            input: input.to_string(),
            current_event: Event::StartDocument,
            text: String::new(),
            attributes: vec![],
        }
    }

    pub fn next(self: &mut TagSoupPullParser) -> &Event {
        self.attributes.clear();
        self.text.clear();

        if self.input.is_empty() {
            self.current_event = Event::EndDocument;
        }

        match self.current_event {
            Event::StartDocument | Event::StartTag | Event::EndTag => {
                match self.input.chars().next() {
                    None => self.current_event = Event::EndDocument,
                    Some('<') => self.handle_tag(),
                    Some(c) => self.handle_text(c),
                }
            }
            Event::Text => self.handle_tag(),
            Event::EndDocument => {}
        }

        self.get_event_type()
    }

    pub fn get_attribute_value(self: &TagSoupPullParser, name: &str) -> Option<&String> {
        self.attributes
            .iter()
            .find(|attribute| attribute.name == name)
            .map(|attribute| &attribute.value)
    }

    pub fn get_event_type(self: &TagSoupPullParser) -> &Event {
        &self.current_event
    }

    pub fn get_text(self: &TagSoupPullParser) -> &str {
        &self.text
    }

    fn handle_tag(self: &TagSoupPullParser) {
        panic!("Unimplemented");
    }

    fn handle_text(self: &mut TagSoupPullParser, c: char) {
        self.text.push(c);

        let (text, remainder) = TagSoupPullParser::split_once(&self.input, '<');
        self.text.push_str(text);
        self.input = remainder.unwrap_or("").to_string();

        self.text = TagSoupPullParser::decode_entities(&self.text);

        remove_soft_hyphens(&mut self.text);

        self.current_event = Event::Text;
    }

    fn decode_entities(input: &str) -> String {
        panic!("Unimplemented");
    }

    fn split_once(input: &str, split_at: char) -> (&str, Option<&str>) {
        let mut x = input.splitn(2, split_at);
        let text_before = x.next().unwrap_or("");
        let remainder = x.next();
        (text_before, remainder)
    }
}

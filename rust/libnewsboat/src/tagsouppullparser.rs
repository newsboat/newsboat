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

    fn handle_text(self: &TagSoupPullParser, c: char) {
        panic!("Unimplemented");
    }
}

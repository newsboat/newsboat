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

    pub fn next() -> Event {
        panic!("Unimplemented");
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
}

pub struct TagSoupPullParser {
    input: String,
    current_event: Event,
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
        }
    }

    pub fn next() -> Event {
        panic!("Unimplemented");
    }

    pub fn get_attribute_value(name: &str) -> Option<String> {
        panic!("Unimplemented");
    }

    pub fn get_event_type(self: &TagSoupPullParser) -> &Event {
        &self.current_event
    }

    pub fn get_text() -> String {
        panic!("Unimplemented");
    }
}

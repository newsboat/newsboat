pub struct TagSoupPullParser {
    input: String,
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
        }
    }

    pub fn next() -> Event {
        panic!("Unimplemented");
    }

    pub fn get_attribute_value(name: &str) -> Option<String> {
        panic!("Unimplemented");
    }

    pub fn get_event_type() -> Event {
        panic!("Unimplemented");
    }

    pub fn get_text() -> String {
        panic!("Unimplemented");
    }
}

use libnewsboat::tagsouppullparser;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct TagSoupPullParser(tagsouppullparser::TagSoupPullParser);

#[cxx::bridge(namespace = "newsboat::tagsouppullparser::bridged")]
mod bridged {
    // This has to be in sync with logger::Level in rust/libnewsboat/src/tagsouppullparser.rs
    enum Event {
        START_DOCUMENT = 0,
        END_DOCUMENT,
        START_TAG,
        END_TAG,
        TEXT,
    }

    extern "Rust" {
        type TagSoupPullParser;

        fn create(source: String) -> Box<TagSoupPullParser>;
        fn get_attribute_value(obj: &TagSoupPullParser, name: &str, result: &mut String) -> bool;
        fn get_text(obj: &TagSoupPullParser) -> String;
        fn next(obj: &mut TagSoupPullParser) -> Event;
    }
}

fn create(source: String) -> Box<TagSoupPullParser> {
    Box::new(TagSoupPullParser(
        tagsouppullparser::TagSoupPullParser::new(source),
    ))
}

fn get_attribute_value(obj: &TagSoupPullParser, name: &str, result: &mut String) -> bool {
    if let Some(value) = obj.0.get_attribute_value(name) {
        *result = value;
        true
    } else {
        false
    }
}

fn get_text(obj: &TagSoupPullParser) -> String {
    obj.0.get_text()
}

fn next(obj: &mut TagSoupPullParser) -> bridged::Event {
    match obj.0.next() {
        tagsouppullparser::Event::StartDocument => bridged::Event::START_DOCUMENT,
        tagsouppullparser::Event::EndDocument => bridged::Event::END_DOCUMENT,
        tagsouppullparser::Event::StartTag => bridged::Event::START_TAG,
        tagsouppullparser::Event::EndTag => bridged::Event::END_TAG,
        tagsouppullparser::Event::Text => bridged::Event::TEXT,
    }
}

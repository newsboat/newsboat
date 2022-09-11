use libnewsboat::tagsouppullparser;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct TagSoupPullParser(tagsouppullparser::TagSoupPullParser);

#[cxx::bridge(namespace = "newsboat::tagsouppullparser::bridged")]
mod bridged {
    enum Event {
        START_DOCUMENT = 1,
        END_DOCUMENT,
        START_TAG,
        END_TAG,
        TEXT,
    }

    extern "Rust" {
        type TagSoupPullParser;

        fn create(input: &str) -> Box<TagSoupPullParser>;
        fn extract_next(parser: &mut TagSoupPullParser) -> Event;
        fn get_attribute_value(
            parser: &TagSoupPullParser,
            name: &str,
            out_value: &mut String,
        ) -> bool;
        fn get_event_type(parser: &TagSoupPullParser) -> Event;
        fn get_text(parser: &TagSoupPullParser) -> &str;
    }
}

fn map_event_for_ffi(event: &tagsouppullparser::Event) -> bridged::Event {
    match event {
        tagsouppullparser::Event::StartDocument => bridged::Event::START_DOCUMENT,
        tagsouppullparser::Event::EndDocument => bridged::Event::END_DOCUMENT,
        tagsouppullparser::Event::StartTag => bridged::Event::START_TAG,
        tagsouppullparser::Event::EndTag => bridged::Event::END_TAG,
        tagsouppullparser::Event::Text => bridged::Event::TEXT,
    }
}

fn create(input: &str) -> Box<TagSoupPullParser> {
    Box::new(TagSoupPullParser(
        tagsouppullparser::TagSoupPullParser::new(input),
    ))
}

fn extract_next(parser: &mut TagSoupPullParser) -> bridged::Event {
    map_event_for_ffi(parser.0.extract_next())
}

fn get_attribute_value(parser: &TagSoupPullParser, name: &str, out_value: &mut String) -> bool {
    match parser.0.get_attribute_value(name) {
        Some(value) => {
            *out_value = value.to_owned();
            true
        }
        None => false,
    }
}

fn get_event_type(parser: &TagSoupPullParser) -> bridged::Event {
    map_event_for_ffi(parser.0.get_event_type())
}

fn get_text(parser: &TagSoupPullParser) -> &str {
    parser.0.get_text()
}

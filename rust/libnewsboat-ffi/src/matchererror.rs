use libnewsboat::matchererror;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct MatcherError(matchererror::MatcherError);

#[cxx::bridge(namespace = "newsboat::matchererror::bridged")]
mod bridged {
    #[repr(u8)]
    enum Type {
        AttributeUnavailable = 0,
        InvalidRegex = 1,
    }

    struct MatcherErrorFfi {
        err_type: Type,
        info: String,
        info2: String,
    }

    extern "Rust" {
        type MatcherError;

        fn matcher_error_to_ffi(error: &MatcherError) -> MatcherErrorFfi;

        fn get_test_attr_unavail_error() -> Box<MatcherError>;
        fn get_test_invalid_regex_error() -> Box<MatcherError>;
    }
}

fn matcher_error_to_ffi(error: &MatcherError) -> bridged::MatcherErrorFfi {
    match &error.0 {
        matchererror::MatcherError::AttributeUnavailable { attr } => bridged::MatcherErrorFfi {
            err_type: bridged::Type::AttributeUnavailable,
            info: attr.to_owned(),
            info2: String::new(),
        },
        matchererror::MatcherError::InvalidRegex { regex, errmsg } => bridged::MatcherErrorFfi {
            err_type: bridged::Type::InvalidRegex,
            info: regex.to_owned(),
            info2: errmsg.to_owned(),
        },
    }
}

fn get_test_attr_unavail_error() -> Box<MatcherError> {
    Box::new(MatcherError(
        matchererror::MatcherError::AttributeUnavailable {
            attr: String::from("test_attribute"),
        },
    ))
}

fn get_test_invalid_regex_error() -> Box<MatcherError> {
    Box::new(MatcherError(matchererror::MatcherError::InvalidRegex {
        regex: String::from("?!"),
        errmsg: String::from("inconceivable happened!"),
    }))
}

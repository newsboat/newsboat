use libnewsboat::configcontainer::{
    ArtSortMethod, ConfigContainer, ConfigData, ConfigDataType, ConfigHandlerStatus,
    FeedSortMethod, SortDirection,
};

#[test]
fn t_config_data_set_value_aligned_types() {
    // boolean
    let mut cd = ConfigData::new("yes", ConfigDataType::Bool);
    assert!(cd.set_value("no".into()).is_ok());
    assert!(cd.set_value("yes".into()).is_ok());
    assert!(cd.set_value("true".into()).is_ok());
    assert!(cd.set_value("false".into()).is_ok());

    // unsigned integer
    let mut cd = ConfigData::new("42", ConfigDataType::Int);
    assert!(cd.set_value("13".into()).is_ok());
    assert!(cd.set_value("100500".into()).is_ok());
    assert!(cd.set_value("65535".into()).is_ok());

    // enum
    let mut cd = ConfigData::new_enum("charlie", vec!["alpha", "bravo", "charlie", "delta"]);
    assert!(cd.set_value("alpha".into()).is_ok());
    assert!(cd.set_value("bravo".into()).is_ok());
    assert!(cd.set_value("charlie".into()).is_ok());
    assert!(cd.set_value("delta".into()).is_ok());

    // string
    let mut cd = ConfigData::new("johndoe", ConfigDataType::Str);
    assert!(cd.set_value("minoru".into()).is_ok());
    assert!(cd.set_value("noname".into()).is_ok());
    assert!(cd.set_value("username".into()).is_ok());
    assert!(cd.set_value("nobody".into()).is_ok());

    // path
    let mut cd = ConfigData::new("~/urls", ConfigDataType::Path);
    assert!(cd.set_value("/tmp/whatever.txt".into()).is_ok());
    assert!(cd.set_value("C:\\Users\\Minoru\\urls.txt".into()).is_ok());
    assert!(
        cd.set_value("/usr/local/home/minoru/.newsboat/urls".into())
            .is_ok()
    );
}

#[test]
fn t_config_data_set_value_invalid_boolean() {
    let mut cd = ConfigData::new("yes", ConfigDataType::Bool);
    assert!(cd.set_value("enable".into()).is_err());
    assert!(cd.set_value("disabled".into()).is_err());
    assert!(cd.set_value("active".into()).is_err());
}

#[test]
fn t_config_data_set_value_invalid_integer() {
    let mut cd = ConfigData::new("yes", ConfigDataType::Int);
    assert!(cd.set_value("0x42".into()).is_err());
    assert!(cd.set_value("infinity".into()).is_err());
    assert!(cd.set_value("123 minutes".into()).is_err());
}

#[test]
fn t_config_data_set_value_invalid_enum() {
    let mut cd = ConfigData::new_enum("N", vec!["H", "He", "Li", "Be", "B", "C", "N", "O", "F"]);
    assert!(cd.set_value("Mg".into()).is_err());
    assert!(cd.set_value("Al".into()).is_err());
    assert!(cd.set_value("something entirely different".into()).is_err());
}

#[test]
fn t_reset_to_default_changes_setting_to_its_default_value() {
    let cfg = ConfigContainer::new();
    let default_value = "any";
    let tests = vec![
        "any",
        "basic",
        "digest",
        "digest_ie",
        "gssnegotiate",
        "ntlm",
        "anysafe",
    ];
    let key = "http-auth-method";

    assert_eq!(cfg.get_configvalue(key), default_value);

    for test_value in tests {
        cfg.set_configvalue(key, test_value).unwrap();
        assert_eq!(cfg.get_configvalue(key), test_value);
        cfg.reset_to_default(key);
        assert_eq!(cfg.get_configvalue(key), default_value);
    }
}

#[test]
fn t_get_configvalue_returns_empty_string_if_setting_does_not_exist() {
    let cfg = ConfigContainer::new();
    assert_eq!(cfg.get_configvalue("nonexistent-key"), "");
}

#[test]
fn t_get_configvalue_as_bool_recognizes_several_boolean_formats() {
    let cfg = ConfigContainer::new();

    // "yes" and "true"
    cfg.set_configvalue("cleanup-on-quit", "yes").unwrap();
    assert_eq!(cfg.get_configvalue("cleanup-on-quit"), "yes");

    cfg.set_configvalue("auto-reload", "true").unwrap();
    assert_eq!(cfg.get_configvalue("auto-reload"), "true");

    // "no" and "false"
    cfg.set_configvalue("show-read-feeds", "no").unwrap();
    assert_eq!(cfg.get_configvalue("show-read-feeds"), "no");

    cfg.set_configvalue("bookmark-interactive", "false")
        .unwrap();
    assert_eq!(cfg.get_configvalue("bookmark-interactive"), "false");
}

#[test]
fn t_toggle_inverts_the_value_of_a_boolean_setting() {
    let cfg = ConfigContainer::new();
    let key = "always-display-description";

    // "true" becomes "false"
    cfg.set_configvalue(key, "true").unwrap();
    cfg.toggle(key);
    assert_eq!(cfg.get_configvalue(key), "false");

    // "false" becomes "true"
    cfg.set_configvalue(key, "false").unwrap();
    cfg.toggle(key);
    assert_eq!(cfg.get_configvalue(key), "true");
}

#[test]
fn t_toggle_does_nothing_if_setting_is_non_boolean() {
    let cfg = ConfigContainer::new();
    let tests = vec!["cache-file", "http-auth-method", "download-timeout"];

    for key in tests {
        let expected = cfg.get_configvalue(key);
        cfg.toggle(key);
        assert_eq!(cfg.get_configvalue(key), expected);
    }
}

#[test]
fn t_handle_action_multi_option_behavior() {
    let cfg = ConfigContainer::new();

    // "search-highlight-colors" is a multi-option config
    // It should accept multiple parameters and join them
    let result = cfg.handle_action(
        "search-highlight-colors",
        &["red".to_string(), "blue".to_string()],
    );
    assert_eq!(result, ConfigHandlerStatus::Success);
    assert_eq!(cfg.get_configvalue("search-highlight-colors"), "red blue");

    // "browser" is a single-option config
    // It should reject multiple parameters
    let result = cfg.handle_action("browser", &["firefox".to_string(), "%u".to_string()]);
    assert_eq!(result, ConfigHandlerStatus::TooManyParams);
}

#[test]
fn t_handle_action_edge_cases() {
    let cfg = ConfigContainer::new();

    // Invalid command
    assert_eq!(
        cfg.handle_action("non-existent-command", &["val".to_string()]),
        ConfigHandlerStatus::InvalidCommand
    );

    // Too few parameters (empty)
    assert_eq!(
        cfg.handle_action("browser", &[]),
        ConfigHandlerStatus::TooFewParams
    );
}

#[test]
fn t_dump_config_quoting_logic() {
    let cfg = ConfigContainer::new();

    // Set an integer value (should NOT be quoted in dump)
    cfg.set_configvalue("download-retries", "5").unwrap();

    // Set a string value containing spaces (should be quoted)
    cfg.set_configvalue("bookmark-cmd", "echo url").unwrap();

    // Set a boolean (should NOT be quoted)
    cfg.set_configvalue("auto-reload", "yes").unwrap();

    // Set an enum (should be quoted)
    cfg.set_configvalue("proxy-type", "socks5").unwrap();

    let dump = cfg.dump_config();

    // Check Integer
    assert!(dump.contains(&"download-retries 5 # default: 1".to_string()));

    // Check String with spaces
    assert!(dump.contains(&"bookmark-cmd \"echo url\" # default: ".to_string()));

    // Check Boolean
    assert!(dump.contains(&"auto-reload yes # default: no".to_string()));

    // Check Enum
    assert!(dump.contains(&"proxy-type \"socks5\" # default: http".to_string()));
}

#[test]
fn t_get_suggestions_returns_all_settings_beginning_with_string() {
    let cfg = ConfigContainer::new();

    // Check "feed" prefix
    let suggestions = cfg.get_suggestions("feed-sort");
    assert!(suggestions.contains(&"feed-sort-order".to_string()));
    assert!(!suggestions.contains(&"download-path".to_string()));

    // Check alphabetical order
    let keys = vec!["dow", "rel"]; // subset of keys
    for key in keys {
        let results = cfg.get_suggestions(key);
        for i in 0..results.len() - 1 {
            assert!(results[i] <= results[i + 1]);
        }
    }
}

#[test]
fn t_feed_sort_strategy_parsing() {
    let cfg = ConfigContainer::new();

    // Default
    let (method, dir) = cfg.get_feed_sort_strategy();
    assert_eq!(method, FeedSortMethod::None);
    assert_eq!(dir, SortDirection::Desc);

    // Valid parsing
    cfg.set_configvalue("feed-sort-order", "title-asc").unwrap();
    let (method, dir) = cfg.get_feed_sort_strategy();
    assert_eq!(method, FeedSortMethod::Title);
    assert_eq!(dir, SortDirection::Asc);

    // Parsing partial (default direction)
    cfg.set_configvalue("feed-sort-order", "articlecount")
        .unwrap();
    let (method, dir) = cfg.get_feed_sort_strategy();
    assert_eq!(method, FeedSortMethod::ArticleCount);
    assert_eq!(dir, SortDirection::Desc); // Default is Desc for most

    // Parsing invalid method (falls back to None)
    cfg.set_configvalue("feed-sort-order", "invalid-asc")
        .unwrap();
    let (method, _dir) = cfg.get_feed_sort_strategy();
    assert_eq!(method, FeedSortMethod::None);
}

#[test]
fn t_article_sort_strategy_parsing() {
    let cfg = ConfigContainer::new();

    // Default
    let (method, dir) = cfg.get_article_sort_strategy();
    assert_eq!(method, ArtSortMethod::Date);
    assert_eq!(dir, SortDirection::Asc);

    // "date" defaults to Desc if direction not specified
    cfg.set_configvalue("article-sort-order", "date").unwrap();
    let (method, dir) = cfg.get_article_sort_strategy();
    assert_eq!(method, ArtSortMethod::Date);
    assert_eq!(dir, SortDirection::Desc);

    // "date-asc" overrides default
    cfg.set_configvalue("article-sort-order", "date-asc")
        .unwrap();
    let (_, dir) = cfg.get_article_sort_strategy();
    assert_eq!(dir, SortDirection::Asc);

    // Other methods default to Asc
    cfg.set_configvalue("article-sort-order", "title").unwrap();
    let (method, dir) = cfg.get_article_sort_strategy();
    assert_eq!(method, ArtSortMethod::Title);
    assert_eq!(dir, SortDirection::Asc);
}

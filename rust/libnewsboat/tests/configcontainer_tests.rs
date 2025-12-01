use libnewsboat::configcontainer::{
    ArtSortMethod, ConfigContainer, ConfigData, ConfigDataType, ConfigHandlerStatus,
    FeedSortMethod, SortDirection,
};

#[test]
fn t_config_data_int_validation() {
    let mut cd = ConfigData::new("0", ConfigDataType::Int);

    // Valid integers
    assert!(cd.set_value("42".into()).is_ok());
    assert_eq!(cd.value, "42");
    assert!(cd.set_value("-10".into()).is_ok());
    assert_eq!(cd.value, "-10");

    // Invalid integers
    assert!(cd.set_value("foo".into()).is_err());
    assert_eq!(cd.value, "-10"); // Value should remain unchanged
    assert!(cd.set_value("12.5".into()).is_err());
}

#[test]
fn t_config_data_bool_validation() {
    let mut cd = ConfigData::new("false", ConfigDataType::Bool);

    // Valid booleans
    for val in &["true", "yes", "false", "no"] {
        assert!(cd.set_value(val.to_string()).is_ok());
        assert_eq!(cd.value, *val);
    }

    // Invalid booleans
    assert!(cd.set_value("maybe".into()).is_err());
    assert!(cd.set_value("1".into()).is_err());
    assert!(cd.set_value("0".into()).is_err());
}

#[test]
fn t_config_data_enum_validation() {
    let mut cd = ConfigData::new_enum("one", vec!["one", "two", "three"]);

    // Valid enum values
    assert!(cd.set_value("two".into()).is_ok());
    assert_eq!(cd.value, "two");

    // Invalid enum values
    assert!(cd.set_value("four".into()).is_err());
    assert_eq!(cd.value, "two"); // Value should remain unchanged
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

    let dump = cfg.dump_config();

    // Check Integer
    assert!(dump.contains(&"download-retries 5 # default: 1".to_string()));

    // Check String with spaces
    assert!(dump.contains(&"bookmark-cmd \"echo url\" # default: ".to_string()));

    // Check Boolean
    assert!(dump.contains(&"auto-reload yes # default: no".to_string()));
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

use crate::filepath::PathBuf;
use cxx::{ExternType, type_id};
use libnewsboat::configcontainer;
use std::pin::Pin;

pub struct ConfigContainer(pub configcontainer::ConfigContainer);

unsafe impl ExternType for ConfigContainer {
    type Id = type_id!("newsboat::configcontainer::bridged::ConfigContainer");
    type Kind = cxx::kind::Opaque;
}

#[cxx::bridge(namespace = "newsboat::configcontainer::bridged")]
mod bridged {
    #[namespace = "newsboat::filepath::bridged"]
    extern "C++" {
        include!("libnewsboat-ffi/src/filepath.rs.h");
        type PathBuf = crate::filepath::PathBuf;
    }

    struct CfgHandlerResult {
        status: u8,
        msg: String,
    }

    extern "Rust" {
        type ConfigContainer;

        fn create() -> Box<ConfigContainer>;

        fn handle_action(
            cc: &ConfigContainer,
            action: &str,
            params: Vec<String>,
        ) -> CfgHandlerResult;

        fn get_configvalue(cc: &ConfigContainer, key: &str) -> String;
        fn get_configvalue_as_int(cc: &ConfigContainer, key: &str) -> i32;
        fn get_configvalue_as_bool(cc: &ConfigContainer, key: &str) -> bool;
        fn get_configvalue_as_filepath(
            cc: &ConfigContainer,
            key: &str,
            path: Pin<&mut PathBuf>,
        ) -> bool;

        fn set_configvalue(
            cc: &ConfigContainer,
            key: &str,
            value: &str,
            error_msg: &mut String,
        ) -> bool;
        fn reset_to_default(cc: &ConfigContainer, key: &str);
        fn toggle(cc: &ConfigContainer, key: &str);
        fn dump_config(cc: &ConfigContainer) -> Vec<String>;
        fn get_suggestions(cc: &ConfigContainer, fragment: &str) -> Vec<String>;

        fn get_feed_sort_strategy_values(cc: &ConfigContainer) -> Vec<i32>;
        fn get_article_sort_strategy_values(cc: &ConfigContainer) -> Vec<i32>;
    }
}

fn create() -> Box<ConfigContainer> {
    Box::new(ConfigContainer(configcontainer::ConfigContainer::new()))
}

fn handle_action(
    cc: &ConfigContainer,
    action: &str,
    params: Vec<String>,
) -> bridged::CfgHandlerResult {
    match cc.0.handle_action(action, &params) {
        configcontainer::ConfigHandlerStatus::Success => bridged::CfgHandlerResult {
            status: 0,
            msg: String::new(),
        },
        configcontainer::ConfigHandlerStatus::InvalidCommand => bridged::CfgHandlerResult {
            status: 1,
            msg: String::new(),
        },
        configcontainer::ConfigHandlerStatus::TooFewParams => bridged::CfgHandlerResult {
            status: 2,
            msg: String::new(),
        },
        configcontainer::ConfigHandlerStatus::TooManyParams => bridged::CfgHandlerResult {
            status: 3,
            msg: String::new(),
        },
        configcontainer::ConfigHandlerStatus::InvalidParams(m) => {
            bridged::CfgHandlerResult { status: 4, msg: m }
        }
    }
}

fn get_configvalue(cc: &ConfigContainer, key: &str) -> String {
    cc.0.get_configvalue(key)
}

fn get_configvalue_as_int(cc: &ConfigContainer, key: &str) -> i32 {
    cc.0.get_configvalue_as_int(key)
}

fn get_configvalue_as_bool(cc: &ConfigContainer, key: &str) -> bool {
    cc.0.get_configvalue_as_bool(key)
}

fn get_configvalue_as_filepath(
    cc: &ConfigContainer,
    key: &str,
    mut path: Pin<&mut PathBuf>,
) -> bool {
    cc.0.get_configvalue_as_filepath(key, &mut path.get_mut().0)
}

fn set_configvalue(cc: &ConfigContainer, key: &str, value: &str, error_msg: &mut String) -> bool {
    match cc.0.set_configvalue(key, value) {
        Ok(_) => true,
        Err(e) => {
            *error_msg = e;
            false
        }
    }
}

fn reset_to_default(cc: &ConfigContainer, key: &str) {
    cc.0.reset_to_default(key);
}

fn toggle(cc: &ConfigContainer, key: &str) {
    cc.0.toggle(key);
}

fn dump_config(cc: &ConfigContainer) -> Vec<String> {
    cc.0.dump_config()
}

fn get_suggestions(cc: &ConfigContainer, fragment: &str) -> Vec<String> {
    cc.0.get_suggestions(fragment)
}

fn get_feed_sort_strategy_values(cc: &ConfigContainer) -> Vec<i32> {
    let (method, direction) = cc.0.get_feed_sort_strategy();

    let m = match method {
        configcontainer::FeedSortMethod::None => 0,
        configcontainer::FeedSortMethod::FirstTag => 1,
        configcontainer::FeedSortMethod::Title => 2,
        configcontainer::FeedSortMethod::ArticleCount => 3,
        configcontainer::FeedSortMethod::UnreadArticleCount => 4,
        configcontainer::FeedSortMethod::LastUpdated => 5,
        configcontainer::FeedSortMethod::LatestUnread => 6,
    };

    let d = match direction {
        configcontainer::SortDirection::Asc => 0,
        configcontainer::SortDirection::Desc => 1,
    };

    vec![m, d]
}

fn get_article_sort_strategy_values(cc: &ConfigContainer) -> Vec<i32> {
    let (method, direction) = cc.0.get_article_sort_strategy();

    let m = match method {
        configcontainer::ArtSortMethod::Title => 0,
        configcontainer::ArtSortMethod::Flags => 1,
        configcontainer::ArtSortMethod::Author => 2,
        configcontainer::ArtSortMethod::Link => 3,
        configcontainer::ArtSortMethod::Guid => 4,
        configcontainer::ArtSortMethod::Date => 5,
        configcontainer::ArtSortMethod::Random => 6,
    };

    let d = match direction {
        configcontainer::SortDirection::Asc => 0,
        configcontainer::SortDirection::Desc => 1,
    };

    vec![m, d]
}

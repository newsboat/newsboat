use gettextrs::gettext;
use std::collections::{BTreeMap, HashSet};
use std::sync::{Arc, Mutex};
use crate::utils;

#[derive(Clone, Debug, PartialEq)]
pub enum ConfigDataType {
    Bool,
    Int,
    Enum,
    Str,
    Path,
}

#[derive(Clone, Debug, PartialEq)]
pub enum FeedSortMethod {
    None,
    FirstTag,
    Title,
    ArticleCount,
    UnreadArticleCount,
    LastUpdated,
    LatestUnread,
}

#[derive(Clone, Debug, PartialEq)]
pub enum ArtSortMethod {
    Title,
    Flags,
    Author,
    Link,
    Guid,
    Date,
    Random,
}

#[derive(Clone, Debug, PartialEq)]
pub enum SortDirection {
    Asc,
    Desc,
}

#[derive(Clone, Debug, PartialEq)]
pub enum ConfigHandlerError {
    InvalidCommand,
    TooFewParams,
    TooManyParams,
    InvalidParams(String),
}

#[derive(Clone, Debug)]
pub struct ConfigData {
    pub value: String,
    pub default_value: String,
    pub data_type: ConfigDataType,
    pub enum_values: HashSet<String>,
    pub multi_option: bool,
}

impl ConfigData {
    pub fn new(val: &str, dtype: ConfigDataType) -> Self {
        ConfigData {
            value: val.to_string(),
            default_value: val.to_string(),
            data_type: dtype,
            enum_values: HashSet::new(),
            multi_option: false,
        }
    }

    pub fn new_multi(val: &str, dtype: ConfigDataType) -> Self {
        ConfigData {
            value: val.to_string(),
            default_value: val.to_string(),
            data_type: dtype,
            enum_values: HashSet::new(),
            multi_option: true,
        }
    }

    pub fn new_enum(val: &str, values: Vec<&str>) -> Self {
        ConfigData {
            value: val.to_string(),
            default_value: val.to_string(),
            data_type: ConfigDataType::Enum,
            enum_values: values.iter().map(|s| s.to_string()).collect(),
            multi_option: false,
        }
    }

    pub fn set_value(&mut self, new_val: String) -> Result<(), String> {
        match self.data_type {
            ConfigDataType::Bool => {
                if new_val == "true" || new_val == "yes" || new_val == "false" || new_val == "no" {
                    self.value = new_val;
                    Ok(())
                } else {
                    Err(gettext("invalid boolean value: %s").replace("%s", &new_val))
                }
            }
            ConfigDataType::Int => {
                if new_val.parse::<isize>().is_ok() {
                    self.value = new_val;
                    Ok(())
                } else {
                    Err(gettext("invalid integer value: %s").replace("%s", &new_val))
                }
            }
            ConfigDataType::Enum => {
                if self.enum_values.contains(&new_val) {
                    self.value = new_val;
                    Ok(())
                } else {
                    Err(gettext("invalid enum value: %s").replace("%s", &new_val))
                }
            }
            _ => {
                self.value = new_val;
                Ok(())
            }
        }
    }
}

pub struct ConfigContainer {
    config_data: Arc<Mutex<BTreeMap<String, ConfigData>>>,
}

impl Default for ConfigContainer {
    fn default() -> Self {
        Self::new()
    }
}

impl ConfigContainer {
    pub fn new() -> Self {
        let mut config_data = BTreeMap::new();

        config_data.insert(
            "always-display-description".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "article-sort-order".to_string(),
            ConfigData::new("date-asc", ConfigDataType::Str),
        );

        config_data.insert(
            "articlelist-format".to_string(),
            ConfigData::new("%4i %f %D %6L  %?T?|%-17T|  &?%t", ConfigDataType::Str),
        );

        config_data.insert(
            "auto-reload".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "bookmark-autopilot".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "bookmark-cmd".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "bookmark-interactive".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        let default_browser = crate::utils::get_default_browser();
        config_data.insert(
            "browser".to_string(),
            ConfigData::new(&default_browser.to_string_lossy(), ConfigDataType::Path),
        );

        config_data.insert(
            "cache-file".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "cleanup-on-quit".to_string(),
            ConfigData::new_enum("nudge", vec!["yes", "no", "nudge", "true", "false"]),
        );

        config_data.insert(
            "confirm-delete-all-articles".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "confirm-mark-all-feeds-read".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "confirm-mark-feed-read".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "confirm-exit".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "cookie-cache".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "datetime-format".to_string(),
            ConfigData::new("%b %d", ConfigDataType::Str),
        );

        config_data.insert(
            "delete-read-articles-on-quit".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "delete-played-files".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "display-article-progress".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "download-filename-format".to_string(),
            ConfigData::new("%?u?%u&%Y-%b-%d-%H%M%S.unknown?", ConfigDataType::Str),
        );

        config_data.insert(
            "download-full-page".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "download-path".to_string(),
            ConfigData::new("~/", ConfigDataType::Path),
        );

        config_data.insert(
            "download-retries".to_string(),
            ConfigData::new("1", ConfigDataType::Int),
        );

        config_data.insert(
            "download-timeout".to_string(),
            ConfigData::new("30", ConfigDataType::Int),
        );

        config_data.insert(
            "error-log".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "external-url-viewer".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "feed-sort-order".to_string(),
            ConfigData::new("none-desc", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-flag-share".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-min-items".to_string(),
            ConfigData::new("20", ConfigDataType::Int),
        );

        config_data.insert(
            "feedhq-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "feedhq-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedhq-show-special-feeds".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "feedhq-url".to_string(),
            ConfigData::new("https://feedhq.org/", ConfigDataType::Str),
        );

        config_data.insert(
            "feedbin-url".to_string(),
            ConfigData::new("https://api.feedbin.com", ConfigDataType::Str),
        );

        config_data.insert(
            "feedbin-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedbin-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedbin-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "feedbin-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedbin-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "freshrss-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "freshrss-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "freshrss-min-items".to_string(),
            ConfigData::new("20", ConfigDataType::Int),
        );

        config_data.insert(
            "freshrss-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "freshrss-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "freshrss-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "freshrss-show-special-feeds".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "freshrss-url".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "feedlist-format".to_string(),
            ConfigData::new("%4i %n %11u %t", ConfigDataType::Str),
        );

        config_data.insert(
            "goto-first-unread".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "goto-next-feed".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "history-limit".to_string(),
            ConfigData::new("100", ConfigDataType::Int),
        );

        config_data.insert(
            "html-renderer".to_string(),
            ConfigData::new("internal", ConfigDataType::Path),
        );

        config_data.insert(
            "http-auth-method".to_string(),
            ConfigData::new_enum(
                "any",
                vec![
                    "any",
                    "basic",
                    "digest",
                    "digest_ie",
                    "gssnegotiate",
                    "ntlm",
                    "anysafe",
                ],
            ),
        );

        config_data.insert(
            "ignore-mode".to_string(),
            ConfigData::new_enum("download", vec!["download", "display"]),
        );

        config_data.insert(
            "inoreader-app-id".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-app-key".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "inoreader-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-show-special-feeds".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "inoreader-flag-share".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "inoreader-min-items".to_string(),
            ConfigData::new("20", ConfigDataType::Int),
        );

        config_data.insert(
            "keep-articles-days".to_string(),
            ConfigData::new("0", ConfigDataType::Int),
        );

        config_data.insert(
            "mark-as-read-on-hover".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "max-browser-tabs".to_string(),
            ConfigData::new("10", ConfigDataType::Int),
        );

        config_data.insert(
            "markfeedread-jumps-to-next-unread".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "max-download-speed".to_string(),
            ConfigData::new("0", ConfigDataType::Int),
        );

        config_data.insert(
            "max-downloads".to_string(),
            ConfigData::new("1", ConfigDataType::Int),
        );

        config_data.insert(
            "max-items".to_string(),
            ConfigData::new("0", ConfigDataType::Int),
        );

        config_data.insert(
            "newsblur-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "newsblur-min-items".to_string(),
            ConfigData::new("20", ConfigDataType::Int),
        );

        config_data.insert(
            "newsblur-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "newsblur-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "newsblur-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "newsblur-url".to_string(),
            ConfigData::new("https://newsblur.com", ConfigDataType::Str),
        );

        config_data.insert(
            "notify-always".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "notify-beep".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "notify-format".to_string(),
            ConfigData::new(
                &gettext("Newsboat: finished reload, %f unread feeds (%n unread articles total)"),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "notify-program".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "notify-screen".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "notify-xterm".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "oldreader-flag-share".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "oldreader-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "oldreader-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "oldreader-min-items".to_string(),
            ConfigData::new("20", ConfigDataType::Int),
        );

        config_data.insert(
            "oldreader-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "oldreader-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "oldreader-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "oldreader-show-special-feeds".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "openbrowser-and-mark-jumps-to-next-unread".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "opml-url".to_string(),
            ConfigData::new_multi("", ConfigDataType::Str),
        );

        config_data.insert(
            "pager".to_string(),
            ConfigData::new("internal", ConfigDataType::Path),
        );

        config_data.insert(
            "player".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "podcast-auto-enqueue".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "podlist-format".to_string(),
            ConfigData::new(
                &gettext("%4i [%6dMB/%6tMB] [%5p %%] [%12K] %-20S %u -> %F"),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "prepopulate-query-feeds".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "ssl-verifyhost".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "ssl-verifypeer".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "proxy".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "proxy-auth".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "proxy-auth-method".to_string(),
            ConfigData::new_enum(
                "any",
                vec![
                    "any",
                    "basic",
                    "digest",
                    "digest_ie",
                    "gssnegotiate",
                    "ntlm",
                    "anysafe",
                ],
            ),
        );

        config_data.insert(
            "proxy-type".to_string(),
            ConfigData::new_enum(
                "http",
                vec!["http", "socks4", "socks4a", "socks5", "socks5h"],
            ),
        );

        config_data.insert(
            "refresh-on-startup".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "reload-only-visible-feeds".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "reload-threads".to_string(),
            ConfigData::new("1", ConfigDataType::Int),
        );

        config_data.insert(
            "reload-time".to_string(),
            ConfigData::new("60", ConfigDataType::Int),
        );

        config_data.insert(
            "restrict-filename".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "save-path".to_string(),
            ConfigData::new("~/", ConfigDataType::Path),
        );

        config_data.insert(
            "scrolloff".to_string(),
            ConfigData::new("0", ConfigDataType::Int),
        );

        config_data.insert(
            "search-highlight-colors".to_string(),
            ConfigData::new_multi("black yellow bold", ConfigDataType::Str),
        );

        config_data.insert(
            "selecttag-format".to_string(),
            ConfigData::new("%4i  %T (%u)", ConfigDataType::Str),
        );

        config_data.insert(
            "show-keymap-hint".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "show-title-bar".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "show-read-articles".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "show-read-feeds".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "suppress-first-reload".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "swap-title-and-hints".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "text-width".to_string(),
            ConfigData::new("0", ConfigDataType::Int),
        );

        config_data.insert(
            "toggleitemread-jumps-to-next".to_string(),
            ConfigData::new("true", ConfigDataType::Bool),
        );

        config_data.insert(
            "toggleitemread-jumps-to-next-unread".to_string(),
            ConfigData::new("false", ConfigDataType::Bool),
        );

        config_data.insert(
            "ttrss-flag-publish".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ttrss-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ttrss-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ttrss-mode".to_string(),
            ConfigData::new_enum("multi", vec!["single", "multi"]),
        );

        config_data.insert(
            "ttrss-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ttrss-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "ttrss-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ttrss-url".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ocnews-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ocnews-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ocnews-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "ocnews-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ocnews-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "ocnews-url".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-flag-star".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-login".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-min-items".to_string(),
            ConfigData::new("100", ConfigDataType::Int),
        );

        config_data.insert(
            "miniflux-password".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-passwordfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "miniflux-passwordeval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-show-special-feeds".to_string(),
            ConfigData::new("yes", ConfigDataType::Bool),
        );

        config_data.insert(
            "miniflux-token".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-tokenfile".to_string(),
            ConfigData::new("", ConfigDataType::Path),
        );

        config_data.insert(
            "miniflux-tokeneval".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "miniflux-url".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert(
            "urls-source".to_string(),
            ConfigData::new_enum(
                "local",
                vec![
                    "local",
                    "opml",
                    "oldreader",
                    "ttrss",
                    "newsblur",
                    "feedhq",
                    "feedbin",
                    "freshrss",
                    "ocnews",
                    "miniflux",
                    "inoreader",
                ],
            ),
        );

        config_data.insert(
            "use-proxy".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        config_data.insert(
            "user-agent".to_string(),
            ConfigData::new("", ConfigDataType::Str),
        );

        config_data.insert("articlelist-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - Articles in feed '%T' (%u unread, %t total)%?F? matching filter '%F'&? - %U"), ConfigDataType::Str));

        config_data.insert(
            "dialogs-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - Dialogs"), ConfigDataType::Str),
        );

        config_data.insert("feedlist-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - %?F?Feeds&Your feeds? (%u unread, %t total)%?F? matching filter '%F'&?%?T? - tag '%T'&?"), ConfigDataType::Str));

        config_data.insert(
            "filebrowser-title-format".to_string(),
            ConfigData::new(
                &gettext("%N %V - %?O?Open File&Save File? - %f"),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "dirbrowser-title-format".to_string(),
            ConfigData::new(
                &gettext("%N %V - %?O?Open Directory&Save File? - %f"),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "help-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - Help"), ConfigDataType::Str),
        );

        config_data.insert(
            "itemview-title-format".to_string(),
            ConfigData::new(
                &gettext("%N %V - Article '%T' (%u unread, %t total)"),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "searchresult-title-format".to_string(),
            ConfigData::new(
                &gettext(
                    "%N %V - Search results for '%s' (%u unread, %t total)%?F? matching filter '%F'&?",
                ),
                ConfigDataType::Str,
            ),
        );

        config_data.insert(
            "selectfilter-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - Select Filter"), ConfigDataType::Str),
        );

        config_data.insert(
            "selecttag-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - Select Tag"), ConfigDataType::Str),
        );

        config_data.insert(
            "urlview-title-format".to_string(),
            ConfigData::new(&gettext("%N %V - URLs"), ConfigDataType::Str),
        );

        config_data.insert(
            "wrap-scroll".to_string(),
            ConfigData::new("no", ConfigDataType::Bool),
        );

        ConfigContainer {
            config_data: Arc::new(Mutex::new(config_data)),
        }
    }

    pub fn handle_action(&self, action: &str, params: &[String]) -> Result<(), ConfigHandlerError> {
        let mut data = self.config_data.lock().unwrap();
        if let Some(entry) = data.get_mut(action) {
            if params.is_empty() {
                return Err(ConfigHandlerError::TooFewParams);
            }

            if !entry.multi_option && params.len() > 1 {
                return Err(ConfigHandlerError::TooManyParams);
            }

            let value = if entry.multi_option {
                params.join(" ")
            } else {
                params[0].clone()
            };

            match entry.set_value(value) {
                Ok(_) => Ok(()),
                Err(msg) => Err(ConfigHandlerError::InvalidParams(msg)),
            }
        } else {
            Err(ConfigHandlerError::InvalidCommand)
        }
    }

    pub fn get_configvalue_as_int(&self, key: &str) -> i32 {
        self.get_configvalue(key).parse().unwrap_or(0)
    }

    pub fn get_configvalue_as_bool(&self, key: &str) -> bool {
        let val = self.get_configvalue(key);
        val == "true" || val == "yes"
    }

    pub fn get_configvalue_as_filepath(&self, key: &str) -> Box<std::path::PathBuf> {
        Box::new(utils::resolve_tilde(self.get_configvalue(key).into()))
    }

    pub fn get_configvalue(&self, key: &str) -> String {
        let data = self.config_data.lock().unwrap();
        match data.get(key) {
            Some(entry) => entry.value.clone(),
            None => String::new(),
        }
    }

    pub fn set_configvalue(&self, key: &str, value: &str) -> Result<(), String> {
        let mut data = self.config_data.lock().unwrap();
        match data.get_mut(key) {
            Some(entry) => entry.set_value(value.to_string()),
            None => Err(gettext("unknown config option: %s").replace("%s", key)),
        }
    }

    pub fn reset_to_default(&self, key: &str) {
        let mut data = self.config_data.lock().unwrap();
        if let Some(entry) = data.get_mut(key) {
            entry.value = entry.default_value.clone();
        }
    }

    pub fn toggle(&self, key: &str) {
        let mut data = self.config_data.lock().unwrap();
        if let Some(entry) = data.get_mut(key)
            && let ConfigDataType::Bool = entry.data_type
        {
            let current = entry.value == "true" || entry.value == "yes";
            entry.value = if current {
                "false".to_string()
            } else {
                "true".to_string()
            };
        }
    }

    pub fn dump_config(&self) -> Vec<String> {
        let data = self.config_data.lock().unwrap();
        let mut output = Vec::new();

        for (key, entry) in data.iter() {
            let formatted_value = match entry.data_type {
                ConfigDataType::Bool | ConfigDataType::Int => entry.value.clone(),
                _ => format!("\"{}\"", entry.value.replace("\"", "\\\"")),
            };

            let mut line = format!("{key} {formatted_value}");
            if entry.value != entry.default_value {
                line.push_str(&format!(" # default: {}", entry.default_value));
            }
            output.push(line);
        }
        output
    }

    pub fn get_suggestions(&self, fragment: &str) -> Vec<String> {
        let data = self.config_data.lock().unwrap();
        let mut result: Vec<String> = data
            .keys()
            .filter(|k| k.starts_with(fragment))
            .cloned()
            .collect();
        result.sort();
        result
    }

    pub fn get_feed_sort_strategy(&self) -> (FeedSortMethod, SortDirection) {
        let val = self.get_configvalue("feed-sort-order");
        let parts: Vec<&str> = val.split('-').collect();

        let method = match parts.first().copied().unwrap_or("") {
            "none" => FeedSortMethod::None,
            "firsttag" => FeedSortMethod::FirstTag,
            "title" => FeedSortMethod::Title,
            "articlecount" => FeedSortMethod::ArticleCount,
            "unreadarticlecount" => FeedSortMethod::UnreadArticleCount,
            "lastupdated" => FeedSortMethod::LastUpdated,
            "latestunread" => FeedSortMethod::LatestUnread,
            _ => FeedSortMethod::None,
        };

        let direction = match parts.get(1).copied().unwrap_or("desc") {
            "asc" => SortDirection::Asc,
            "desc" => SortDirection::Desc,
            _ => SortDirection::Desc,
        };

        (method, direction)
    }

    pub fn get_article_sort_strategy(&self) -> (ArtSortMethod, SortDirection) {
        let val = self.get_configvalue("article-sort-order");
        let parts: Vec<&str> = val.split('-').collect();

        let method_str = parts.first().copied().unwrap_or("date");

        let mut direction = SortDirection::Asc;
        let mut method = ArtSortMethod::Date;

        if method_str == "date" {
            direction = SortDirection::Desc;
            if parts.get(1).copied() == Some("asc") {
                direction = SortDirection::Asc;
            }
        } else if parts.get(1).copied() == Some("desc") {
            direction = SortDirection::Desc;
        }

        match method_str {
            "title" => method = ArtSortMethod::Title,
            "flags" => method = ArtSortMethod::Flags,
            "author" => method = ArtSortMethod::Author,
            "link" => method = ArtSortMethod::Link,
            "guid" => method = ArtSortMethod::Guid,
            "date" => method = ArtSortMethod::Date,
            "random" => method = ArtSortMethod::Random,
            _ => {}
        }

        (method, direction)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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

        assert_eq!(result, Ok(()));
        assert_eq!(cfg.get_configvalue("search-highlight-colors"), "red blue");

        // "browser" is a single-option config
        // It should reject multiple parameters
        let result = cfg.handle_action("browser", &["firefox".to_string(), "%u".to_string()]);
        assert_eq!(result, Err(ConfigHandlerError::TooManyParams));
    }

    #[test]
    fn t_handle_action_edge_cases() {
        let cfg = ConfigContainer::new();

        // Invalid command
        assert_eq!(
            cfg.handle_action("non-existent-command", &["val".to_string()]),
            Err(ConfigHandlerError::InvalidCommand)
        );

        // Too few parameters (empty)
        assert_eq!(
            cfg.handle_action("browser", &[]),
            Err(ConfigHandlerError::TooFewParams)
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
}

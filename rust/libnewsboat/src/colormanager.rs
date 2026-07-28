use std::collections::HashMap;
use std::sync::LazyLock;

use gettextrs::gettext;
use strprintf::fmt;

use crate::configparser::ActionHandlerStatus;
use crate::{
    logger::{self, Level},
    textstyle::TextStyle,
};

static DEFAULT_STYLES: LazyLock<HashMap<&str, TextStyle>> = LazyLock::new(|| {
    // Unwrapping `TextStyle::from` results in this function should be fine because the inputs are
    // constant and all of these are hit anytime ColorManager is used.
    // This makes sure tests should find all issues even if not all defaults are explicitly tested.
    HashMap::from([
        (
            "listnormal",
            TextStyle::from("default", "default", &[]).unwrap(),
        ),
        (
            "listfocus",
            TextStyle::from("yellow", "blue", &["bold"]).unwrap(),
        ),
        (
            "listnormal_unread",
            TextStyle::from("default", "default", &["bold"]).unwrap(),
        ),
        (
            "listfocus_unread",
            TextStyle::from("yellow", "blue", &["bold"]).unwrap(),
        ),
        (
            "info",
            TextStyle::from("yellow", "blue", &["bold"]).unwrap(),
        ),
        (
            "background",
            TextStyle::from("default", "default", &[]).unwrap(),
        ),
        (
            "article",
            TextStyle::from("default", "default", &[]).unwrap(),
        ),
        (
            "end-of-text-marker",
            TextStyle::from("blue", "default", &["bold"]).unwrap(),
        ),
        (
            "title",
            TextStyle::from("yellow", "blue", &["bold"]).unwrap(),
        ),
        (
            "hint-key",
            TextStyle::from("yellow", "blue", &["bold"]).unwrap(),
        ),
        (
            "hint-keys-delimiter",
            TextStyle::from("white", "blue", &[]).unwrap(),
        ),
        (
            "hint-separator",
            TextStyle::from("white", "blue", &["bold"]).unwrap(),
        ),
        (
            "hint-description",
            TextStyle::from("white", "blue", &[]).unwrap(),
        ),
    ])
});

#[derive(Default)]
pub struct ColorManager {
    element_styles: HashMap<String, TextStyle>,
}

impl ColorManager {
    pub fn handle_action(
        &mut self,
        action: &str,
        params: &[&str],
    ) -> Result<(), ActionHandlerStatus> {
        match action {
            "color" => match params {
                [element, fgcolor, bgcolor, attributes @ ..] => {
                    if !DEFAULT_STYLES.contains_key(element) {
                        return Err(ActionHandlerStatus::CustomErrorMessage(fmt!(
                            &gettext("`%s' is not a valid configuration element"),
                            action
                        )));
                    }

                    let text_style = TextStyle::from(fgcolor, bgcolor, attributes)?;

                    self.element_styles.insert(element.to_string(), text_style);

                    Ok(())
                }
                [..] => Err(ActionHandlerStatus::TooFewParameters),
            },
            _ => Err(ActionHandlerStatus::InvalidCommand),
        }
    }

    pub fn dump_config(&self) -> Vec<String> {
        self.element_styles
            .iter()
            .map(|(element, style)| format!("color {} {}", element, style.get_config_string()))
            .collect()
    }

    fn style_fallback(element: &str) -> Option<&str> {
        match element {
            "title" => Some("info"),
            "hint-key" => Some("info"),
            "hint-keys-delimiter" => Some("info"),
            "hint-separator" => Some("info"),
            "hint-description" => Some("info"),
            _ => None,
        }
    }

    pub fn get_stfl_styles(&self) -> HashMap<String, String> {
        let mut stfl_styles = HashMap::new();

        for (&element, default_style) in &*DEFAULT_STYLES {
            let style = if let Some(style) = self.element_styles.get(element) {
                style
            } else if let Some(fallback_element) = Self::style_fallback(element)
                && let Some(fallback_style) = self.element_styles.get(fallback_element)
            {
                fallback_style
            } else {
                default_style
            };

            let stfl_style_string = style.get_stfl_style_string();

            log!(
                Level::Debug,
                &format!(
                    "ColorManager::apply_colors: {} {}",
                    element, stfl_style_string
                )
            );

            stfl_styles.insert(element.to_string(), stfl_style_string.clone());

            if element == "article" {
                let mut bold = stfl_style_string.clone();
                let mut underline = stfl_style_string.clone();
                if !bold.is_empty() {
                    bold.push(',');
                }
                if !underline.is_empty() {
                    underline.push(',');
                }
                bold.push_str("attr=bold");
                underline.push_str("attr=underline");

                // STFL will just ignore those in forms which don't have the
                // `color_bold` and `color_underline` variables.
                log!(
                    Level::Debug,
                    &format!("ColorManager::apply_colors: color_bold {}", bold)
                );
                stfl_styles.insert("color_bold".to_string(), bold);
                log!(
                    Level::Debug,
                    &format!("ColorManager::apply_colors: color_underline {}", underline)
                );
                stfl_styles.insert("color_underline".to_string(), underline);
            }
        }

        stfl_styles
    }
}

#[cfg(test)]
mod tests {
    use crate::configparser::ActionHandlerStatus;

    use super::ColorManager;

    #[test]
    fn t_colormanager_handle_action_unknown_command() {
        let mut colormanager = ColorManager::default();
        assert_eq!(
            colormanager.handle_action("notaconfigcommand", &[]),
            Err(ActionHandlerStatus::InvalidCommand)
        );
    }

    #[test]
    fn t_colormanager_handle_action_too_few_parameters() {
        let mut colormanager = ColorManager::default();
        assert_eq!(
            colormanager.handle_action("color", &[]),
            Err(ActionHandlerStatus::TooFewParameters)
        );

        assert_eq!(
            colormanager.handle_action("color", &["background"]),
            Err(ActionHandlerStatus::TooFewParameters)
        );

        assert_eq!(
            colormanager.handle_action("color", &["background", "red"]),
            Err(ActionHandlerStatus::TooFewParameters)
        );
    }

    #[test]
    fn t_colormanager_handle_action_invalid_element_name() {
        let mut colormanager = ColorManager::default();

        assert!(matches!(
            colormanager.handle_action("color", &["notanelement", "red", "blue"]),
            Err(ActionHandlerStatus::CustomErrorMessage(_))
        ));
    }

    #[test]
    fn t_colormanager_handle_action_invalid_style() {
        let mut colormanager = ColorManager::default();

        assert!(matches!(
            colormanager.handle_action("color", &["background", "notacolor", "blue"]),
            Err(ActionHandlerStatus::CustomErrorMessage(_))
        ));

        assert!(matches!(
            colormanager.handle_action("color", &["background", "red", "notacolor"]),
            Err(ActionHandlerStatus::CustomErrorMessage(_))
        ));

        assert!(matches!(
            colormanager.handle_action("color", &["background", "red", "blue", "notanattribute"]),
            Err(ActionHandlerStatus::CustomErrorMessage(_))
        ));

        assert!(matches!(
            colormanager.handle_action(
                "color",
                &["background", "red", "blue", "bold", "notanattribute"]
            ),
            Err(ActionHandlerStatus::CustomErrorMessage(_))
        ));
    }

    #[test]
    fn t_colormanager_dump_config() {
        let mut colormanager = ColorManager::default();

        assert!(colormanager.dump_config().is_empty());

        colormanager
            .handle_action("color", &["info", "red", "blue", "underline", "bold"])
            .unwrap();

        let dump = colormanager.dump_config();
        assert_eq!(dump.len(), 1);
        assert_eq!(dump.first().unwrap(), "color info red blue underline bold");

        colormanager
            .handle_action("color", &["info", "white", "black"])
            .unwrap();

        let dump = colormanager.dump_config();
        assert_eq!(dump.len(), 1);
        assert_eq!(dump.first().unwrap(), "color info white black");
    }

    #[test]
    fn t_colormanager_get_stfl_styles_fallback() {
        let mut colormanager = ColorManager::default();

        let element = "hint-keys-delimiter";

        // Element has its own default
        let styles = colormanager.get_stfl_styles();
        let style = styles.get(element).unwrap();
        assert_eq!(style, "fg=white,bg=blue");

        // When "info" is set explicitly, element takes over "info" style as fallback
        colormanager
            .handle_action("color", &["info", "red", "black", "underline"])
            .unwrap();
        let styles = colormanager.get_stfl_styles();
        let style = styles.get(element).unwrap();
        assert_eq!(style, "fg=red,bg=black,attr=underline");

        // When element is set explicitly, fallback to "info" is not used
        colormanager
            .handle_action("color", &[element, "yellow", "default"])
            .unwrap();
        let styles = colormanager.get_stfl_styles();
        let style = styles.get(element).unwrap();
        assert_eq!(style, "fg=yellow");
    }

    #[test]
    fn t_colormanager_get_stfl_styles_article_has_extra_bold_underline() {
        let mut colormanager = ColorManager::default();

        colormanager
            .handle_action("color", &["article", "red", "blue"])
            .unwrap();
        let styles = colormanager.get_stfl_styles();
        assert_eq!(styles.get("article").unwrap(), "fg=red,bg=blue");
        assert_eq!(
            styles.get("color_bold").unwrap(),
            "fg=red,bg=blue,attr=bold"
        );
        assert_eq!(
            styles.get("color_underline").unwrap(),
            "fg=red,bg=blue,attr=underline"
        );
    }
}

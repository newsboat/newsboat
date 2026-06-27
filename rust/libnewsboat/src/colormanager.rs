use std::collections::HashMap;
use std::sync::LazyLock;

use gettextrs::gettext;
use strprintf::fmt;

use crate::logger::{self, Level};
use crate::utils;

#[derive(Debug)]
pub enum ActionHandlerStatus {
    InvalidCommand,
    TooFewParameters,
    CustomErrorMessage(String),
}

struct TextStyle {
    fgcolor: String,
    bgcolor: String,
    attributes: Vec<String>,
}

impl TextStyle {
    pub fn from(
        fgcolor: &str,
        bgcolor: &str,
        attributes: &[&str],
    ) -> Result<Self, ActionHandlerStatus> {
        let fgcolor = if utils::is_valid_color(fgcolor) {
            fgcolor.to_string()
        } else {
            return Err(ActionHandlerStatus::CustomErrorMessage(fmt!(
                &gettext("`%s' is not a valid color"),
                fgcolor
            )));
        };

        let bgcolor = if utils::is_valid_color(bgcolor) {
            bgcolor.to_string()
        } else {
            return Err(ActionHandlerStatus::CustomErrorMessage(fmt!(
                &gettext("`%s' is not a valid color"),
                bgcolor
            )));
        };

        let attributes = attributes
            .iter()
            .map(|attribute| {
                if utils::is_valid_attribute(attribute) {
                    Ok(attribute.to_string())
                } else {
                    Err(ActionHandlerStatus::CustomErrorMessage(fmt!(
                        &gettext("`%s' is not a valid attribute"),
                        *attribute
                    )))
                }
            })
            .collect::<Result<Vec<_>, _>>()?;

        Ok(Self {
            fgcolor,
            bgcolor,
            attributes,
        })
    }

    pub fn get_stfl_style_string(&self) -> String {
        let mut result = String::new();

        if self.fgcolor != "default" {
            result.push_str("fg=");
            result.push_str(&self.fgcolor);
        }

        if self.bgcolor != "default" {
            if !result.is_empty() {
                result.push(',');
            }
            result.push_str("bg=");
            result.push_str(&self.bgcolor);
        }

        for attribute in &self.attributes {
            if attribute != "default" {
                if !result.is_empty() {
                    result.push(',');
                }
                result.push_str("attr=");
                result.push_str(attribute);
            }
        }

        result
    }
}

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
            .map(|(element, style)| {
                let mut attributes_str = String::new();
                for attribute in &style.attributes {
                    attributes_str.push(' ');
                    attributes_str.push_str(attribute);
                }
                format!(
                    "color {} {} {}{}",
                    element, style.fgcolor, style.bgcolor, attributes_str
                )
            })
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

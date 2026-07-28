use crate::configparser::ActionHandlerStatus;
use crate::utils;

use gettextrs::gettext;
use strprintf::fmt;

pub struct TextStyle {
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

    /// Returnst a string containing the foreground color, background color and attributes;
    /// all separated by spaces.
    pub fn get_config_string(&self) -> String {
        let mut attributes_str = String::new();
        for attribute in &self.attributes {
            attributes_str.push(' ');
            attributes_str.push_str(attribute);
        }

        format!("{} {}{}", self.fgcolor, self.bgcolor, attributes_str)
    }
}

#[cfg(test)]
mod tests {
    use super::TextStyle;

    #[test]
    fn t_text_style_from_valid_style() {
        assert!(TextStyle::from("default", "default", &["bold", "underline"]).is_ok());
        assert!(TextStyle::from("default", "default", &["bold"]).is_ok());
        assert!(TextStyle::from("default", "default", &[]).is_ok());
        assert!(TextStyle::from("blue", "yellow", &[]).is_ok());
    }

    #[test]
    fn t_text_style_from_invalid_foreground_color() {
        assert!(TextStyle::from("notacolor", "yellow", &[]).is_err());
    }

    #[test]
    fn t_text_style_from_invalid_background_color() {
        assert!(TextStyle::from("blue", "notacolor", &[]).is_err());
    }

    #[test]
    fn t_text_style_from_invalid_attribute() {
        assert!(TextStyle::from("default", "default", &["notanattribute"]).is_err());
    }

    #[test]
    fn t_text_style_get_stfl_style_string() {
        assert_eq!(
            TextStyle::from("default", "default", &[])
                .unwrap()
                .get_stfl_style_string(),
            ""
        );

        assert_eq!(
            TextStyle::from("yellow", "default", &[])
                .unwrap()
                .get_stfl_style_string(),
            "fg=yellow"
        );

        assert_eq!(
            TextStyle::from("default", "blue", &[])
                .unwrap()
                .get_stfl_style_string(),
            "bg=blue"
        );

        assert_eq!(
            TextStyle::from("default", "default", &["bold", "underline"])
                .unwrap()
                .get_stfl_style_string(),
            "attr=bold,attr=underline"
        );

        assert_eq!(
            TextStyle::from("red", "black", &["bold", "underline"])
                .unwrap()
                .get_stfl_style_string(),
            "fg=red,bg=black,attr=bold,attr=underline"
        );
    }

    #[test]
    fn t_text_style_get_config_string() {
        assert_eq!(
            TextStyle::from("default", "default", &[])
                .unwrap()
                .get_config_string(),
            "default default"
        );

        assert_eq!(
            TextStyle::from("red", "blue", &[])
                .unwrap()
                .get_config_string(),
            "red blue"
        );

        assert_eq!(
            TextStyle::from("red", "default", &["bold"])
                .unwrap()
                .get_config_string(),
            "red default bold"
        );

        assert_eq!(
            TextStyle::from("red", "default", &["bold", "underline"])
                .unwrap()
                .get_config_string(),
            "red default bold underline"
        );
    }
}

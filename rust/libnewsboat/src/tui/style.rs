#[derive(Eq, Hash, PartialEq)]
pub enum StyleIdentifier {
    Reset, // Or None/Default?
    Info,
    Title,
    HintDescription,
    HintSeparator,
    HintKeysDelimiter,
    HintKey,
    ListNormal,
    ListFocus,
    Numbered(u16),
}

impl StyleIdentifier {
    pub fn from_set_var(key: &str) -> Option<Self> {
        match key {
            "info" => Some(Self::Info),
            "title" => Some(Self::Title),
            "hint-description" => Some(Self::HintDescription),
            "hint-separator" => Some(Self::HintSeparator),
            "hint-keys-delimiter" => Some(Self::HintKeysDelimiter),
            "hint-key" => Some(Self::HintKey),
            "listnormal" => Some(Self::ListNormal),
            "listfocus" => Some(Self::ListFocus),
            // TODO: Check for missing keys, e.g.:
            // "color_underline"
            // "color_bold"
            // "listnormal_unread"
            // "listfocus_unread"
            // "article"
            _ => None,
        }
    }

    pub fn from_list_replace(name: &str) -> Option<Self> {
        // TODO: Handle all list replace names
        Self::numbered_name(name)
    }

    fn numbered_name(name: &str) -> Option<Self> {
        let name = name.strip_prefix("style_")?;
        // TODO: Handle "_focus"
        let name = name.strip_suffix("_normal")?;
        name.parse::<u16>().ok().map(Self::Numbered)
    }

    pub fn from_stye_tag(tag: &str) -> Option<Self> {
        match tag {
            "</>" => Some(Self::Reset),
            "<key>" => Some(Self::HintKey),
            "<colon>" => Some(Self::HintSeparator),
            "<comma>" => Some(Self::HintKeysDelimiter),
            "<desc>" => Some(Self::HintDescription),
            // TODO: Add missing tags
            _ => None,
        }
        .or_else(|| Self::numbered_tag(tag))
    }

    fn numbered_tag(tag: &str) -> Option<Self> {
        let tag = tag.strip_prefix("<")?;
        let tag = tag.strip_suffix(">")?;
        tag.parse::<u16>().ok().map(Self::Numbered)
    }
}

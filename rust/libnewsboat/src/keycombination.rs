use nom::{
    branch::alt,
    bytes::complete::tag,
    character::{complete::anychar, is_alphabetic},
    combinator::{eof, verify},
    IResult,
};

#[derive(Debug, PartialEq)]
pub enum ShiftState {
    Shift,
    NoShift,
}

#[derive(Debug, PartialEq)]
pub enum ControlState {
    Control,
    NoControl,
}

#[derive(Debug, PartialEq)]
pub enum AltState {
    Alt,
    NoAlt,
}

#[derive(Debug, PartialEq)]
pub struct KeyCombination {
    key: String,
    shift: ShiftState,
    control: ControlState,
    alt: AltState,
}

impl KeyCombination {
    pub fn new(key: String) -> Self {
        Self {
            key,
            shift: ShiftState::NoShift,
            control: ControlState::NoControl,
            alt: AltState::NoAlt,
        }
    }

    pub fn with_shift(mut self) -> Self {
        self.shift = ShiftState::Shift;
        self
    }

    pub fn with_control(mut self) -> Self {
        self.control = ControlState::Control;
        self
    }

    pub fn with_alt(mut self) -> Self {
        self.alt = AltState::Alt;
        self
    }

    pub fn get_key(&self) -> &str {
        &self.key
    }

    pub fn has_shift(&self) -> bool {
        self.shift == ShiftState::Shift
    }

    pub fn has_control(&self) -> bool {
        self.control == ControlState::Control
    }

    pub fn has_alt(&self) -> bool {
        self.alt == AltState::Alt
    }
}

fn alphabetic(input: &str) -> IResult<&str, char> {
    verify(anychar, |c: &char| is_alphabetic(*c as u8))(input)
}

fn regular_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    Ok(("", KeyCombination::new(input.to_owned())))
}

fn control_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    let (input, _) = tag("^")(input)?;
    let (input, key) = alphabetic(input)?;
    eof(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_control();
    Ok((input, key_combination))
}

fn shift_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    let (input, key) = verify(alphabetic, |c: &char| c.is_uppercase())(input)?;
    eof(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_shift();
    Ok((input, key_combination))
}

pub fn bindkey(input: &str) -> KeyCombination {
    let result = alt((shift_bindkey, control_bindkey, regular_bindkey))(input);
    // Should be save to unwrap because `regular_bindkey` accepts any input
    let (_, key_combination) = result.unwrap();
    key_combination
}

#[cfg(test)]
mod tests {
    use super::bindkey;
    use super::KeyCombination;

    #[test]
    fn t_bindkey_no_modifiers() {
        assert_eq!(bindkey("a"), KeyCombination::new("a".to_owned()));
        assert_eq!(bindkey("ENTER"), KeyCombination::new("ENTER".to_owned()));
        assert_eq!(bindkey("^"), KeyCombination::new("^".to_owned()));
        assert_eq!(bindkey("<"), KeyCombination::new("<".to_owned()));
        assert_eq!(bindkey(">"), KeyCombination::new(">".to_owned()));
    }

    #[test]
    fn t_bindkey_with_shift() {
        assert_eq!(
            bindkey("A"),
            KeyCombination::new("a".to_owned()).with_shift()
        );
        assert_eq!(
            bindkey("Z"),
            KeyCombination::new("z".to_owned()).with_shift()
        );
    }

    #[test]
    fn t_bindkey_with_control() {
        assert_eq!(
            bindkey("^A"),
            KeyCombination::new("a".to_owned()).with_control()
        );
        assert_eq!(
            bindkey("^Z"),
            KeyCombination::new("z".to_owned()).with_control()
        );

        assert_eq!(
            bindkey("^a"),
            KeyCombination::new("a".to_owned()).with_control()
        );
        assert_eq!(
            bindkey("^z"),
            KeyCombination::new("z".to_owned()).with_control()
        );
    }
}

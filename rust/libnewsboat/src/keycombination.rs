use nom::AsChar;
use nom::Parser;
use nom::{
    IResult,
    branch::alt,
    bytes::complete::tag,
    character::complete::{anychar, none_of},
    combinator::{eof, recognize, verify},
    multi::{many0, many1},
    sequence::terminated,
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
    verify(anychar, |&c| c.is_ascii() && c.is_alpha()).parse(input)
}

fn regular_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    let key_combination = match input {
        "<" => KeyCombination::new("LT".to_owned()),
        ">" => KeyCombination::new("GT".to_owned()),
        key => KeyCombination::new(key.to_owned()),
    };
    Ok(("", key_combination))
}

fn control_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    let (input, _) = tag("^")(input)?;
    let (input, key) = alphabetic(input)?;
    eof(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_control();
    Ok((input, key_combination))
}

fn shift_bindkey(input: &str) -> IResult<&str, KeyCombination> {
    let (input, key) = verify(alphabetic, |c: &char| c.is_uppercase()).parse(input)?;
    eof(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_shift();
    Ok((input, key_combination))
}

pub fn bindkey(input: &str) -> KeyCombination {
    let result = alt((shift_bindkey, control_bindkey, regular_bindkey)).parse(input);
    // Should be save to unwrap because `regular_bindkey` accepts any input
    let (_, key_combination) = result.unwrap();
    key_combination
}

fn control_key_bind(input: &str) -> IResult<&str, KeyCombination> {
    let (input, _) = tag("^")(input)?;
    let (input, key) = alphabetic(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_control();
    Ok((input, key_combination))
}

fn shift_key_bind(input: &str) -> IResult<&str, KeyCombination> {
    let (input, key) = verify(alphabetic, |c: &char| c.is_uppercase()).parse(input)?;

    let key_combination = KeyCombination::new(key.to_lowercase().to_string()).with_shift();
    Ok((input, key_combination))
}

fn combination_key_bind(input: &str) -> IResult<&str, KeyCombination> {
    let (input, _) = tag("<")(input)?;
    let (input, modifiers) = many0(alt((tag("C-"), tag("S-"), tag("M-")))).parse(input)?;
    let (input, key) = recognize(many1(none_of(">"))).parse(input)?;
    let (input, _) = tag(">")(input)?;

    let mut key_combination = KeyCombination::new(key.to_owned());

    for modifier in modifiers {
        match modifier {
            "C-" => key_combination = key_combination.with_control(),
            "S-" => key_combination = key_combination.with_shift(),
            "M-" => key_combination = key_combination.with_alt(),
            _ => (),
        }
    }

    Ok((input, key_combination))
}

fn single_key_bind(input: &str) -> IResult<&str, KeyCombination> {
    let (input, key) = anychar(input)?;
    let mut key_combination = KeyCombination::new(key.to_string());

    key_combination.key = match key_combination.key.as_str() {
        "<" => String::from("LT"),
        ">" => String::from("GT"),
        _ => key_combination.key,
    };

    Ok((input, key_combination))
}

pub fn bind(input: &str) -> Vec<KeyCombination> {
    let result = terminated(
        many0(alt((
            control_key_bind,
            shift_key_bind,
            combination_key_bind,
            single_key_bind,
        ))),
        eof,
    )
    .parse(input);
    // Should be save to unwrap because `single_key_bind` accepts any input
    let (_, key_combinations) = result.unwrap();

    key_combinations
}

#[cfg(test)]
mod tests {
    use super::KeyCombination;
    use super::bind;
    use super::bindkey;

    proptest::proptest! {
        #[test]
        fn t_bindkey_does_not_crash_on_any_input(ref input in "\\PC*") {
            // Result explicitly ignored because we just want to make sure this call doesn't panic.
            let _ = bindkey(input);
        }
        #[test]
        fn t_bind_does_not_crash_on_any_input(ref input in "\\PC*") {
            // Result explicitly ignored because we just want to make sure this call doesn't panic.
            let _ = bind(input);
        }
    }

    #[test]
    fn t_bindkey_no_modifiers() {
        assert_eq!(bindkey("a"), KeyCombination::new("a".to_owned()));
        assert_eq!(bindkey("ENTER"), KeyCombination::new("ENTER".to_owned()));
        assert_eq!(bindkey("^"), KeyCombination::new("^".to_owned()));
        assert_eq!(bindkey("<"), KeyCombination::new("LT".to_owned()));
        assert_eq!(bindkey(">"), KeyCombination::new("GT".to_owned()));
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

    #[test]
    fn t_bindkey_single_special_key() {
        assert_eq!(bindkey("="), KeyCombination::new("=".to_owned()));
        assert_eq!(bindkey("<"), KeyCombination::new("LT".to_owned()));
        assert_eq!(bindkey(">"), KeyCombination::new("GT".to_owned()));
        assert_eq!(bindkey("-"), KeyCombination::new("-".to_owned()));
    }

    #[test]
    fn t_bind_single_regular_key() {
        assert_eq!(bind("a"), vec![KeyCombination::new("a".to_owned())]);
        assert_eq!(
            bind("A"),
            vec![KeyCombination::new("a".to_owned()).with_shift()]
        );
        assert_eq!(
            bind("^A"),
            vec![KeyCombination::new("a".to_owned()).with_control()]
        );
        assert_eq!(
            bind("^a"),
            vec![KeyCombination::new("a".to_owned()).with_control()]
        );
    }

    #[test]
    fn t_bind_single_special_key() {
        assert_eq!(bind("="), vec![KeyCombination::new("=".to_owned())]);
        assert_eq!(bind("<"), vec![KeyCombination::new("LT".to_owned())]);
        assert_eq!(bind(">"), vec![KeyCombination::new("GT".to_owned())]);
        assert_eq!(bind("-"), vec![KeyCombination::new("-".to_owned())]);
    }

    #[test]
    fn t_bind_single_key_with_modifiers() {
        assert_eq!(
            bind("<SPACE>"),
            vec![KeyCombination::new("SPACE".to_owned())]
        );
        assert_eq!(
            bind("<M-SPACE>"),
            vec![KeyCombination::new("SPACE".to_owned()).with_alt()]
        );
        assert_eq!(
            bind("<S-SPACE>"),
            vec![KeyCombination::new("SPACE".to_owned()).with_shift()]
        );
        assert_eq!(
            bind("<S-M-SPACE>"),
            vec![
                KeyCombination::new("SPACE".to_owned())
                    .with_shift()
                    .with_alt()
            ]
        );
        assert_eq!(
            bind("<C-SPACE>"),
            vec![KeyCombination::new("SPACE".to_owned()).with_control()]
        );
        assert_eq!(
            bind("<C-M-SPACE>"),
            vec![
                KeyCombination::new("SPACE".to_owned())
                    .with_control()
                    .with_alt()
            ]
        );
        assert_eq!(
            bind("<C-S-SPACE>"),
            vec![
                KeyCombination::new("SPACE".to_owned())
                    .with_control()
                    .with_shift()
            ]
        );
        assert_eq!(
            bind("<C-S-M-SPACE>"),
            vec![
                KeyCombination::new("SPACE".to_owned())
                    .with_control()
                    .with_shift()
                    .with_alt()
            ]
        );
    }

    #[test]
    fn t_bind_single_key_with_modifiers_in_nonstandard_order() {
        assert_eq!(
            bind("<M-S-C-SPACE>"),
            vec![
                KeyCombination::new("SPACE".to_owned())
                    .with_control()
                    .with_shift()
                    .with_alt()
            ]
        );
    }

    #[test]
    fn t_bind_multiple_regular_keys() {
        assert_eq!(
            bind("abc"),
            vec![
                KeyCombination::new("a".to_owned()),
                KeyCombination::new("b".to_owned()),
                KeyCombination::new("c".to_owned()),
            ]
        );
    }

    #[test]
    fn t_bind_multiple_special_keys() {
        assert_eq!(
            bind("<F1><S-ENTER>"),
            vec![
                KeyCombination::new("F1".to_owned()),
                KeyCombination::new("ENTER".to_owned()).with_shift(),
            ]
        );
    }

    #[test]
    fn t_bind_multiple_mixed_keys() {
        assert_eq!(
            bind("^G<ENTER>p"),
            vec![
                KeyCombination::new("g".to_owned()).with_control(),
                KeyCombination::new("ENTER".to_owned()),
                KeyCombination::new("p".to_owned()),
            ]
        );
    }
}

#[derive(PartialEq)]
pub enum ShiftState {
    Shift,
    NoShift,
}

#[derive(PartialEq)]
pub enum ControlState {
    Control,
    NoControl,
}

#[derive(PartialEq)]
pub enum AltState {
    Alt,
    NoAlt,
}

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

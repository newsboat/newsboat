#[derive(PartialEq, Debug)]
pub enum ActionHandlerStatus {
    InvalidCommand,
    TooFewParameters,
    CustomErrorMessage(String),
}

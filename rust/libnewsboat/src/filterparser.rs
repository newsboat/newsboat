#[derive(Debug, Clone)]
enum Value {
    Str(String),
    Int(i32),
    Range(i32, i32)
}

#[derive(Debug, Clone)]
enum Expression {
    And(Box<Expression>, Box<Expression>),
    Or(Box<Expression>, Box<Expression>),
    Condition{
        attribute: String,
        op: String,
        value: Value
    },
}

use filterparser;
use filterparser::Expression;

struct Matcher {
    expr: Expression,
}

impl Matcher {
    pub fn parse(expr: &str) -> Result<Matcher, &str> {
        let expr = filterparser::parse(expr)?;
        Ok(Matcher {
            expr
        })
    }
}

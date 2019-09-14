use filterparser;
use filterparser::Expression;

struct Matcher {
    expr: Expression,
}

impl Matcher {
    fn parse(expr: &str) -> Matcher {
        let expr = filterparser::parse(expr).unwrap();
        Matcher {
            expr
        }
    }
}

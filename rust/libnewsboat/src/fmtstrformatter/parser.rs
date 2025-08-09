use nom::IResult;
use nom::Parser;
use nom::branch::alt;
use nom::bytes::complete::{tag, take, take_till1, take_while};
use nom::multi::many0;
use std::cmp::Ordering;
use std::str;

/// Describes how formats should be padded: on the left, on the right, or not at all.
#[derive(PartialEq, Eq, Debug)]
pub enum Padding {
    /// Do not pad the value.
    None,
    /// Pad the value on the left until it reaches specified width.
    Left(usize),
    /// Pad the value on the right until it reaches specified width.
    Right(usize),
    /// Pad the value on the left and right equally until it reaches specified width.
    Center(usize),
}

/// Describes all the different "format specifiers" we support, plus a chunk of text that would be
/// copied to the output verbatim.
#[derive(PartialEq, Eq, Debug)]
pub enum Specifier<'a> {
    /// Will expand to pad everything that comes next to the right. Given char is used for padding.
    Spacing(char),
    /// A format to be replaced with a value (`%a`, `%t` etc.), padded to the given width on the
    /// left (if it's positive) or on the right (if it's negative).
    Format(char, Padding),
    /// A chunk of text that will be copied to the output verbatim.
    Text(&'a str),
    /// Conditional format that is replaced by one of the sub-formats depending on the value of the
    /// given key. "Else" branch might be missing.
    Conditional(char, Vec<Specifier<'a>>, Option<Vec<Specifier<'a>>>),
}

fn escaped_percent_sign(input: &str) -> IResult<&str, Specifier<'_>> {
    tag("%%")(input).map(|result| (result.0, Specifier::Text(&result.1[0..1])))
}

fn spacing(input: &str) -> IResult<&str, Specifier<'_>> {
    let (input, _) = tag("%>")(input)?;
    let (input, c) = take(1usize)(input)?;

    // unwrap() won't panic because we use take!(1) in parser above
    let chr = c.chars().next().unwrap();

    Ok((input, Specifier::Spacing(chr)))
}

fn center_format(input: &str) -> IResult<&str, Specifier<'_>> {
    let (input, _) = tag("%=")(input)?;
    let (input, width) = take_while(|chr: char| chr.is_ascii() && (chr.is_numeric()))(input)?;
    let (input, format) = take(1usize)(input)?;

    // unwrap() won't fail because parser uses take!(1) to get exactly one character
    let format = format.chars().next().unwrap();
    let width = width.parse::<usize>().unwrap_or(0);

    Ok((input, Specifier::Format(format, Padding::Center(width))))
}

fn padded_format(input: &str) -> IResult<&str, Specifier<'_>> {
    let (input, _) = tag("%")(input)?;
    let (input, width) =
        take_while(|chr: char| chr.is_ascii() && (chr.is_numeric() || chr == '-'))(input)?;
    let (input, format) = take(1usize)(input)?;

    // unwrap() won't fail because parser uses take!(1) to get exactly one character
    let format = format.chars().next().unwrap();

    let width = width.parse::<isize>().unwrap_or(0);
    let padding = match width.cmp(&0isize) {
        Ordering::Equal => Padding::None,
        Ordering::Greater => Padding::Left(width.unsigned_abs()),
        Ordering::Less => Padding::Right(width.unsigned_abs()),
    };

    Ok((input, Specifier::Format(format, padding)))
}

fn text_outside_conditional(input: &str) -> IResult<&str, Specifier<'_>> {
    let (input, text) = take_till1(|chr: char| chr == '%')(input)?;

    Ok((input, Specifier::Text(text)))
}

fn text_inside_conditional(input: &str) -> IResult<&str, Specifier<'_>> {
    let (input, text) = take_till1(|chr: char| chr == '%' || chr == '&' || chr == '?')(input)?;

    Ok((input, Specifier::Text(text)))
}

fn conditional(input: &str) -> IResult<&str, Specifier<'_>> {
    // Prepared partial parsers
    let start_tag = tag("%?");
    let mut condition = take(1usize);
    let then_tag = tag("?");
    let then_branch = conditional_branch;
    let else_tag = tag("&");
    let end_tag = tag("?");

    let some_else_branch = |input| {
        let (input, _) = else_tag(input)?;
        let (input, els) = conditional_branch(input)?;
        let (input, _) = end_tag(input)?;
        Ok((input, Some(els)))
    };
    let none_else_branch = |input| {
        let (input, _) = end_tag(input)?;
        Ok((input, None))
    };
    let mut else_branch = alt((some_else_branch, none_else_branch));

    // Input parsing
    let (input, _) = start_tag(input)?;
    let (input, cond) = condition(input)?;
    let (input, _) = then_tag(input)?;
    let (input, then) = then_branch(input)?;
    let (input, els) = else_branch.parse(input)?;

    // unwrap() won't panic because we're using take!(1) to get exactly one character
    let cond = cond.chars().next().unwrap();

    Ok((input, Specifier::Conditional(cond, then, els)))
}

fn conditional_branch(input: &str) -> IResult<&str, Vec<Specifier<'_>>> {
    let alternatives = (
        escaped_percent_sign,
        spacing,
        center_format,
        padded_format,
        text_inside_conditional,
    );
    many0(alt(alternatives)).parse(input)
}

fn parser(input: &str) -> IResult<&str, Vec<Specifier<'_>>> {
    let alternatives = (
        conditional,
        escaped_percent_sign,
        spacing,
        center_format,
        padded_format,
        text_outside_conditional,
    );
    many0(alt(alternatives)).parse(input)
}

fn sanitize(mut input: Vec<Specifier>) -> Vec<Specifier> {
    input.retain(|s| {
        if let Specifier::Format(c, ref _b) = *s {
            c.is_ascii()
        } else {
            true
        }
    });
    input
}

pub fn parse(input: &str) -> Vec<Specifier<'_>> {
    match parser(input) {
        Ok((_leftovers, ast)) => sanitize(ast),
        Err(_) => vec![Specifier::Text("")],
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_parses_formats_without_specifiers() {
        let input = "Hello, world!";
        let (leftovers, result) = parser(input).unwrap();
        assert_eq!(leftovers, "");
        assert_eq!(result, vec![Specifier::Text("Hello, world!")]);
    }

    #[test]
    fn t_replaces_double_percent_with_a_single_percent() {
        let input = "%%";
        let (leftovers, result) = parser(input).unwrap();
        assert_eq!(leftovers, "");
        assert_eq!(result, vec![Specifier::Text("%")]);
    }

    #[test]
    fn t_parses_sequences_of_specifiers() {
        let input = "100%% pure Ceylon tea";
        let (leftovers, result) = parser(input).unwrap();
        assert_eq!(leftovers, "");

        let expected = vec![
            Specifier::Text("100"),
            Specifier::Text("%"),
            Specifier::Text(" pure Ceylon tea"),
        ];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_formats_with_letters() {
        let input = "%t (%a)";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![
            Specifier::Format('t', Padding::None),
            Specifier::Text(" ("),
            Specifier::Format('a', Padding::None),
            Specifier::Text(")"),
        ];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_formats_with_positive_padding() {
        let input = "%8a%4b%13x";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![
            Specifier::Format('a', Padding::Left(8)),
            Specifier::Format('b', Padding::Left(4)),
            Specifier::Format('x', Padding::Left(13)),
        ];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_formats_with_negative_padding() {
        let input = "%-8a%-4b%-13x";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![
            Specifier::Format('a', Padding::Right(8)),
            Specifier::Format('b', Padding::Right(4)),
            Specifier::Format('x', Padding::Right(13)),
        ];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_spacing_format() {
        let input = "%-8a%>m%4b%> %-13x";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![
            Specifier::Format('a', Padding::Right(8)),
            Specifier::Spacing('m'),
            Specifier::Format('b', Padding::Left(4)),
            Specifier::Spacing(' '),
            Specifier::Format('x', Padding::Right(13)),
        ];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_conditionals() {
        let input = "%?x?success&failure?";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![Specifier::Conditional(
            'x',
            vec![Specifier::Text("success")],
            Some(vec![Specifier::Text("failure")]),
        )];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_conditionals_without_else_branch() {
        let input = "%?x?success?";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![Specifier::Conditional(
            'x',
            vec![Specifier::Text("success")],
            None,
        )];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_conditionals_with_empty_then_branch() {
        let input = "%?x??";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![Specifier::Conditional('x', vec![], None)];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_conditionals_with_empty_then_nonempty_else_branches() {
        let input = "%?x?&nonempty?";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![Specifier::Conditional(
            'x',
            vec![],
            Some(vec![Specifier::Text("nonempty")]),
        )];
        assert_eq!(result, expected);
    }

    #[test]
    fn t_parses_conditionals_with_empty_then_and_else_branches() {
        let input = "%?x?&?";
        let (leftovers, result) = parser(input).unwrap();

        assert_eq!(leftovers, "");

        let expected = vec![Specifier::Conditional('x', vec![], Some(vec![]))];
        assert_eq!(result, expected);
    }
}

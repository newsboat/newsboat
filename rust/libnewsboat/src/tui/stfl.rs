use nom::combinator::value;
use nom::{
    branch::alt,
    bytes::complete::{escaped_transform, is_not, tag},
    combinator::eof,
    multi::many0,
    IResult,
};

fn parse_fragment_single(input: &str) -> IResult<&str, String> {
    let (input, _) = tag("'")(input)?;
    let (input, fragment) = escaped_transform(
        is_not(r#"'\"#),
        '\\',
        alt((value("\\", tag("\\")), value("'", tag("'")))),
    )(input)?;
    let (input, _) = tag("'")(input)?;
    Ok((input, fragment))
}

fn parse_fragment_double(input: &str) -> IResult<&str, String> {
    let (input, _) = tag("\"")(input)?;
    let (input, item) = escaped_transform(
        is_not(r#""\"#),
        '\\',
        alt((value("\\", tag("\\")), value("\"", tag("\"")))),
    )(input)?;
    let (input, _) = tag("\"")(input)?;
    Ok((input, item))
}

fn parse_string(input: &str) -> IResult<&str, String> {
    let (input, fragments) = many0(alt((parse_fragment_single, parse_fragment_double)))(input)?;
    let item = fragments.concat();
    Ok((input, item))
}

fn parse_listitem(input: &str) -> IResult<&str, String> {
    let (input, _) = tag("{listitem text:")(input)?;
    let (input, item) = parse_string(input)?;
    let (input, _) = tag("}")(input)?;
    Ok((input, item))
}

pub fn parse_list(input: &str) -> IResult<&str, Vec<String>> {
    let (input, _) = tag("{list")(input)?;
    let (input, items) = many0(parse_listitem)(input)?;
    let (input, _) = tag("}")(input)?;
    let (input, _) = eof(input)?;
    Ok((input, items))
}

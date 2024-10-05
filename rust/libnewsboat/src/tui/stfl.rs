use nom::bytes::complete::take_till1;
use nom::character::complete::alpha1;
use nom::character::complete::alphanumeric0;
use nom::character::complete::alphanumeric1;
use nom::character::complete::space0;
use nom::character::complete::space1;
use nom::combinator::map;
use nom::combinator::opt;
use nom::combinator::recognize;
use nom::combinator::value;
use nom::multi::separated_list0;
use nom::{
    branch::alt,
    bytes::complete::{escaped_transform, is_not, tag},
    combinator::eof,
    multi::many0,
    IResult,
};

#[derive(Clone, Debug)]
pub struct Style<'a> {
    pub fg: &'a str,
    pub bg: &'a str,
    pub attributes: Vec<&'a str>,
}

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

fn parse_style_fg(input: &str) -> IResult<&str, &str> {
    let (input, _) = opt(tag(","))(input)?;
    let (input, _) = tag("fg=")(input)?;
    alphanumeric1(input)
}

fn parse_style_bg(input: &str) -> IResult<&str, &str> {
    let (input, _) = opt(tag(","))(input)?;
    let (input, _) = tag("bg=")(input)?;
    alphanumeric1(input)
}

fn parse_attribute(input: &str) -> IResult<&str, &str> {
    let (input, _) = opt(tag(","))(input)?;
    let (input, _) = tag("attr=")(input)?;
    let (input, _) = space0(input)?;
    alpha1(input)
}

pub fn parse_style(input: &str) -> IResult<&str, Style> {
    let (input, fg) = opt(parse_style_fg)(input)?;
    let fg = fg.unwrap_or("white");
    let fg = fg.strip_prefix("color").unwrap_or(fg);
    let (input, bg) = opt(parse_style_bg)(input)?;
    let bg = bg.unwrap_or("black");
    let bg = bg.strip_prefix("color").unwrap_or(bg);
    let (input, attributes) = many0(parse_attribute)(input)?;
    Ok((input, Style { fg, bg, attributes }))
}

fn parse_var_name(input: &str) -> IResult<&str, &str> {
    let (input, _) = tag("[")(input)?;
    let (input, name) = take_till1(|c| c == ']')(input)?;
    let (input, _) = tag("]")(input)?;
    Ok((input, name))
}

fn recognize_list_attribute(input: &str) -> IResult<&str, ()> {
    let (input, _) = opt(tag("."))(input)?;
    let (input, _) = alphanumeric1(input)?;
    let (input, _) = opt(parse_var_name)(input)?;
    let (input, _) = tag(":")(input)?;
    let (input, _) = alphanumeric0(input)?;
    Ok((input, ()))
}

fn recognize_style_name(input: &str) -> IResult<&str, ()> {
    let (input, _) = tag("style_")(input)?;
    let (input, _) = take_till1(|c| c == ':' || c == '[')(input)?;
    Ok((input, ()))
}

fn parse_list_style_attribute(input: &str) -> IResult<&str, (&str, Style)> {
    let (input, _) = opt(tag("@"))(input)?;
    let (input, name) = recognize(recognize_style_name)(input)?;
    let (input, _) = opt(parse_var_name)(input)?;
    let (input, _) = tag(":")(input)?;
    let (input, style) = parse_style(input)?;
    Ok((input, (name, style)))
}

fn parse_list_attribute(input: &str) -> IResult<&str, Option<(&str, Style)>> {
    let (input, named_style) = alt((
        map(parse_list_style_attribute, Option::Some),
        value(None, recognize_list_attribute),
    ))(input)?;
    Ok((input, named_style))
}

pub fn parse_list_replacement(input: &str) -> IResult<&str, Vec<(&str, Style)>> {
    let (input, _) = tag("{!list[")(input)?;
    let (input, _list_name) = alphanumeric1(input)?;
    let (input, _) = tag("]")(input)?;
    let (input, _) = space1(input)?;
    let (input, styles) = separated_list0(space1, parse_list_attribute)(input)?;
    let styles = styles.into_iter().flatten().collect();
    let (input, _) = tag("}")(input)?;
    Ok((input, styles))
}

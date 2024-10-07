use nom::{
    bytes::complete::{tag, take_until},
    combinator::eof,
    multi::many0,
    IResult,
};

fn parse_listitem(input: &str) -> IResult<&str, &str> {
    let (input, _) = tag("{listitem text:\"")(input)?;
    // TODO: Handle escaped double quotes
    let (input, item) = take_until("\"")(input)?;
    let (input, _) = tag("\"}")(input)?;
    Ok((input, item))
}

pub fn parse_list(input: &str) -> IResult<&str, Vec<&str>> {
    let (input, _) = tag("{list")(input)?;
    let (input, items) = many0(parse_listitem)(input)?;
    let (input, _) = tag("}")(input)?;
    let (input, _) = eof(input)?;
    Ok((input, items))
}

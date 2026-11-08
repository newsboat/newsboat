use nom::Parser;
use nom::{
    IResult,
    branch::alt,
    bytes::complete::{escaped_transform, is_not, tag, take},
    character::complete::{alpha1, space0, space1},
    combinator::{complete, eof, map, opt, recognize, value, verify},
    multi::{many0, many1, separated_list0, separated_list1},
    sequence::{delimited, preceded},
};

fn unquoted_token(input: &str) -> IResult<&str, String> {
    let parser = map(recognize(is_not("\t\" ;")), String::from);
    let mut parser = verify(parser, |t: &str| t != "--");

    parser.parse(input)
}

fn quoted_token<'a>(input: &'a str) -> IResult<&'a str, String> {
    let parser = escaped_transform(is_not(r#""\"#), '\\', |control_char: &'a str| {
        alt((
            value(r#"""#, tag(r#"""#)),
            value(r"\", tag(r"\")),
            value("\r", tag("r")),
            value("\n", tag("n")),
            value("\t", tag("t")),
            take(1usize), // all other escaped characters are passed through, unmodified
        ))
        .parse(control_char)
    });

    let double_quote = tag("\"");
    let mut parser = delimited(&double_quote, parser, alt((&double_quote, eof)));

    parser.parse(input)
}

fn token(input: &str) -> IResult<&str, String> {
    let mut parser = alt((quoted_token, unquoted_token));
    parser.parse(input)
}

fn operation_with_args(input: &str) -> IResult<&str, Vec<String>> {
    let mut parser = separated_list1(space1, token);
    parser.parse(input)
}

fn semicolon(input: &str) -> IResult<&str, &str> {
    delimited(space0, tag(";"), space0).parse(input)
}

fn operation_description(input: &str) -> IResult<&str, String> {
    let start_token = delimited(space0, tag("--"), space0);

    let string_content = escaped_transform(
        is_not(r#""\"#),
        '\\',
        alt((
            value("\\", tag("\\")), // `\\` -> `\`
            value("\"", tag("\"")), // `\"` -> `"`
            take(1usize),           // all other escaped characters are passed through, unmodified
        )),
    );

    let double_quote = tag("\"");
    let parser = delimited(&double_quote, string_content, &double_quote);

    let mut parser = preceded(start_token, parser);

    parser.parse(input)
}

fn operation_sequence(
    input: &str,
    allow_description: bool,
) -> IResult<&str, (Vec<Vec<String>>, Option<String>)> {
    let (input, _) = space0(input)?;
    let (input, _) = many0(semicolon).parse(input)?;
    let (input, operations) =
        separated_list0(many1(semicolon), operation_with_args).parse(input)?;
    let (input, _) = many0(semicolon).parse(input)?;

    let (input, optional_description) = if allow_description {
        opt(operation_description).parse(input)?
    } else {
        (input, None)
    };

    let (input, _) = space0(input)?;
    let (input, _) = complete(eof).parse(input)?;

    Ok((input, (operations, optional_description)))
}

fn contexts(input: &str) -> IResult<&str, Vec<&str>> {
    separated_list1(tag(","), alpha1).parse(input)
}

fn key_sequence(input: &str) -> IResult<&str, String> {
    token(input)
}

fn binding(input: &str) -> IResult<&str, Binding> {
    let (input, _) = space0(input)?;
    let (input, key_sequence) = key_sequence(input)?;
    let (input, _) = space1(input)?;
    let (input, contexts) = contexts(input)?;
    let (input, _) = space1(input)?;
    let (input, (operations, description)) = operation_sequence(input, true)?;

    Ok((
        input,
        Binding {
            key_sequence,
            contexts: contexts.into_iter().map(|s| s.to_owned()).collect(),
            operations,
            description,
        },
    ))
}

fn unbind(input: &str) -> IResult<&str, Unbind> {
    let (input, _) = space0(input)?;
    let (input, key_sequence) = key_sequence(input)?;
    let (input, _) = space1(input)?;
    let (input, contexts) = contexts(input)?;
    let (input, _) = space0(input)?;
    let (input, _) = complete(eof).parse(input)?;

    Ok((
        input,
        Unbind {
            key_sequence,
            contexts: contexts.into_iter().map(|s| s.to_owned()).collect(),
        },
    ))
}

/// Split a semicolon-separated list of operations into a vector.
///
/// Each operation is represented by a non-empty sub-vector, where the first element is the name of
/// the operation, and the rest of the elements are operation's arguments.
///
/// Tokens can be double-quoted. Such tokens can contain spaces and C-like escaped sequences: `\n`
/// for newline, `\r` for carriage return, `\t` for tab, `\"` for double quote, `\\` for backslash.
/// Unsupported sequences are stripped of the escaping, e.g. `\e` turns into `e`.
///
/// This function assumes that the input string:
/// 1. doesn't contain a comment;
/// 2. doesn't contain backticks that need to be processed.
///
/// Returns a vector of operations togeter with an optional description, as a tuple, or `None` if
/// the input could not be parsed.
pub fn tokenize_operation_sequence(
    input: &str,
    allow_description: bool,
) -> Option<(Vec<Vec<String>>, Option<String>)> {
    match operation_sequence(input, allow_description) {
        Ok((_leftovers, tokens)) => Some(tokens),
        Err(_error) => None,
    }
}

#[derive(Debug, PartialEq)]
pub struct Binding {
    pub key_sequence: String,
    pub contexts: Vec<String>,
    pub operations: Vec<Vec<String>>,
    pub description: Option<String>,
}

pub fn tokenize_binding(input: &str) -> Option<Binding> {
    binding(input).ok().map(|(_, binding)| binding)
}

#[derive(Debug, PartialEq)]
pub struct Unbind {
    pub key_sequence: String,
    pub contexts: Vec<String>,
}

pub fn tokenize_unbind(input: &str) -> Option<Unbind> {
    unbind(input).ok().map(|(_, unbind)| unbind)
}

#[cfg(test)]
mod tests {
    use super::contexts;
    use super::key_sequence;
    use super::tokenize_binding;
    use super::tokenize_operation_sequence;
    use super::tokenize_unbind;

    macro_rules! vec_of_strings {
        ($($x:expr),*) => (vec![$($x.to_string()),*]);
    }

    #[test]
    fn t_tokenize_operation_sequence_works_for_all_cpp_inputs() {
        assert_eq!(
            tokenize_operation_sequence("", true).unwrap(),
            (Vec::<Vec<String>>::new(), None)
        );
        assert_eq!(
            tokenize_operation_sequence("open", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open-all-unread-in-browser-and-mark-read", true).unwrap(),
            (
                vec![vec_of_strings!["open-all-unread-in-browser-and-mark-read"]],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("; ; ; ;", true).unwrap(),
            (Vec::<Vec<String>>::new(), None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; next", true).unwrap(),
            (vec![vec_of_strings!["open"], vec_of_strings!["next"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; next ; prev", true).unwrap(),
            (
                vec![
                    vec_of_strings!["open"],
                    vec_of_strings!["next"],
                    vec_of_strings!["prev"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("open ; next ; prev ; quit", true).unwrap(),
            (
                vec![
                    vec_of_strings!["open"],
                    vec_of_strings!["next"],
                    vec_of_strings!["prev"],
                    vec_of_strings!["quit"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(r#"set "arg 1""#, true).unwrap(),
            (vec![vec_of_strings!["set", "arg 1"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"set "arg 1" ; set "arg 2" "arg 3""#, true).unwrap(),
            (
                vec![
                    vec_of_strings!["set", "arg 1"],
                    vec_of_strings!["set", "arg 2", "arg 3"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(r#"set browser "firefox"; open-in-browser"#, true).unwrap(),
            (
                vec![
                    vec_of_strings!["set", "browser", "firefox"],
                    vec_of_strings!["open-in-browser"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("set browser firefox; open-in-browser", true).unwrap(),
            (
                vec![
                    vec_of_strings!["set", "browser", "firefox"],
                    vec_of_strings!["open-in-browser"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("open-in-browser; quit", true).unwrap(),
            (
                vec![vec_of_strings!["open-in-browser"], vec_of_strings!["quit"]],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(
                r#"open; set browser "firefox --private-window"; quit"#,
                true
            )
            .unwrap(),
            (
                vec![
                    vec_of_strings!["open"],
                    vec_of_strings!["set", "browser", "firefox --private-window"],
                    vec_of_strings!["quit"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(
                r#"open ;set browser "firefox --private-window" ;quit"#,
                true
            )
            .unwrap(),
            (
                vec![
                    vec_of_strings!["open"],
                    vec_of_strings!["set", "browser", "firefox --private-window"],
                    vec_of_strings!["quit"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(
                r#"open;set browser "firefox --private-window";quit"#,
                true
            )
            .unwrap(),
            (
                vec![
                    vec_of_strings!["open"],
                    vec_of_strings!["set", "browser", "firefox --private-window"],
                    vec_of_strings!["quit"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("; ;; ; open", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ;; ;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ; ;;;;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; open ; ;;;;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("; open ;; ;; ;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; ;;; ;;", true).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(
                r#"set browser "sleep 3; do-something ; echo hi"; open-in-browser"#,
                true
            )
            .unwrap(),
            (
                vec![
                    vec_of_strings!["set", "browser", "sleep 3; do-something ; echo hi"],
                    vec_of_strings!["open-in-browser"]
                ],
                None
            )
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_ignores_escaped_sequences_outside_double_quotes() {
        assert_eq!(
            tokenize_operation_sequence(r"\t", true).unwrap(),
            (vec![vec_of_strings![r"\t"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r"\r", true).unwrap(),
            (vec![vec_of_strings![r"\r"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r"\n", true).unwrap(),
            (vec![vec_of_strings![r"\n"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r"\v", true).unwrap(),
            (vec![vec_of_strings![r"\v"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r"\\", true).unwrap(),
            (vec![vec_of_strings![r"\\"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_expands_escaped_sequences_inside_double_quotes() {
        assert_eq!(
            tokenize_operation_sequence(r#""\t""#, true).unwrap(),
            (vec![vec_of_strings!["\t"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\r""#, true).unwrap(),
            (vec![vec_of_strings!["\r"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\n""#, true).unwrap(),
            (vec![vec_of_strings!["\n"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\"""#, true).unwrap(),
            (vec![vec_of_strings!["\""]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\\""#, true).unwrap(),
            (vec![vec_of_strings!["\\"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_passes_through_unsupported_escaped_chars_inside_double_quotes()
    {
        assert_eq!(
            tokenize_operation_sequence(r#""\1""#, true).unwrap(),
            (vec![vec_of_strings!["1"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\W""#, true).unwrap(),
            (vec![vec_of_strings!["W"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\b""#, true).unwrap(),
            (vec![vec_of_strings!["b"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\d""#, true).unwrap(),
            (vec![vec_of_strings!["d"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\x""#, true).unwrap(),
            (vec![vec_of_strings!["x"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_implicitly_closes_double_quotes_at_end_of_input() {
        assert_eq!(
            tokenize_operation_sequence(r#"set "arg 1"#, true).unwrap(),
            (vec![vec_of_strings!["set", "arg 1"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_single_character_unquoted() {
        assert_eq!(
            tokenize_operation_sequence(r#"set a b"#, true).unwrap(),
            (vec![vec_of_strings!["set", "a", "b"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_ignores_leading_and_trailing_whitespace() {
        assert_eq!(
            tokenize_operation_sequence(" \t set a b \t   ", true).unwrap(),
            (vec![vec_of_strings!["set", "a", "b"]], None)
        );

        let (operations, description) =
            tokenize_operation_sequence(" \t set a b -- \"description\" \t   ", true).unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert_eq!(description, Some("description".to_string()));
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_tabs_between_arguments() {
        assert_eq!(
            tokenize_operation_sequence("\tset\ta\tb\t;\topen\t", true).unwrap(),
            (
                vec![vec_of_strings!["set", "a", "b"], vec_of_strings!["open"]],
                None
            )
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_supports_optional_description() {
        let (operations, description) =
            tokenize_operation_sequence(r#"set a b -- "name of function""#, true).unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert_eq!(description, Some("name of function".to_string()));
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_dashdash_in_quoted_string() {
        let (operations, description) =
            tokenize_operation_sequence(r#"set a b "--" "name of function""#, true).unwrap();
        assert_eq!(
            operations,
            vec![vec_of_strings!["set", "a", "b", "--", "name of function"]]
        );
        assert!(description.is_none());
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_missing_description() {
        let (operations, description) = tokenize_operation_sequence(r#"set a b"#, true).unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert!(description.is_none());
    }

    #[test]
    fn t_tokenize_operation_sequence_can_disallow_descriptions() {
        let allow_description = false;
        assert_eq!(
            tokenize_operation_sequence(r#"set a b"#, allow_description).unwrap(),
            (vec![vec_of_strings!["set", "a", "b"]], None)
        );

        assert_eq!(
            tokenize_operation_sequence(
                r#"set a b -- "disallowed description""#,
                allow_description
            ),
            None
        );
    }

    fn verify_parsed_description(input: &str, expected_output: &str) {
        let (_operations, description) = tokenize_operation_sequence(input, true).unwrap();
        assert_eq!(description, Some(expected_output.to_string()));
    }

    #[test]
    fn t_tokenize_operation_sequence_ignores_whitespace_preceding_description() {
        verify_parsed_description(
            &format!(r#"set "a" "b"{}--{}"description""#, "", ""),
            "description",
        );
        verify_parsed_description(
            &format!(r#"set "a" "b"{}--{}"description""#, " \t", ""),
            "description",
        );
        verify_parsed_description(
            &format!(r#"set "a" "b"{}--{}"description""#, "", " \t"),
            "description",
        );
        verify_parsed_description(
            &format!(r#"set "a" "b"{}--{}"description""#, " \t", " \t"),
            "description",
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_includes_whitespace_in_description() {
        verify_parsed_description(
            r#"open -- "multi-word description""#,
            "multi-word description",
        );
        verify_parsed_description(
            r#"open -- "  leading and trailing  ""#,
            "  leading and trailing  ",
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_requires_closing_quote_in_description() {
        assert_eq!(
            tokenize_operation_sequence(r#"open -- "description not closed "#, true),
            None
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_requires_quoted_string_after_delimiter() {
        assert_eq!(tokenize_operation_sequence(r#"open --"#, true), None);
        assert_eq!(
            tokenize_operation_sequence(r#"open -- invalid description"#, true),
            None
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_handles_escaped_quotes_in_description() {
        verify_parsed_description(r#"open -- "internal \" quote""#, r#"internal " quote"#);
        verify_parsed_description(
            r#"open -- "\"internal \"\"\" quotes\"""#,
            r#""internal """ quotes""#,
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_handles_escaped_backslashes_in_description() {
        verify_parsed_description(
            r#"open -- "internal \\ backslash""#,
            r"internal \ backslash",
        );
        verify_parsed_description(
            r#"open -- "\"internal \\\\\\ backslashes\"""#,
            r#""internal \\\ backslashes""#,
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_passes_through_unknown_escaped_characters_in_description() {
        verify_parsed_description(r#"open -- "\f\o\o\"\\\b\a\r""#, r#"foo"\bar"#);
    }

    #[test]
    fn t_key_sequence() {
        assert_eq!(key_sequence("x").unwrap().1, "x");
        assert_eq!(key_sequence("\"x\"").unwrap().1, "x");
        assert_eq!(key_sequence("gg").unwrap().1, "gg");
        assert_eq!(key_sequence("<ENTER>").unwrap().1, "<ENTER>");
        assert_eq!(key_sequence("^U<ENTER>").unwrap().1, "^U<ENTER>");
    }

    #[test]
    fn t_contexts() {
        assert_eq!(contexts("everywhere").unwrap().1, vec!["everywhere"]);
        assert_eq!(contexts("feedlist").unwrap().1, vec!["feedlist"]);
        assert_eq!(
            contexts("feedlist,articlelist").unwrap().1,
            vec!["feedlist", "articlelist"]
        );
    }

    #[test]
    fn t_test_tokenize_binding_no_description() {
        let input = "q everywhere quit";

        let parsed_binding = tokenize_binding(input).unwrap();

        assert_eq!(parsed_binding.key_sequence, "q");
        assert_eq!(parsed_binding.contexts, vec!["everywhere"]);
        assert_eq!(parsed_binding.operations, vec![vec_of_strings!("quit")]);
        assert_eq!(parsed_binding.description, None);
    }

    #[test]
    fn t_test_tokenize_binding_with_description() {
        let input = "gg feedlist,articlelist home ; open -- \"Open entry at top of list\"";

        let parsed_binding = tokenize_binding(input).unwrap();

        assert_eq!(parsed_binding.key_sequence, "gg");
        assert_eq!(parsed_binding.contexts, vec!["feedlist", "articlelist"]);
        assert_eq!(
            parsed_binding.operations,
            vec![vec_of_strings!("home"), vec_of_strings!("open")]
        );
        assert_eq!(
            parsed_binding.description,
            Some("Open entry at top of list".to_string())
        );
    }

    #[test]
    fn t_test_tokenize_binding_missing_parts() {
        assert_eq!(tokenize_binding(""), None);
        assert_eq!(tokenize_binding("gg"), None);
        assert_eq!(tokenize_binding("gg everywhere"), None);
    }

    #[test]
    fn t_test_tokenize_binding_incomplete_description_syntax() {
        assert_eq!(tokenize_binding("q everywhere quit -- "), None);
    }

    #[test]
    fn t_tokenize_unbind() {
        let input = "ov feedlist,articlelist";

        let parsed_unbind = tokenize_unbind(input).unwrap();

        assert_eq!(parsed_unbind.key_sequence, "ov");
        assert_eq!(parsed_unbind.contexts, vec!["feedlist", "articlelist"]);
    }

    #[test]
    fn t_tokenize_unbind_missing_parts() {
        assert_eq!(tokenize_unbind(""), None);
        assert_eq!(tokenize_unbind("gg"), None);
        assert_eq!(tokenize_unbind("everywhere"), None);
        assert_eq!(tokenize_unbind("feedlist,articlelist"), None);
    }
}

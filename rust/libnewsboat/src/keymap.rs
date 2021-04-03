use nom::{
    branch::alt,
    bytes::complete::{escaped_transform, is_not, tag, take},
    character::complete::one_of,
    combinator::{complete, eof, map, opt, recognize, value, verify},
    multi::{many0, many1, separated_list0, separated_list1},
    sequence::{delimited, preceded, terminated, tuple},
    IResult,
};

fn unquoted_token(input: &str) -> IResult<&str, String> {
    let parser = map(recognize(is_not("\t\" ;")), String::from);
    let mut parser = verify(parser, |t: &str| t != "--");

    parser(input)
}

fn quoted_token<'a>(input: &'a str) -> IResult<&'a str, String> {
    let parser = escaped_transform(is_not(r#""\"#), '\\', |control_char: &'a str| {
        alt((
            value(r#"""#, tag(r#"""#)),
            value(r#"\"#, tag(r#"\"#)),
            value("\r", tag("r")),
            value("\n", tag("n")),
            value("\t", tag("t")),
            take(1usize), // all other escaped characters are passed through, unmodified
        ))(control_char)
    });

    let double_quote = tag("\"");
    let mut parser = delimited(&double_quote, parser, alt((&double_quote, eof)));

    parser(input)
}

fn token(input: &str) -> IResult<&str, String> {
    let mut parser = alt((quoted_token, unquoted_token));
    parser(input)
}

fn operation_with_args(input: &str) -> IResult<&str, Vec<String>> {
    let mut parser = separated_list1(many1(one_of(" \t")), token);
    parser(input)
}

fn semicolon(input: &str) -> IResult<&str, &str> {
    delimited(many0(one_of(" \t")), tag(";"), many0(one_of(" \t")))(input)
}

fn operation_description(input: &str) -> IResult<&str, String> {
    let start_token = delimited(many0(one_of(" \t")), tag("--"), many0(one_of(" \t")));

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

    parser(input)
}

fn operation_sequence(input: &str) -> IResult<&str, (Vec<Vec<String>>, Option<String>)> {
    let parser = separated_list0(many1(semicolon), operation_with_args);
    let parser = delimited(many0(semicolon), parser, many0(semicolon));
    let parser = tuple((parser, opt(operation_description)));
    let parser = delimited(many0(one_of(" \t")), parser, many0(one_of(" \t")));
    let parser = terminated(parser, eof);

    let mut parser = complete(parser);

    parser(input)
}

/// Split a semicolon-separated list of operations into a vector. Each operation is represented by
/// a non-empty sub-vector, where the first element is the name of the operation, and the rest of
/// the elements are operation's arguments.
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
pub fn tokenize_operation_sequence(input: &str) -> Option<(Vec<Vec<String>>, Option<String>)> {
    match operation_sequence(input) {
        Ok((_leftovers, tokens)) => Some(tokens),
        Err(_error) => None,
    }
}

#[cfg(test)]
mod tests {
    use super::tokenize_operation_sequence;

    macro_rules! vec_of_strings {
        ($($x:expr),*) => (vec![$($x.to_string()),*]);
    }

    #[test]
    fn t_tokenize_operation_sequence_works_for_all_cpp_inputs() {
        assert_eq!(
            tokenize_operation_sequence("").unwrap(),
            (Vec::<Vec<String>>::new(), None)
        );
        assert_eq!(
            tokenize_operation_sequence("open").unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open-all-unread-in-browser-and-mark-read").unwrap(),
            (
                vec![vec_of_strings!["open-all-unread-in-browser-and-mark-read"]],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("; ; ; ;").unwrap(),
            (Vec::<Vec<String>>::new(), None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; next").unwrap(),
            (vec![vec_of_strings!["open"], vec_of_strings!["next"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; next ; prev").unwrap(),
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
            tokenize_operation_sequence("open ; next ; prev ; quit").unwrap(),
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
            tokenize_operation_sequence(r#"set "arg 1""#).unwrap(),
            (vec![vec_of_strings!["set", "arg 1"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"set "arg 1" ; set "arg 2" "arg 3""#).unwrap(),
            (
                vec![
                    vec_of_strings!["set", "arg 1"],
                    vec_of_strings!["set", "arg 2", "arg 3"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(r#"set browser "firefox"; open-in-browser"#).unwrap(),
            (
                vec![
                    vec_of_strings!["set", "browser", "firefox"],
                    vec_of_strings!["open-in-browser"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("set browser firefox; open-in-browser").unwrap(),
            (
                vec![
                    vec_of_strings!["set", "browser", "firefox"],
                    vec_of_strings!["open-in-browser"]
                ],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence("open-in-browser; quit").unwrap(),
            (
                vec![vec_of_strings!["open-in-browser"], vec_of_strings!["quit"]],
                None
            )
        );
        assert_eq!(
            tokenize_operation_sequence(r#"open; set browser "firefox --private-window"; quit"#)
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
            tokenize_operation_sequence(r#"open ;set browser "firefox --private-window" ;quit"#)
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
            tokenize_operation_sequence(r#"open;set browser "firefox --private-window";quit"#)
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
            tokenize_operation_sequence("; ;; ; open",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ;; ;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; ;; ; open ; ;;;;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(";;; open ; ;;;;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("; open ;; ;; ;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence("open ; ;;; ;;",).unwrap(),
            (vec![vec_of_strings!["open"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(
                r#"set browser "sleep 3; do-something ; echo hi"; open-in-browser"#
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
            tokenize_operation_sequence(r#"\t"#).unwrap(),
            (vec![vec_of_strings![r#"\t"#]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"\r"#).unwrap(),
            (vec![vec_of_strings![r#"\r"#]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"\n"#).unwrap(),
            (vec![vec_of_strings![r#"\n"#]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"\v"#).unwrap(),
            (vec![vec_of_strings![r#"\v"#]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#"\\"#).unwrap(),
            (vec![vec_of_strings![r#"\\"#]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_expands_escaped_sequences_inside_double_quotes() {
        assert_eq!(
            tokenize_operation_sequence(r#""\t""#).unwrap(),
            (vec![vec_of_strings!["\t"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\r""#).unwrap(),
            (vec![vec_of_strings!["\r"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\n""#).unwrap(),
            (vec![vec_of_strings!["\n"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\"""#).unwrap(),
            (vec![vec_of_strings!["\""]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\\""#).unwrap(),
            (vec![vec_of_strings!["\\"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_passes_through_unsupported_escaped_chars_inside_double_quotes()
    {
        assert_eq!(
            tokenize_operation_sequence(r#""\1""#).unwrap(),
            (vec![vec_of_strings!["1"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\W""#).unwrap(),
            (vec![vec_of_strings!["W"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\b""#).unwrap(),
            (vec![vec_of_strings!["b"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\d""#).unwrap(),
            (vec![vec_of_strings!["d"]], None)
        );
        assert_eq!(
            tokenize_operation_sequence(r#""\x""#).unwrap(),
            (vec![vec_of_strings!["x"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_implicitly_closes_double_quotes_at_end_of_input() {
        assert_eq!(
            tokenize_operation_sequence(r#"set "arg 1"#).unwrap(),
            (vec![vec_of_strings!["set", "arg 1"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_single_character_unquoted() {
        assert_eq!(
            tokenize_operation_sequence(r#"set a b"#).unwrap(),
            (vec![vec_of_strings!["set", "a", "b"]], None)
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_ignores_leading_and_trailing_whitespace() {
        assert_eq!(
            tokenize_operation_sequence(" \t set a b \t   ").unwrap(),
            (vec![vec_of_strings!["set", "a", "b"]], None)
        );

        let (operations, description) =
            tokenize_operation_sequence(" \t set a b -- \"description\" \t   ").unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert!(description.is_some());
        assert_eq!(description.unwrap(), "description");
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_tabs_between_arguments() {
        assert_eq!(
            tokenize_operation_sequence("\tset\ta\tb\t;\topen\t").unwrap(),
            (
                vec![vec_of_strings!["set", "a", "b"], vec_of_strings!["open"]],
                None
            )
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_supports_optional_description() {
        let (operations, description) =
            tokenize_operation_sequence(r#"set a b -- "name of function""#).unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert!(description.is_some());
        assert_eq!(description.unwrap(), "name of function");
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_dashdash_in_quoted_string() {
        let (operations, description) =
            tokenize_operation_sequence(r#"set a b "--" "name of function""#).unwrap();
        assert_eq!(
            operations,
            vec![vec_of_strings!["set", "a", "b", "--", "name of function"]]
        );
        assert!(description.is_none());
    }

    #[test]
    fn t_tokenize_operation_sequence_allows_missing_description() {
        let (operations, description) = tokenize_operation_sequence(r#"set a b"#).unwrap();
        assert_eq!(operations, vec![vec_of_strings!["set", "a", "b"]]);
        assert!(description.is_none());
    }

    fn verify_parsed_description(input: &str, expected_output: &str) {
        let (_operations, description) = tokenize_operation_sequence(input).unwrap();
        assert!(description.is_some());
        assert_eq!(description.unwrap(), expected_output);
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
            tokenize_operation_sequence(r#"open -- "description not closed "#),
            None
        );
    }

    #[test]
    fn t_tokenize_operation_sequence_requires_quoted_string_after_delimiter() {
        assert_eq!(tokenize_operation_sequence(r#"open --"#), None);
        assert_eq!(
            tokenize_operation_sequence(r#"open -- invalid description"#),
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
            r#"internal \ backslash"#,
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
}

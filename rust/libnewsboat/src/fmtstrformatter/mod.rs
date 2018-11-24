//! Produces strings of values in a specified format, strftime(3)-like.
mod parser;

use self::parser::{parse, Specifier};
use std::collections::BTreeMap;

/// Produces strings of values in a specified format, strftime(3)-like.
///
/// Newsboat lets users customize its appearance using "format strings". These strings consist of
/// "format specifiers", each of which stand for some fact about the item.
///
/// For example, format strings dictate how each line of an article list look like. `%t` might mean
/// "article title", and `%a` might mean "article's author". Thus, a format string of `"%t (%a)"`
/// would be turned into "How I Spent My Summer (John Doe)" for one article and into "This Week in
/// Rust (Rust Project)" for another.
///
/// Since Newsboat contains a number of different format strings with different specifiers, we need
/// an object to encapsulate the common part: the act of formatting. That way, the above example
/// with titles and authors can be implemented very easily:
/// ```
/// use libnewsboat::fmtstrformatter::*;
///
/// let format = "%t (%a)";
/// let mut fmt = FmtStrFormatter::new();
///
/// // First example
/// {
///     fmt.register_fmt('a', "John Doe".to_string());
///     fmt.register_fmt('t', "How I Spent My Summer".to_string());
///
///     assert_eq!(fmt.do_format(format, 0), "How I Spent My Summer (John Doe)");
/// }
///
/// // Second example
/// {
///     fmt.register_fmt('t', "This Week in Rust".to_string());
///     fmt.register_fmt('a', "Rust Project".to_string());
///
///     assert_eq!(fmt.do_format(format, 0), "This Week in Rust (Rust Project)");
/// }
/// ```
///
/// We also have specifiers for different kinds of padding, and a conditional specifier that's
/// replaced by different values depending on the third one. See the Newsboat documentation for
/// details.
///
/// For the rest of this doc, we'll refer to letters like `a` and `t` above as "keys", and `John
/// Doe` etc. would be "values". The term "format specifiers" will be reserved to things like `%a`,
/// and "format strings" would mean a collection of format specifiers, with optional text in
/// between.
pub struct FmtStrFormatter {
    /// Stores keys and their values.
    fmts: BTreeMap<char, String>,
}

impl FmtStrFormatter {
    /// Construct new `FmtStrFormatter` that contains no keys and no values.
    pub fn new() -> FmtStrFormatter {
        FmtStrFormatter {
            fmts: BTreeMap::new(),
        }
    }

    /// Adds a key-value pair to the formatter.
    pub fn register_fmt(&mut self, key: char, value: String) {
        self.fmts.insert(key, value);
    }

    /// Takes a format string and replaces format specifiers with their values.
    pub fn do_format(&self, format: &str, width: u32) -> String {
        let ast = parse(format);
        self.formatting_helper(&ast, width)
    }

    fn formatting_helper(&self, format_ast: &[Specifier], width: u32) -> String {
        let mut result = String::new();

        for (i, specifier) in format_ast.iter().enumerate() {
            match specifier {
                Specifier::Spacing(c) => {
                    if width == 0 {
                        result.push(*c);
                    } else {
                        let rest = self.formatting_helper(&format_ast[i + 1..], 0);
                        let padding_width = width as usize - rest.len();

                        let mut padding = String::new();
                        padding.push(*c);
                        padding = padding.repeat(padding_width);

                        result.push_str(&padding);
                    };
                }

                Specifier::Format(c, pad_width) => match self.fmts.get(&c) {
                    Some(value) => {
                        let width = value.len();
                        if pad_width.abs() as usize > width {
                            let padding_width = pad_width.abs() as usize - width;
                            let padding = String::from(" ").repeat(padding_width);
                            if *pad_width < 0 {
                                result.push_str(value);
                                result.push_str(&padding);
                            } else {
                                result.push_str(&padding);
                                result.push_str(value);
                            };
                        } else {
                            if let Some((i, _)) = value.char_indices().nth(pad_width.abs() as usize)
                            {
                                result.push_str(&value[..i]);
                            }
                        };
                    }
                    None => continue,
                },

                Specifier::Text(s) => result.push_str(s),

                Specifier::Conditional(cond, then, els) => match self.fmts.get(&cond) {
                    Some(value) if !value.is_empty() => {
                        result.push_str(&self.formatting_helper(then, width))
                    }
                    _ => match els {
                        Some(els) => result.push_str(&self.formatting_helper(els, width)),
                        None => continue,
                    },
                },
            }
        }

        result
    }
}

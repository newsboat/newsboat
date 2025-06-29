use gettextrs::gettext;
use lexopt::{Parser, ValueExt};
use libc::{EXIT_FAILURE, EXIT_SUCCESS};
use std::ffi::{OsStr, OsString};

use std::path::{Path, PathBuf};

use crate::logger::Level;
use crate::utils;
use strprintf::fmt;

#[derive(Default)]
pub struct CliArgsParser {
    pub do_export: bool,
    pub export_as_opml2: bool,
    pub do_vacuum: bool,
    pub do_cleanup: bool,
    pub program_name: String,
    pub show_version: usize,
    pub silent: bool,

    /// If this contains some value, it's the path to the OPML file that should be imported.
    pub importfile: Option<PathBuf>,

    /// If this contains some value, it's the path to the file from which the list of read articles
    /// should be imported.
    pub readinfo_import_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the file to which the list of read articles
    /// should be exported.
    pub readinfo_export_file: Option<PathBuf>,

    /// If this contains some value, the creator of `CliArgsParser` object should call
    /// `exit(return_code)`.
    pub return_code: Option<i32>,

    /// If `display_msg` is not empty, the creator of `CliArgsParser` should
    /// print its contents to stderr.
    ///
    /// \note The contents of this string should be checked before processing `return_code`.
    pub display_msg: String,

    /// If `should_print_usage` is `true`, the creator of `CliArgsParser`
    /// object should print usage information.
    ///
    /// \note This field should be checked before processing `return_code`.
    pub should_print_usage: bool,

    pub refresh_on_start: bool,

    /// If this contains some value, it's the path to the url file specified by the user.
    pub url_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the lock file derived from the cache file
    /// path specified by the user.
    pub lock_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the cache file specified by the user.
    pub cache_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the config file specified by the user.
    pub config_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the podboat queue file specified by the user.
    pub queue_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the search history file specified by the user.
    pub search_history_file: Option<PathBuf>,

    /// If this contains some value, it's the path to the command line history file specified by the user.
    pub cmdline_history_file: Option<PathBuf>,

    /// A vector of Newsboat commands to execute. Empty means user didn't specify any commands to
    /// run.
    ///
    /// \note The parser does not check if the passed commands are valid.
    pub cmds_to_execute: Vec<String>,

    /// If this contains some value, it's the path to the log file specified by the user.
    pub log_file: Option<PathBuf>,

    /// If this contains some value, it's the log level specified by the user.
    pub log_level: Option<Level>,
}

/// Returns new path with an added extension
fn make_sibling_file(path: &Path, extension: impl AsRef<Path>) -> PathBuf {
    let mut result = path.to_path_buf();
    match path.extension() {
        Some(current_extension) => {
            let mut new_extension = current_extension.to_os_string();
            new_extension.push(".");
            new_extension.push(extension.as_ref());
            result.set_extension(new_extension);
        }
        None => {
            result.set_extension(extension.as_ref());
        }
    }
    result
}

use std::convert::TryFrom;
impl TryFrom<&OsStr> for Level {
    type Error = ();

    fn try_from(value: &OsStr) -> Result<Self, <Level as TryFrom<&OsStr>>::Error> {
        match value.to_owned().parse::<u8>() {
            Ok(1) => Ok(Level::UserError),
            Ok(2) => Ok(Level::Critical),
            Ok(3) => Ok(Level::Error),
            Ok(4) => Ok(Level::Warn),
            Ok(5) => Ok(Level::Info),
            Ok(6) => Ok(Level::Debug),
            _ => Err(()),
        }
    }
}
#[derive(Debug)]
pub enum CliParseError {
    LexoptError(lexopt::Error),
    InvalidLogLevel(String),
    PrintAndExit,
}

impl std::fmt::Display for CliParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CliParseError::LexoptError(lexopt) => write!(f, "{lexopt}"),
            CliParseError::InvalidLogLevel(log_message) => write!(f, "{log_message}"),
            CliParseError::PrintAndExit => write!(f, "Erroneous command line arguments"),
        }
    }
}

impl From<lexopt::Error> for CliParseError {
    fn from(err: lexopt::Error) -> CliParseError {
        CliParseError::LexoptError(err)
    }
}

/// Parses command line arguments and stores them in parameter args.
/// Returns Ok(()) or a CliParseError to indicate additional values on CliArgParser that needs
/// setting.
pub fn parse_cliargs(opts: Vec<OsString>, args: &mut CliArgsParser) -> Result<(), CliParseError> {
    use lexopt::prelude::*;

    let resolve_path = |string: &OsString| -> Option<PathBuf> {
        Some(utils::resolve_tilde(PathBuf::from(&string)))
    };

    let mut parser = Parser::from_args(opts);

    while let Some(arg) = parser.next()? {
        match arg {
            Short('i') | Long("import-from-opml") => {
                let import_file_str = parser.value()?;
                args.importfile = resolve_path(&import_file_str);
            }
            Short('e') | Long("export-to-opml") => {
                args.do_export = true;
                args.export_as_opml2 = false;
                args.silent = true;
            }
            Long("export-to-opml2") => {
                args.do_export = true;
                args.export_as_opml2 = true;
                args.silent = true;
            }
            Short('r') | Long("refresh-on-start") => args.refresh_on_start = true,
            Short('h') | Long("help") => {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_SUCCESS);
            }
            Short('u') | Long("url-file") => {
                let url_file = parser.value()?;
                args.url_file = resolve_path(&url_file);
            }
            Short('c') | Long("cache-file") => {
                let cache_file = parser.value()?;
                let cache_file = resolve_path(&cache_file);
                // unwrap won't panic on cache_file.unwrap(), we know we return Some(...) from ^ utility closure
                let lock_file = make_sibling_file(cache_file.as_ref().unwrap(), "lock");
                args.cache_file = cache_file;
                args.lock_file = Some(lock_file);
            }
            Short('C') | Long("config-file") => {
                let config_file = parser.value()?;
                args.config_file = resolve_path(&config_file);
            }
            Long("queue-file") => {
                let queue_file = parser.value()?;
                args.queue_file = resolve_path(&queue_file);
            }
            Long("search-history-file") => {
                let search_history_file = parser.value()?;
                args.search_history_file = resolve_path(&search_history_file);
            }
            Long("cmdline-history-file") => {
                let cmdline_history_file = parser.value()?;
                args.cmdline_history_file = resolve_path(&cmdline_history_file);
            }
            Short('X') | Long("vacuum") => args.do_vacuum = true,
            Long("cleanup") => args.do_cleanup = true,
            Short('v') | Long("version") | Short('V') | Long("-V") => args.show_version += 1,
            Short('x') | Long("execute") => {
                for cmd in parser.values()? {
                    args.cmds_to_execute.push(cmd.to_string_lossy().into());
                }
                if args.cmds_to_execute.is_empty() {
                    return Err(CliParseError::PrintAndExit);
                }
                args.silent = true;
            }
            Short('q') | Long("quiet") => args.silent = true,
            Short('I') | Long("import-from-file") => {
                let importfile = parser.value()?;
                args.readinfo_import_file = resolve_path(&importfile);
            }
            Short('E') | Long("export-to-file") => {
                let exportfile = parser.value()?;
                args.readinfo_export_file = resolve_path(&exportfile);
            }
            Short('d') | Long("log-file") => {
                let log_file = parser.value()?;
                args.log_file = resolve_path(&log_file);
            }
            Short('l') | Long("log-level") => {
                let log_level_str = parser.value()?;
                match Level::try_from(log_level_str.as_ref()) {
                    Ok(level) => {
                        args.log_level = Some(level);
                    }
                    Err(()) => {
                        return Err(CliParseError::InvalidLogLevel(fmt!(
                            &gettext("%s: %s: invalid loglevel value"),
                            &args.program_name,
                            log_level_str.to_string_lossy().to_string()
                        )));
                    }
                }
            }
            _ => return Err(CliParseError::from(arg.unexpected())),
        }
    }

    if args.do_export && args.importfile.is_some() {
        return Err(CliParseError::PrintAndExit);
    }

    if args.readinfo_import_file.is_some() && args.readinfo_export_file.is_some() {
        return Err(CliParseError::PrintAndExit);
    }

    Ok(())
}

impl CliArgsParser {
    pub fn new(mut opts: Vec<OsString>) -> CliArgsParser {
        let mut args = CliArgsParser::default();

        if let Some(program_name) = opts
            .first()
            .map(|program_name| program_name.to_string_lossy().into_owned())
        {
            args.program_name = program_name;
            opts.remove(0);
        }

        match parse_cliargs(opts, &mut args) {
            Ok(()) => args,
            Err(err) => {
                match err {
                    CliParseError::InvalidLogLevel(display_msg) => {
                        args.display_msg = display_msg;
                    }
                    CliParseError::LexoptError(_) => {
                        args.should_print_usage = true;
                    }
                    CliParseError::PrintAndExit => {
                        args.should_print_usage = true;
                    }
                }
                args.return_code = Some(EXIT_FAILURE);
                args
            }
        }
    }

    pub fn using_nonstandard_configs(&self) -> bool {
        self.url_file.is_some()
            || self.cache_file.is_some()
            || self.config_file.is_some()
            || self.queue_file.is_some()
            || self.search_history_file.is_some()
            || self.cmdline_history_file.is_some()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    const LOCK_SUFFIX: &str = ".lock";
    #[test]
    fn t_asks_to_print_usage_info_and_exit_with_failure_on_unknown_option() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);
            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec!["newsboat".into(), "--some-unknown-option".into()]);

        check(vec!["newsboat".into(), "-s".into()]);
        check(vec!["newsboat".into(), "-s".into()]);
        check(vec!["newsboat".into(), "-m ix".into()]);
        check(vec!["newsboat".into(), "-wtf".into()]);
    }

    #[test]
    fn t_sets_do_import_and_importfile_if_dash_i_is_provided() {
        let filename = "blogroll.opml";

        let check = |opts| {
            let args = CliArgsParser::new(opts);
            assert_eq!(args.importfile, Some(PathBuf::from(filename)));
        };

        check(vec!["newsboat".into(), "-i".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--import-from-opml={filename}").into(),
        ]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_failure_if_both_import_and_export_are_provided() {
        let importf: OsString = "import.opml".into();
        let exportf: OsString = "export.opml".into();

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec![
            "newsboat".into(),
            "-i".into(),
            importf.clone(),
            "-e".into(),
            exportf.clone(),
        ]);
        check(vec![
            "newsboat".into(),
            "-e".into(),
            exportf,
            "-i".into(),
            importf,
        ]);
    }

    #[test]
    fn t_sets_refresh_on_start_if_dash_r_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.refresh_on_start);
        };

        check(vec!["newsboat".into(), "-r".into()]);
        check(vec!["newsboat".into(), "--refresh-on-start".into()]);
    }

    #[test]
    fn t_requests_silent_mode_if_dash_e_is_provided() {
        let check = |args| {
            let args = CliArgsParser::new(args);

            assert!(args.silent);
        };

        check(vec!["newsboat".into(), "-e".into()]);
        check(vec!["newsboat".into(), "--export-to-opml".into()]);
    }

    #[test]
    fn t_sets_do_export_if_dash_e_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_export);
        };

        check(vec!["newsboat".into(), "-e".into()]);
        check(vec!["newsboat".into(), "--export-to-opml".into()]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_success_if_dash_h_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_SUCCESS));
        };

        check(vec!["newsboat".into(), "-h".into()]);
        check(vec!["newsboat".into(), "--help".into()]);
    }

    #[test]
    fn t_sets_url_file_set_url_file_and_using_nonstandard_configs_if_dash_u_is_provided() {
        let filename = "urlfile";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.url_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec!["newsboat".into(), "-u".into(), filename.into()]);

        check(vec![
            "newsboat".into(),
            format!("--url-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_sets_proper_fields_when_dash_c_is_provided() {
        let filename = "cache.db";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.cache_file, Some(PathBuf::from(filename)));
            assert_eq!(
                args.lock_file,
                Some(PathBuf::from(filename.to_string() + LOCK_SUFFIX))
            );
            assert!(args.using_nonstandard_configs());
        };

        check(vec!["newsboat".into(), "-c".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--cache-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_supports_combined_short_options() {
        let filename = "cache.db";

        let opts = vec!["newsboat".into(), "-vc".into(), filename.into()];

        let args = CliArgsParser::new(opts);

        assert_eq!(args.cache_file, Some(PathBuf::from(filename)));
        assert_eq!(
            args.lock_file,
            Some(PathBuf::from(filename.to_string() + LOCK_SUFFIX))
        );
        assert!(args.using_nonstandard_configs());
        assert_eq!(args.show_version, 1)
    }

    #[test]
    fn t_supports_combined_short_option_and_value() {
        let filename = "cache.db";

        let opts = vec!["newsboat".into(), format!("-c{}", &filename).into()];

        let args = CliArgsParser::new(opts);

        assert_eq!(args.cache_file, Some(PathBuf::from(filename)));
        assert_eq!(
            args.lock_file,
            Some(PathBuf::from(filename.to_string() + LOCK_SUFFIX))
        );
        assert!(args.using_nonstandard_configs());
    }

    #[test]
    fn t_supports_combined_short_options_last_has_equalsign() {
        let check = |opts, cmds: Vec<String>| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.cmds_to_execute, cmds);
        };

        check(
            vec!["newsboat".into(), "-rx=reload".into()],
            vec!["reload".into()],
        );
    }

    #[test]
    fn t_should_fail_on_equal_sign_with_multiple_values() {
        let check_not_eq = |opts, cmds: Vec<String>| {
            let args = CliArgsParser::new(opts);

            assert_ne!(args.cmds_to_execute, cmds);
            assert!(args.should_print_usage);
        };

        check_not_eq(
            vec!["newsboat".into(), "-x=reload".into(), "print-unread".into()],
            vec!["reload".into(), "print-unread".into()],
        );

        check_not_eq(
            vec![
                "newsboat".into(),
                "--execute=reload".into(),
                "print-unread".into(),
            ],
            vec!["reload".into(), "print-unread".into()],
        );
    }

    #[test]
    fn t_supports_equals_between_combined_short_option_and_value() {
        let filename = "cache.db";

        let opts = vec!["newsboat".into(), format!("-c={}", &filename).into()];

        let args = CliArgsParser::new(opts);

        assert_eq!(args.cache_file, Some(PathBuf::from(filename)));
        assert_eq!(
            args.lock_file,
            Some(PathBuf::from(filename.to_string() + LOCK_SUFFIX))
        );
        assert!(args.using_nonstandard_configs());
    }

    #[test]
    fn t_sets_config_file_if_dash_capital_c_is_provided() {
        let filename = "config file";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.config_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec!["newsboat".into(), "-C".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--config-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_sets_queue_file_if_dash_dash_queue_file_is_provided() {
        let filename = "queue file";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.queue_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec![
            "newsboat".into(),
            "--queue-file".into(),
            filename.into(),
        ]);
    }

    #[test]
    fn t_sets_search_history_file_if_dash_dash_search_history_file_is_provided() {
        let filename = "search history file";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.search_history_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec![
            "newsboat".into(),
            "--search-history-file".into(),
            filename.into(),
        ]);
    }

    #[test]
    fn t_sets_cmdline_history_file_if_dash_dash_cmdline_history_file_is_provided() {
        let filename = "cmdline history file";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.cmdline_history_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec![
            "newsboat".into(),
            "--cmdline-history-file".into(),
            filename.into(),
        ]);
    }

    #[test]
    fn t_sets_do_vacuum_if_dash_capital_x_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_vacuum);
        };

        check(vec!["newsboat".into(), "-X".into()]);
        check(vec!["newsboat".into(), "--vacuum".into()]);
    }

    #[test]
    fn t_sets_do_cleanup_if_dash_dash_cleanup_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_cleanup);
        };

        check(vec!["newsboat".into(), "--cleanup".into()]);
    }

    #[test]
    fn t_increases_show_version_with_each_dash_v_provided() {
        let check = |opts, expected_version| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.show_version, expected_version);
        };

        check(vec!["newsboat".into(), "-v".into()], 1);
        check(vec!["newsboat".into(), "-V".into()], 1);
        check(vec!["newsboat".into(), "--version".into()], 1);
        check(vec!["newsboat".into(), "-vvvv".into()], 4);
        check(vec!["newsboat".into(), "-vV".into()], 2);
        check(vec!["newsboat".into(), "--version".into(), "-v".into()], 2);
        check(
            vec![
                "newsboat".into(),
                "-V".into(),
                "--version".into(),
                "-v".into(),
            ],
            3,
        );
        check(vec!["newsboat".into(), "-VvVVvvvvV".into()], 9);
    }

    #[test]
    fn t_requests_silent_mode_if_dash_x_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.silent);
        };

        check(vec!["newsboat".into(), "-x".into(), "reload".into()]);
        check(vec!["newsboat".into(), "--execute".into(), "reload".into()]);
    }

    #[test]
    fn t_inserts_commands_to_cmds_to_execute_if_dash_x_is_provided() {
        let check = |opts, cmds: Vec<String>| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.cmds_to_execute, cmds);
        };

        check(
            vec!["newsboat".into(), "-x".into(), "reload".into()],
            vec!["reload".into()],
        );
        check(
            vec!["newsboat".into(), "--execute".into(), "reload".into()],
            vec!["reload".into()],
        );
        check(
            vec![
                "newsboat".into(),
                "-x".into(),
                "reload".into(),
                "print-unread".into(),
            ],
            vec!["reload".into(), "print-unread".into()],
        );
        check(
            vec![
                "newsboat".into(),
                "--execute".into(),
                "reload".into(),
                "print-unread".into(),
            ],
            vec!["reload".into(), "print-unread".into()],
        );
        check(
            vec![
                "newsboat".into(),
                "-x".into(),
                "print-unread".into(),
                "--execute".into(),
                "reload".into(),
            ],
            vec!["print-unread".into(), "reload".into()],
        );
    }

    #[test]
    fn t_requests_silent_mode_if_dash_q_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.silent);
        };

        check(vec!["newsboat".into(), "-q".into()]);
        check(vec!["newsboat".into(), "--quiet".into()]);
    }

    #[test]
    fn t_sets_readinfo_import_file_if_dash_capital_i_is_provided() {
        let filename = "filename";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.readinfo_import_file, Some(PathBuf::from(filename)));
        };

        check(vec!["newsboat".into(), "-I".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--import-from-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_sets_readinfo_export_file_if_dash_capital_e_is_provided() {
        let filename = "filename";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.readinfo_export_file, Some(PathBuf::from(filename)));
        };

        check(vec!["newsboat".into(), "-E".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--export-to-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_failure_if_both_capital_e_and_capital_i_are_provided() {
        let importf: OsString = "import.opml".into();
        let exportf: OsString = "export.opml".into();

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec![
            "newsboat".into(),
            "-I".into(),
            importf.clone(),
            "-E".into(),
            exportf.clone(),
        ]);
        check(vec![
            "newsboat".into(),
            "-E".into(),
            exportf,
            "-I".into(),
            importf,
        ]);
    }

    #[test]
    fn t_sets_set_log_file_and_log_file_if_dash_d_is_provided() {
        let filename = "log file.txt";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.log_file, Some(PathBuf::from(filename)));
        };

        check(vec!["newsboat".into(), "-d".into(), filename.into()]);
        check(vec![
            "newsboat".into(),
            format!("--log-file={filename}").into(),
        ]);
    }

    #[test]
    fn t_sets_set_log_level_and_log_level_if_argument_to_dash_l_is_1_to_6() {
        let check = |opts, expected_level| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.log_level, Some(expected_level));
        };

        // --log-level=1 means UserError
        check(
            vec!["newsboat".into(), "--log-level=1".into()],
            Level::UserError,
        );

        // --log-level=2 means Critical
        check(
            vec!["newsboat".into(), "--log-level=2".into()],
            Level::Critical,
        );

        // -l3 means Error
        check(vec!["newsboat".into(), "-l3".into()], Level::Error);

        // --log-level=4 means Warn
        check(vec!["newsboat".into(), "--log-level=4".into()], Level::Warn);

        // -l5 means Info
        check(vec!["newsboat".into(), "-l5".into()], Level::Info);

        // -l6 means Debug
        check(vec!["newsboat".into(), "-l6".into()], Level::Debug);
    }

    #[test]
    fn t_sets_display_msg_and_asks_to_exit_with_failure_if_argument_to_dash_l_is_outside_1_to_6_range()
     {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(!args.display_msg.is_empty());
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec!["newsboat".into(), "-l0".into()]);
        check(vec!["newsboat".into(), "--log-level=7".into()]);
        check(vec!["newsboat".into(), "--log-level=90001".into()]);
    }

    #[test]
    fn t_sets_program_name_to_the_first_string_of_the_options_list() {
        let check = |opts, expected: String| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.program_name, expected);
        };

        check(vec!["newsboat".into()], "newsboat".into());
        check(vec!["podboat".into(), "-h".into()], "podboat".into());
        check(
            vec![
                "something else entirely".into(),
                "--foo".into(),
                "--bar".into(),
                "--baz".into(),
            ],
            "something else entirely".into(),
        );
        check(
            vec!["/usr/local/bin/app-with-a-path".into()],
            "/usr/local/bin/app-with-a-path".into(),
        );
    }
}

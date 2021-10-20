use gettextrs::gettext;
use lexopt::Parser;
use libc::{EXIT_FAILURE, EXIT_SUCCESS};
use std::ffi::{OsStr, OsString};
use std::os::unix::ffi::OsStrExt;

use std::path::PathBuf;

use crate::logger::Level;
use crate::utils;
use strprintf::fmt;

#[derive(Default)]
pub struct CliArgsParser {
    pub do_export: bool,
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

const LOCK_SUFFIX: &str = ".lock";

fn strip_eq(value: &OsString) -> &OsStr {
    const EQUAL_SIGN: u8 = b'=';
    if let Some(&EQUAL_SIGN) = value.as_bytes().get(0) {
        OsStr::from_bytes(&value.as_bytes()[1..])
    } else {
        OsStr::from_bytes(value.as_bytes())
    }
}

enum MultiValueOption {
    Execute,
    None,
}

// Wrapper around the Lexopt parser
// Keeps info about what option type we saw last, whether it was in short form or long form
// Also keeps track of if we're parsing a multi value option
struct LexoptWrapper {
    pub parser: lexopt::Parser,
    pub last_seen: MultiValueOption,
    pub last_was_short: bool,
}

impl LexoptWrapper {
    pub fn next(&mut self) -> Result<Option<lexopt::Arg>, lexopt::Error> {
        let arg = self.parser.next();
        match arg {
            Ok(Some(lexopt::prelude::Short(_))) => {
                self.last_was_short = true;
            }
            _ => self.last_was_short = false,
        }
        arg
    }

    // Sets last seen multi value option
    // Currently we only have "Execute" that does this. This was such a tiny addition
    // That I thought it worth it, should other multi value options ever be added
    // it would simply require adding to MultiValueOption enum & handling it under the
    // Value(_) pattern match
    pub fn set_last_seen(&mut self, last_seen: MultiValueOption) {
        self.last_seen = last_seen;
    }

    pub fn value(&mut self) -> Result<OsString, lexopt::Error> {
        self.parser.value()
    }
}

impl CliArgsParser {
    pub fn new(mut opts: Vec<OsString>) -> CliArgsParser {
        use lexopt::prelude::*;
        const CACHE_FILE: &str = "cache-file";
        const CONFIG_FILE: &str = "config-file";
        const EXECUTE: &str = "execute";
        const EXPORT_TO_FILE: &str = "export-to-file";
        const EXPORT_TO_OPML: &str = "export-to-opml";
        const HELP: &str = "help";
        const IMPORT_FROM_FILE: &str = "import-from-file";
        const IMPORT_FROM_OPML: &str = "import-from-opml";
        const LOG_FILE: &str = "log-file";
        const LOG_LEVEL: &str = "log-level";
        const QUIET: &str = "quiet";
        const REFRESH_ON_START: &str = "refresh-on-start";
        const URL_FILE: &str = "url-file";
        const VACUUM: &str = "vacuum";
        const CLEANUP: &str = "cleanup";
        const VERSION: &str = "version";
        const VERSION_V: &str = "-V";
        let mut args = CliArgsParser::default();

        if let Some(program_name) = opts
            .get(0)
            .map(|program_name| program_name.to_string_lossy().into_owned())
        {
            args.program_name = program_name;
            opts.remove(0);
        }

        let parser = Parser::from_args(opts.into_iter());
        let mut parser_wrapper = LexoptWrapper {
            parser,
            last_was_short: false,
            last_seen: MultiValueOption::None,
        };
        while let Ok(Some(arg)) = parser_wrapper.next() {
            match arg {
                Short('i') | Long(IMPORT_FROM_OPML) => match parser_wrapper.value() {
                    Ok(import_file_str) => {
                        args.importfile = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(
                                &import_file_str,
                            ))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&import_file_str)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('e') | Long(EXPORT_TO_OPML) => {
                    args.do_export = true;
                    args.silent = true;
                }
                Short('r') | Long(REFRESH_ON_START) => args.refresh_on_start = true,
                Short('h') | Long(HELP) => {
                    args.should_print_usage = true;
                    args.return_code = Some(EXIT_SUCCESS);
                }
                Short('u') | Long(URL_FILE) => match parser_wrapper.value() {
                    Ok(url_file) => {
                        args.url_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&url_file))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&url_file)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('c') | Long(CACHE_FILE) => match parser_wrapper.value() {
                    Ok(cache_file) => {
                        args.cache_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&cache_file))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&cache_file)))
                        };

                        let mut lock_cache_file = args.cache_file.as_ref().unwrap().clone();
                        lock_cache_file.set_file_name(
                            args.cache_file
                                .as_ref()
                                .unwrap()
                                .file_name()
                                .map(|p| {
                                    OsString::from(p.to_string_lossy().to_string() + LOCK_SUFFIX)
                                })
                                .unwrap(),
                        );
                        args.lock_file = Some(lock_cache_file);
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('C') | Long(CONFIG_FILE) => match parser_wrapper.value() {
                    Ok(config_file) => {
                        args.config_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&config_file))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&config_file)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('X') | Long(VACUUM) => args.do_vacuum = true,
                Long(CLEANUP) => args.do_cleanup = true,
                Short('v') | Long(VERSION) | Short('V') | Long(VERSION_V) => args.show_version += 1,
                Short('x') | Long(EXECUTE) => match parser_wrapper.value() {
                    Ok(cmd) => {
                        parser_wrapper.set_last_seen(MultiValueOption::Execute);
                        let cmd = if parser_wrapper.last_was_short {
                            strip_eq(&cmd)
                        } else {
                            &cmd
                        };
                        args.cmds_to_execute.push(cmd.to_string_lossy().into());
                        args.silent = true;
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('q') | Long(QUIET) => args.silent = true,
                Short('I') | Long(IMPORT_FROM_FILE) => match parser_wrapper.value() {
                    Ok(importfile) => {
                        args.readinfo_import_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&importfile))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&importfile)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('E') | Long(EXPORT_TO_FILE) => match parser_wrapper.value() {
                    Ok(exportfile) => {
                        args.readinfo_export_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&exportfile))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&exportfile)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('d') | Long(LOG_FILE) => match parser_wrapper.value() {
                    Ok(log_file) => {
                        args.log_file = if parser_wrapper.last_was_short {
                            Some(utils::resolve_tilde(PathBuf::from(strip_eq(&log_file))))
                        } else {
                            Some(utils::resolve_tilde(PathBuf::from(&log_file)))
                        };
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                Short('l') | Long(LOG_LEVEL) => match parser_wrapper.value() {
                    Ok(log_level_str) => {
                        let log_level_str_checked = if parser_wrapper.last_was_short {
                            strip_eq(&log_level_str)
                        } else {
                            &log_level_str
                        };
                        match log_level_str_checked.to_owned().parse::<u8>() {
                            Ok(1) => {
                                args.log_level = Some(Level::UserError);
                            }
                            Ok(2) => {
                                args.log_level = Some(Level::Critical);
                            }
                            Ok(3) => {
                                args.log_level = Some(Level::Error);
                            }
                            Ok(4) => {
                                args.log_level = Some(Level::Warn);
                            }
                            Ok(5) => {
                                args.log_level = Some(Level::Info);
                            }
                            Ok(6) => {
                                args.log_level = Some(Level::Debug);
                            }
                            _ => {
                                args.display_msg = fmt!(
                                    &gettext("%s: %s: invalid loglevel value"),
                                    &args.program_name,
                                    log_level_str.to_string_lossy().to_string()
                                );
                                args.return_code = Some(EXIT_FAILURE);
                            }
                        }
                    }
                    _ => return CliArgsParser::should_print_and_exit(args),
                },
                // Here is where we collect (possible) multi values to multi value options
                Value(positional_arg_string) => match parser_wrapper.last_seen {
                    MultiValueOption::Execute => {
                        args.cmds_to_execute
                            .push(positional_arg_string.to_string_lossy().into());
                    }
                    MultiValueOption::None => {
                        // means we found free standing args -> exit
                        return CliArgsParser::should_print_and_exit(args);
                    }
                },
                _ => {
                    return args.should_print_and_exit();
                }
            }
        }

        if args.do_export && args.importfile.is_some() {
            return CliArgsParser::should_print_and_exit(args);
        }

        if args.readinfo_import_file.is_some() && args.readinfo_export_file.is_some() {
            return CliArgsParser::should_print_and_exit(args);
        }
        args
    }

    pub fn using_nonstandard_configs(&self) -> bool {
        self.url_file.is_some() || self.cache_file.is_some() || self.config_file.is_some()
    }

    fn should_print_and_exit(mut self) -> CliArgsParser {
        self.should_print_usage = true;
        self.return_code = Some(EXIT_FAILURE);
        self
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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
            format!("--import-from-opml={}", filename).into(),
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
            format!("--url-file={}", filename).into(),
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
            format!("--cache-file={}", filename).into(),
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
            format!("--config-file={}", filename).into(),
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
            format!("--import-from-file={}", filename).into(),
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
            format!("--export-to-file={}", filename).into(),
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
            format!("--log-file={}", filename).into(),
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
    fn t_sets_display_msg_and_asks_to_exit_with_failure_if_argument_to_dash_l_is_outside_1_to_6_range(
    ) {
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

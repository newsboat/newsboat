use clap::{App, Arg};
use gettextrs::gettext;
use libc::{EXIT_FAILURE, EXIT_SUCCESS};
use std::ffi::OsString;
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

impl CliArgsParser {
    pub fn new(opts: Vec<OsString>) -> CliArgsParser {
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

        let app = App::new("Newsboat")
            .arg(
                Arg::with_name(IMPORT_FROM_OPML)
                    .short("i")
                    .long(IMPORT_FROM_OPML)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(EXPORT_TO_OPML)
                    .short("e")
                    .long(EXPORT_TO_OPML),
            )
            .arg(
                Arg::with_name(REFRESH_ON_START)
                    .short("r")
                    .long(REFRESH_ON_START),
            )
            .arg(Arg::with_name(HELP).short("h").long(HELP))
            .arg(
                Arg::with_name(URL_FILE)
                    .short("u")
                    .long(URL_FILE)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(CACHE_FILE)
                    .short("c")
                    .long(CACHE_FILE)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(CONFIG_FILE)
                    .short("C")
                    .long(CONFIG_FILE)
                    .takes_value(true),
            )
            .arg(Arg::with_name(VACUUM).short("X").long(VACUUM))
            .arg(Arg::with_name(CLEANUP).long(CLEANUP))
            .arg(
                Arg::with_name(VERSION)
                    .short("v")
                    .long(VERSION)
                    .multiple(true),
            )
            .arg(Arg::with_name(VERSION_V).short("V").multiple(true))
            .arg(
                Arg::with_name(EXECUTE)
                    .short("x")
                    .long(EXECUTE)
                    .takes_value(true)
                    .multiple(true),
            )
            .arg(Arg::with_name(QUIET).short("q").long(QUIET))
            .arg(
                Arg::with_name(IMPORT_FROM_FILE)
                    .short("I")
                    .long(IMPORT_FROM_FILE)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(EXPORT_TO_FILE)
                    .short("E")
                    .long(EXPORT_TO_FILE)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(LOG_FILE)
                    .short("d")
                    .long(LOG_FILE)
                    .takes_value(true),
            )
            .arg(
                Arg::with_name(LOG_LEVEL)
                    .short("l")
                    .long(LOG_LEVEL)
                    .takes_value(true),
            );

        let mut args = CliArgsParser::default();

        if let Some(program_name) = opts
            .get(0)
            .map(|program_name| program_name.to_string_lossy().into_owned())
        {
            args.program_name = program_name;
        }

        let matches = match app.get_matches_from_safe(&opts) {
            Ok(matches) => matches,
            Err(_) => {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_FAILURE);
                return args;
            }
        };

        if matches.is_present(EXPORT_TO_OPML) {
            if args.importfile.is_some() {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_FAILURE);
            } else {
                args.do_export = true;
                args.silent = true;
            }
        }

        args.refresh_on_start = matches.is_present(REFRESH_ON_START);

        if matches.is_present(HELP) {
            args.should_print_usage = true;
            args.return_code = Some(EXIT_SUCCESS);
        }

        args.do_vacuum = matches.is_present(VACUUM);

        args.do_cleanup = matches.is_present(CLEANUP);

        args.silent = args.silent || matches.is_present(QUIET);

        if let Some(importfile) = matches.value_of(IMPORT_FROM_OPML) {
            if args.do_export {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_FAILURE);
            } else {
                args.importfile = Some(utils::resolve_tilde(PathBuf::from(importfile)));
            }
        }

        if let Some(url_file) = matches.value_of(URL_FILE) {
            args.url_file = Some(utils::resolve_tilde(PathBuf::from(url_file)));
        }

        if let Some(cache_file) = matches.value_of(CACHE_FILE) {
            args.cache_file = Some(utils::resolve_tilde(PathBuf::from(cache_file)));
            args.lock_file = Some(utils::resolve_tilde(PathBuf::from(
                cache_file.to_string() + LOCK_SUFFIX,
            )));
        }

        if let Some(config_file) = matches.value_of(CONFIG_FILE) {
            args.config_file = Some(utils::resolve_tilde(PathBuf::from(config_file)));
        }

        // Casting u64 to usize. Highly unlikely that the user will hit the limit, even if we were
        // running on an 8-bit platform.
        //
        // There are three "version" options, two of which (-v and --version) are grouped into
        // VERSION, and the other one (-V) is under VERSION_V. This is because Clap won't let us
        // have short aliases.
        args.show_version =
            (matches.occurrences_of(VERSION) + matches.occurrences_of(VERSION_V)) as usize;

        if let Some(mut commands) = matches.values_of_lossy(EXECUTE) {
            args.silent = true;
            args.cmds_to_execute.append(&mut commands);
        }

        if let Some(importfile) = matches.value_of(IMPORT_FROM_FILE) {
            if args.readinfo_export_file.is_some() {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_FAILURE);
            } else {
                args.readinfo_import_file = Some(utils::resolve_tilde(PathBuf::from(importfile)));
            }
        }

        if let Some(exportfile) = matches.value_of(EXPORT_TO_FILE) {
            if args.readinfo_import_file.is_some() {
                args.should_print_usage = true;
                args.return_code = Some(EXIT_FAILURE);
            } else {
                args.readinfo_export_file = Some(utils::resolve_tilde(PathBuf::from(exportfile)));
            }
        }

        if let Some(log_file) = matches.value_of(LOG_FILE) {
            args.log_file = Some(utils::resolve_tilde(PathBuf::from(log_file)));
        }

        if let Some(log_level_str) = matches.value_of(LOG_LEVEL) {
            match log_level_str.parse::<u8>() {
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
                        log_level_str
                    );
                    args.return_code = Some(EXIT_FAILURE);
                }
            };
        }

        args
    }

    pub fn using_nonstandard_configs(&self) -> bool {
        self.url_file.is_some() || self.cache_file.is_some() || self.config_file.is_some()
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

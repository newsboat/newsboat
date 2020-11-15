use clap::{App, Arg};
use gettextrs::gettext;
use libc::{EXIT_FAILURE, EXIT_SUCCESS};
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
    pub fn new(opts: Vec<String>) -> CliArgsParser {
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

        if let Some(program_name) = opts.get(0).cloned() {
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
                        &opts[0],
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

        check(vec![
            "newsboat".to_string(),
            "--some-unknown-option".to_string(),
        ]);

        check(vec!["newsboat".to_string(), "-s".to_string()]);
        check(vec!["newsboat".to_string(), "-s".to_string()]);
        check(vec!["newsboat".to_string(), "-m ix".to_string()]);
        check(vec!["newsboat".to_string(), "-wtf".to_string()]);
    }

    #[test]
    fn t_sets_do_import_and_importfile_if_dash_i_is_provided() {
        let filename = "blogroll.opml";

        let check = |opts| {
            let args = CliArgsParser::new(opts);
            assert_eq!(args.importfile, Some(PathBuf::from(filename)));
        };

        check(vec![
            "newsboat".to_string(),
            "-i".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--import-from-opml=".to_string() + filename,
        ]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_failure_if_both_import_and_export_are_provided() {
        let importf = "import.opml".to_string();
        let exportf = "export.opml".to_string();

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec![
            "newsboat".to_string(),
            "-i".to_string(),
            importf.clone(),
            "-e".to_string(),
            exportf.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "-e".to_string(),
            exportf,
            "-i".to_string(),
            importf,
        ]);
    }

    #[test]
    fn t_sets_refresh_on_start_if_dash_r_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.refresh_on_start);
        };

        check(vec!["newsboat".to_string(), "-r".to_string()]);
        check(vec![
            "newsboat".to_string(),
            "--refresh-on-start".to_string(),
        ]);
    }

    #[test]
    fn t_requests_silent_mode_if_dash_e_is_provided() {
        let check = |args| {
            let args = CliArgsParser::new(args);

            assert!(args.silent);
        };

        check(vec!["newsboat".to_string(), "-e".to_string()]);
        check(vec!["newsboat".to_string(), "--export-to-opml".to_string()]);
    }

    #[test]
    fn t_sets_do_export_if_dash_e_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_export);
        };

        check(vec!["newsboat".to_string(), "-e".to_string()]);
        check(vec!["newsboat".to_string(), "--export-to-opml".to_string()]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_success_if_dash_h_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_SUCCESS));
        };

        check(vec!["newsboat".to_string(), "-h".to_string()]);
        check(vec!["newsboat".to_string(), "--help".to_string()]);
    }

    #[test]
    fn t_sets_url_file_set_url_file_and_using_nonstandard_configs_if_dash_u_is_provided() {
        let filename = "urlfile";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.url_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec![
            "newsboat".to_string(),
            "-u".to_string(),
            filename.to_string(),
        ]);

        check(vec![
            "newsboat".to_string(),
            "--url-file=".to_string() + filename,
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

        check(vec![
            "newsboat".to_string(),
            "-c".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--cache-file=".to_string() + filename,
        ]);
    }

    #[test]
    fn t_sets_config_file_if_dash_capital_c_is_provided() {
        let filename = "config file";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.config_file, Some(PathBuf::from(filename)));
            assert!(args.using_nonstandard_configs());
        };

        check(vec![
            "newsboat".to_string(),
            "-C".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--config-file=".to_string() + filename,
        ]);
    }

    #[test]
    fn t_sets_do_vacuum_if_dash_capital_x_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_vacuum);
        };

        check(vec!["newsboat".to_string(), "-X".to_string()]);
        check(vec!["newsboat".to_string(), "--vacuum".to_string()]);
    }

    #[test]
    fn t_sets_do_cleanup_if_dash_dash_cleanup_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.do_cleanup);
        };

        check(vec!["newsboat".to_string(), "--cleanup".to_string()]);
    }

    #[test]
    fn t_increases_show_version_with_each_dash_v_provided() {
        let check = |opts, expected_version| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.show_version, expected_version);
        };

        check(vec!["newsboat".to_string(), "-v".to_string()], 1);
        check(vec!["newsboat".to_string(), "-V".to_string()], 1);
        check(vec!["newsboat".to_string(), "--version".to_string()], 1);
        check(vec!["newsboat".to_string(), "-vvvv".to_string()], 4);
        check(vec!["newsboat".to_string(), "-vV".to_string()], 2);
        check(
            vec![
                "newsboat".to_string(),
                "--version".to_string(),
                "-v".to_string(),
            ],
            2,
        );
        check(
            vec![
                "newsboat".to_string(),
                "-V".to_string(),
                "--version".to_string(),
                "-v".to_string(),
            ],
            3,
        );
        check(vec!["newsboat".to_string(), "-VvVVvvvvV".to_string()], 9);
    }

    #[test]
    fn t_requests_silent_mode_if_dash_x_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.silent);
        };

        check(vec![
            "newsboat".to_string(),
            "-x".to_string(),
            "reload".to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--execute".to_string(),
            "reload".to_string(),
        ]);
    }

    #[test]
    fn t_inserts_commands_to_cmds_to_execute_if_dash_x_is_provided() {
        let check = |opts, cmds| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.cmds_to_execute, cmds);
        };

        check(
            vec![
                "newsboat".to_string(),
                "-x".to_string(),
                "reload".to_string(),
            ],
            vec!["reload".to_string()],
        );
        check(
            vec![
                "newsboat".to_string(),
                "--execute".to_string(),
                "reload".to_string(),
            ],
            vec!["reload".to_string()],
        );
        check(
            vec![
                "newsboat".to_string(),
                "-x".to_string(),
                "reload".to_string(),
                "print-unread".to_string(),
            ],
            vec!["reload".to_string(), "print-unread".to_string()],
        );
        check(
            vec![
                "newsboat".to_string(),
                "--execute".to_string(),
                "reload".to_string(),
                "print-unread".to_string(),
            ],
            vec!["reload".to_string(), "print-unread".to_string()],
        );
        check(
            vec![
                "newsboat".to_string(),
                "-x".to_string(),
                "print-unread".to_string(),
                "--execute".to_string(),
                "reload".to_string(),
            ],
            vec!["print-unread".to_string(), "reload".to_string()],
        );
    }

    #[test]
    fn t_requests_silent_mode_if_dash_q_is_provided() {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.silent);
        };

        check(vec!["newsboat".to_string(), "-q".to_string()]);
        check(vec!["newsboat".to_string(), "--quiet".to_string()]);
    }

    #[test]
    fn t_sets_readinfo_import_file_if_dash_capital_i_is_provided() {
        let filename = "filename";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.readinfo_import_file, Some(PathBuf::from(filename)));
        };

        check(vec![
            "newsboat".to_string(),
            "-I".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--import-from-file=".to_string() + filename,
        ]);
    }

    #[test]
    fn t_sets_readinfo_export_file_if_dash_capital_e_is_provided() {
        let filename = "filename";

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.readinfo_export_file, Some(PathBuf::from(filename)));
        };

        check(vec![
            "newsboat".to_string(),
            "-E".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--export-to-file=".to_string() + filename,
        ]);
    }

    #[test]
    fn t_asks_to_print_usage_and_exit_with_failure_if_both_capital_e_and_capital_i_are_provided() {
        let importf = "import.opml".to_string();
        let exportf = "export.opml".to_string();

        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(args.should_print_usage);
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec![
            "newsboat".to_string(),
            "-I".to_string(),
            importf.clone(),
            "-E".to_string(),
            exportf.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "-E".to_string(),
            exportf,
            "-I".to_string(),
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

        check(vec![
            "newsboat".to_string(),
            "-d".to_string(),
            filename.to_string(),
        ]);
        check(vec![
            "newsboat".to_string(),
            "--log-file=".to_string() + filename,
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
            vec!["newsboat".to_string(), "--log-level=1".to_string()],
            Level::UserError,
        );

        // --log-level=2 means Critical
        check(
            vec!["newsboat".to_string(), "--log-level=2".to_string()],
            Level::Critical,
        );

        // -l3 means Error
        check(
            vec!["newsboat".to_string(), "-l3".to_string()],
            Level::Error,
        );

        // --log-level=4 means Warn
        check(
            vec!["newsboat".to_string(), "--log-level=4".to_string()],
            Level::Warn,
        );

        // -l5 means Info
        check(vec!["newsboat".to_string(), "-l5".to_string()], Level::Info);

        // -l6 means Debug
        check(
            vec!["newsboat".to_string(), "-l6".to_string()],
            Level::Debug,
        );
    }

    #[test]
    fn t_sets_display_msg_and_asks_to_exit_with_failure_if_argument_to_dash_l_is_outside_1_to_6_range(
    ) {
        let check = |opts| {
            let args = CliArgsParser::new(opts);

            assert!(!args.display_msg.is_empty());
            assert_eq!(args.return_code, Some(EXIT_FAILURE));
        };

        check(vec!["newsboat".to_string(), "-l0".to_string()]);
        check(vec!["newsboat".to_string(), "--log-level=7".to_string()]);
        check(vec![
            "newsboat".to_string(),
            "--log-level=90001".to_string(),
        ]);
    }

    #[test]
    fn t_sets_program_name_to_the_first_string_of_the_options_list() {
        let check = |opts, expected| {
            let args = CliArgsParser::new(opts);

            assert_eq!(args.program_name, expected);
        };

        check(vec!["newsboat".to_string()], "newsboat".to_string());
        check(
            vec!["podboat".to_string(), "-h".to_string()],
            "podboat".to_string(),
        );
        check(
            vec![
                "something else entirely".to_string(),
                "--foo".to_string(),
                "--bar".to_string(),
                "--baz".to_string(),
            ],
            "something else entirely".to_string(),
        );
        check(
            vec!["/usr/local/bin/app-with-a-path".to_string()],
            "/usr/local/bin/app-with-a-path".to_string(),
        );
    }
}

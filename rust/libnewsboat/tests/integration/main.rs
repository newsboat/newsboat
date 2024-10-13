//! this is the single integration test, as documented by matklad
//! in <https://matklad.github.io/2021/02/27/delete-cargo-integration-tests.html>

mod configpaths_helpers;
mod locale_helpers;

mod cliargsparser_resolves_tilde_to_homedir_in_cache_file;
mod cliargsparser_resolves_tilde_to_homedir_in_config_file;
mod cliargsparser_resolves_tilde_to_homedir_in_exportfile;
mod cliargsparser_resolves_tilde_to_homedir_in_importfile;
mod cliargsparser_resolves_tilde_to_homedir_in_logfile;
mod cliargsparser_resolves_tilde_to_homedir_in_opmlfile;
mod cliargsparser_resolves_tilde_to_homedir_in_urlfile;
mod configpaths_01;
mod configpaths_02;
mod configpaths_03;
mod configpaths_04;
mod configpaths_05;
mod configpaths_06;
mod configpaths_07;
mod configpaths_08;
mod configpaths_09;
mod configpaths_10;
mod configpaths_11;
mod configpaths_12;
mod configpaths_13;
mod configpaths_14;
mod configpaths_15;
mod configpaths_16;
mod configpaths_17;
mod fslock;
mod get_default_browser;
mod getcwd_returns_nonempty_string;
mod global_logger_creates_logfile;
mod locale_to_utf8_converts_text_from_the_locale_encoding_to_utf8;
mod log_macro_works;
mod resolve_tilde;
mod run_program_works_for_large_inputs;
mod scopemeasure_dropping_writes_a_line_to_the_log;
mod scopemeasure_stopover_adds_an_extra_line_to_the_log_upon_each_call;
mod utf8_to_locale_01;
mod utf8_to_locale_02;

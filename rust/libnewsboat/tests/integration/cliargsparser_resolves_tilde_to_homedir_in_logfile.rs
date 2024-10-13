use libnewsboat::cliargsparser::CliArgsParser;
use serial_test::serial;
use std::env;
use tempfile::TempDir;

#[test]
#[serial] // Because it changes environment variables
fn t_cliargsparser_dash_d_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "newsboat.log";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.log_file, Some(tmp.path().join(filename)));
    };

    check(vec!["newsboat".into(), "-d".into(), arg.clone().into()]);

    check(vec!["newsboat".into(), format!("--log-file={arg}").into()]);
}

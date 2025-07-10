use libNewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_d_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "Newsboat.log";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.log_file, Some(tmp.path().join(filename)));
    };

    check(vec!["Newsboat".into(), "-d".into(), arg.clone().into()]);

    check(vec!["Newsboat".into(), format!("--log-file={arg}").into()]);
}

use libnewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_capital_c_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    let filename = "newsboat-config";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.config_file, Some(tmp.path().join(filename)));
    };

    check(vec!["newsboat".into(), "-C".into(), arg.clone().into()]);

    check(vec![
        "newsboat".into(),
        format!("--config-file={}", &arg).into(),
    ]);
}

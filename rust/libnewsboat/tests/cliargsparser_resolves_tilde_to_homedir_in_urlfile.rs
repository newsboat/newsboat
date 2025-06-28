use libnewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_u_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    let filename = "urlfile";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.url_file, Some(tmp.path().join(filename)));
    };

    check(vec!["newsboat".into(), "-u".into(), arg.clone().into()]);

    check(vec![
        "newsboat".into(),
        format!("--url-file={}", &arg).into(),
    ]);
}

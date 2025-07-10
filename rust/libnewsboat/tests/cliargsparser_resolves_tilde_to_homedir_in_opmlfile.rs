use libNewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_i_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "feedlist.opml";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.importfile, Some(tmp.path().join(filename)));
    };

    check(vec!["Newsboat".into(), "-i".into(), arg.clone().into()]);

    check(vec![
        "Newsboat".into(),
        format!("--import-from-opml={}", &arg).into(),
    ]);
}

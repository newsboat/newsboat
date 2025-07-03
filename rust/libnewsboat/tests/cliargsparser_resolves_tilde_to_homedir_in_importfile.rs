use libnewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_capital_i_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    let filename = "read.txt";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.readinfo_import_file, Some(tmp.path().join(filename)));
    };

    check(vec!["newsboat".into(), "-I".into(), arg.clone().into()]);

    check(vec![
        "newsboat".into(),
        format!("--import-from-file={arg}").into(),
    ]);
}

use libNewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_capital_e_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "read.txt";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.readinfo_export_file, Some(tmp.path().join(filename)));
    };

    check(vec!["Newsboat".into(), "-E".into(), arg.clone().into()]);

    check(vec![
        "Newsboat".into(),
        format!("--export-to-file={}", &arg).into(),
    ]);
}

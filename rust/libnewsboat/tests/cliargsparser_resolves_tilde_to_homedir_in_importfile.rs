use libnewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_capital_i_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "read.txt";
    let arg = format!("~/{}", filename);

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.readinfo_import_file, Some(tmp.path().join(filename)));
    };

    check(vec![
        "newsboat".to_string(),
        "-I".to_string(),
        arg.to_string(),
    ]);

    check(vec![
        "newsboat".to_string(),
        "--import-from-file=".to_string() + &arg,
    ]);
}

use libnewsboat::cliargsparser::CliArgsParser;
use std::env;
use tempfile::TempDir;

#[test]
fn t_cliargsparser_dash_c_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    let filename = "mycache.db";
    let arg = format!("~/{filename}");

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(args.cache_file, Some(tmp.path().join(filename)));
        assert_eq!(
            args.lock_file,
            Some(tmp.path().join(filename.to_owned() + ".lock"))
        );
    };

    check(vec!["newsboat".into(), "-c".into(), arg.clone().into()]);

    check(vec![
        "newsboat".into(),
        format!("--cache-file={}", &arg).into(),
    ]);
}

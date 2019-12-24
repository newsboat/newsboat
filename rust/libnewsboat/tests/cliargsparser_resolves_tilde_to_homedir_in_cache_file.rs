extern crate libnewsboat;
extern crate tempfile;

use self::libnewsboat::cliargsparser::CliArgsParser;
use self::tempfile::TempDir;
use std::{env, path::PathBuf};

#[test]
fn t_cliargsparser_dash_c_resolves_tilde_to_homedir() {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    let filename = "mycache.db";
    let arg = format!("~/{}", filename);

    let check = |opts| {
        let args = CliArgsParser::new(opts);
        assert_eq!(
            args.cache_file,
            Some(PathBuf::from(tmp.path().join(filename)))
        );
        assert_eq!(
            args.lock_file,
            Some(PathBuf::from(
                tmp.path().join(filename.to_owned() + ".lock")
            ))
        );
    };

    check(vec![
        "newsboat".to_string(),
        "-c".to_string(),
        arg.to_string(),
    ]);

    check(vec![
        "newsboat".to_string(),
        "--cache-file=".to_string() + &arg,
    ]);
}

use cxx::{type_id, ExternType};
use std::ffi::OsStr;
use std::os::unix::ffi::OsStrExt;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct PathBuf(pub std::path::PathBuf);

unsafe impl ExternType for PathBuf {
    type Id = type_id!("newsboat::filepath::bridged::PathBuf");
    type Kind = cxx::kind::Opaque;
}

#[cxx::bridge(namespace = "newsboat::filepath::bridged")]
mod bridged {
    extern "Rust" {
        type PathBuf;

        fn create_empty() -> Box<PathBuf>;
        fn create(filepath: Vec<u8>) -> Box<PathBuf>;
        fn equals(lhs: &PathBuf, rhs: &PathBuf) -> bool;
        fn less_than(lhs: &PathBuf, rhs: &PathBuf) -> bool;
        fn into_bytes(filepath: &PathBuf) -> Vec<u8>;
        fn display(filepath: &PathBuf) -> String;
        fn push(filepath: &mut PathBuf, component: &PathBuf);
        fn clone(filepath: &PathBuf) -> Box<PathBuf>;
        fn is_absolute(filepath: &PathBuf) -> bool;
        fn set_extension(filepath: &mut PathBuf, extension: Vec<u8>) -> bool;
        fn starts_with(filepath: &PathBuf, base: &PathBuf) -> bool;
        fn file_name(filepath: &PathBuf) -> Vec<u8>;
    }
}

fn create_empty() -> Box<PathBuf> {
    Box::new(PathBuf(std::path::PathBuf::new()))
}

fn create(filepath: Vec<u8>) -> Box<PathBuf> {
    let filepath: &OsStr = OsStrExt::from_bytes(&filepath);
    let filepath = std::path::Path::new(filepath);
    Box::new(PathBuf(filepath.to_path_buf()))
}

fn equals(lhs: &PathBuf, rhs: &PathBuf) -> bool {
    lhs.0 == rhs.0
}

fn less_than(lhs: &PathBuf, rhs: &PathBuf) -> bool {
    lhs.0 < rhs.0
}

fn into_bytes(filepath: &PathBuf) -> Vec<u8> {
    filepath.0.as_os_str().as_bytes().to_owned()
}

fn display(filepath: &PathBuf) -> String {
    format!("{}", filepath.0.display())
}

fn push(filepath: &mut PathBuf, component: &PathBuf) {
    filepath.0.push(&component.0)
}

fn clone(filepath: &PathBuf) -> Box<PathBuf> {
    Box::new(PathBuf(filepath.0.clone()))
}

fn is_absolute(filepath: &PathBuf) -> bool {
    filepath.0.is_absolute()
}

fn set_extension(filepath: &mut PathBuf, extension: Vec<u8>) -> bool {
    filepath.0.set_extension(OsStr::from_bytes(&extension))
}

fn starts_with(filepath: &PathBuf, base: &PathBuf) -> bool {
    filepath.0.starts_with(&base.0)
}

fn file_name(filepath: &PathBuf) -> Vec<u8> {
    if let Some(res) = filepath.0.file_name() {
        res.as_bytes().to_vec()
    } else {
        Vec::new()
    }
}

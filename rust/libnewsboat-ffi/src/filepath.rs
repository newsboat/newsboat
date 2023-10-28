use cxx::{ExternType, type_id};

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
        fn into_bytes(filepath: &PathBuf) -> Vec<u8>;
        fn display(filepath: &PathBuf) -> String;
        fn push(filepath: &mut PathBuf, component: &PathBuf);
        fn clone(filepath: &PathBuf) -> Box<PathBuf>;
        fn is_absolute(filepath: &PathBuf) -> bool;
        fn set_extension(filepath: &mut PathBuf, extension: &str) -> bool;

        // These functions are actually in utils.rs, but I couldn't find a way to return
        // `Box<PathBuf>` from libnewsboat-ffi/src/utils.rs, so I moved the bindings here
        fn get_default_browser() -> Box<PathBuf>;
        fn resolve_tilde(path: &PathBuf) -> Box<PathBuf>;
        fn resolve_relative(reference: &PathBuf, path: &PathBuf) -> Box<PathBuf>;
        fn getcwd() -> Box<PathBuf>;
    }
}

fn create_empty() -> Box<PathBuf> {
    Box::new(PathBuf(std::path::PathBuf::new()))
}

fn create(filepath: Vec<u8>) -> Box<PathBuf> {
    use std::ffi::OsStr;
    use std::os::unix::ffi::OsStrExt;

    let filepath: &OsStr = OsStrExt::from_bytes(&filepath);
    let filepath = std::path::Path::new(filepath);
    Box::new(PathBuf(filepath.to_path_buf()))
}

fn equals(lhs: &PathBuf, rhs: &PathBuf) -> bool {
    lhs.0 == rhs.0
}

fn into_bytes(filepath: &PathBuf) -> Vec<u8> {
    use std::os::unix::ffi::OsStrExt;
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

fn get_default_browser() -> Box<PathBuf> {
    Box::new(PathBuf(libnewsboat::utils::get_default_browser()))
}

fn resolve_tilde(path: &PathBuf) -> Box<PathBuf> {
    Box::new(PathBuf(libnewsboat::utils::resolve_tilde(path.0.clone())))
}

fn resolve_relative(reference: &PathBuf, path: &PathBuf) -> Box<PathBuf> {
    Box::new(PathBuf(libnewsboat::utils::resolve_relative(
        &reference.0,
        &path.0,
    )))
}

fn getcwd() -> Box<PathBuf> {
    let result = libnewsboat::utils::getcwd().unwrap_or_else(|_| std::path::PathBuf::new());
    Box::new(PathBuf(result))
}

fn is_absolute(filepath: &PathBuf) -> bool {
    filepath.0.is_absolute()
}

fn set_extension(filepath: &mut PathBuf, extension: &str) -> bool {
    filepath.0.set_extension(extension)
}

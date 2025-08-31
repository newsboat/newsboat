use std::ffi::OsStr;
use std::os::unix::ffi::OsStrExt;
use std::path::PathBuf;

/// Append `extension` to the filename.
///
/// Return `true` if successful, `false` if `filepath` is empty.
pub fn add_extension(filepath: &mut PathBuf, extension: Vec<u8>) -> bool {
    if extension.is_empty() {
        true
    } else {
        let extension = OsStr::from_bytes(&extension);

        match filepath.extension() {
            None => filepath.set_extension(extension),
            Some(ext) => {
                let mut ext = ext.to_os_string();
                ext.push(OsStr::new("."));
                ext.push(extension);
                filepath.set_extension(ext)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::path::Path;

    #[test]
    fn t_examples_from_rust_lang_docs() {
        // Copied from https://doc.rust-lang.org/std/path/struct.PathBuf.html#method.add_extension

        let mut p = PathBuf::from("/feel/the");

        add_extension(&mut p, "formatted".as_bytes().to_vec());
        assert_eq!(Path::new("/feel/the.formatted"), p.as_path());

        add_extension(&mut p, "dark.side".as_bytes().to_vec());
        assert_eq!(Path::new("/feel/the.formatted.dark.side"), p.as_path());

        p.set_extension("cookie");
        assert_eq!(Path::new("/feel/the.formatted.dark.cookie"), p.as_path());

        p.set_extension("");
        assert_eq!(Path::new("/feel/the.formatted.dark"), p.as_path());

        add_extension(&mut p, "".as_bytes().to_vec());
        assert_eq!(Path::new("/feel/the.formatted.dark"), p.as_path());
    }
}

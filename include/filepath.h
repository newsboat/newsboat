#ifndef NEWSBOAT_FILEPATH_H_
#define NEWSBOAT_FILEPATH_H_

#include "libnewsboat-ffi/src/filepath.rs.h"

#include <cstdint>
#include <string>

namespace newsboat {

/// A path in the file system.
///
/// This is a thin wrapper over Rust's `PathBuf`. The interface also mimics
/// that of `PathBuf`, for ease of migration.
/// https://doc.rust-lang.org/nightly/std/path/struct.PathBuf.html
class Filepath {
public:
	/// Constructs an empty path.
	Filepath();

	/// Constructs a filepath from string in locale encoding.
	///
	/// \note This does not perform any canonicalization, i.e. it does nothing
	/// to tilde (~), repeated path separators (path///to/file), symbolic links
	/// etc.
	static Filepath from_locale_string(const std::string&);

	/// Returns the filepath as a string in locale encoding.
	///
	/// This is just bytes showed into `std::string`, no conversions are
	/// performed.
	std::string to_locale_string() const;

	/// Returns the filepath interpreted as UTF-8 string, with U+FFFD
	/// REPLACEMENT CHARACTER for any non-Unicode data.
	std::string display() const;

	/// Extends this path with a new component.
	///
	/// If component is an absolute path, it replaces the current path.
	void push(const Filepath& component);

	/// Returns a copy of this path.
	Filepath clone() const;

	Filepath(const Filepath&) = delete;
	Filepath& operator=(const Filepath&) = delete;
	Filepath(Filepath&&) = default;
	Filepath& operator=(Filepath&&) = default;

	bool operator==(const Filepath&) const;
	bool operator!=(const Filepath&) const;

private:
	rust::Box<filepath::bridged::PathBuf> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEPATH_H_ */

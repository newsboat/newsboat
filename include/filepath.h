#ifndef NEWSBOAT_FILEPATH_H_
#define NEWSBOAT_FILEPATH_H_

#include "libnewsboat-ffi/src/filepath.rs.h"

#include <cstdint>
#include <string>

#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS
#ifdef ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#endif

namespace newsboat {

/// A path in the file system.
///
/// This is a thin wrapper over Rust's `PathBuf`. The interface also mimics
/// that of `PathBuf`, for ease of migration.
/// https://doc.rust-lang.org/nightly/std/path/struct.PathBuf.html
class Filepath {
public:
#ifdef ENABLE_IMPLICIT_FILEPATH_CONVERSIONS
	// FIXME: remove these once the codebase is fully migrated from std::string
	// to Filepath

	Filepath(const std::string& input)
		: rs_object(std::move(Filepath::from_locale_string(input).rs_object))
	{
	}

	operator const filepath::bridged::PathBuf& () const
	{
		return *rs_object;
	}
#endif

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

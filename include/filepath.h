#ifndef NEWSBOAT_FILEPATH_H_
#define NEWSBOAT_FILEPATH_H_

#include "libnewsboat-ffi/src/filepath.rs.h"

#include <optional>
#include <ostream>
#include <string>

namespace newsboat {

/// A path in the file system.
///
/// This is a thin wrapper over Rust's `PathBuf`. The interface also mimics
/// that of `PathBuf`, for ease of migration.
/// https://doc.rust-lang.org/nightly/std/path/struct.PathBuf.html
class Filepath {
public:
	Filepath(rust::Box<filepath::bridged::PathBuf>&& rs_object)
		: rs_object(std::move(rs_object))
	{
	}

	operator const filepath::bridged::PathBuf& () const
	{
		return *rs_object;
	}

	/// Constructs an empty path.
	Filepath();

	/// Constructs a filepath from string in locale encoding.
	///
	/// \note This does not perform any canonicalization, i.e. it does nothing
	/// to tilde (~), repeated path separators (path///to/file), symbolic links
	/// etc.
	static Filepath from_locale_string(const std::string&);

	Filepath(Filepath&&) = default;
	Filepath& operator=(Filepath&&) = default;

	Filepath(const Filepath& filepath);
	Filepath& operator=(const Filepath& filepath);

	bool operator==(const Filepath&) const;
	bool operator!=(const Filepath&) const;
	bool operator<(const Filepath&) const;
	bool operator<=(const Filepath&) const;
	bool operator>(const Filepath&) const;
	bool operator>=(const Filepath&) const;

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

	/// Creates a new Filepath with a given component appended to the current
	/// path (with a separator in between).
	Filepath join(const Filepath& component) const;

	// Return `true` if path is absolute.
	bool is_absolute() const;

	// Return `false` and do nothing if Filepath is empty, set extension and
	// return `true` otherwise.
	//
	// See also `add_extension()`.
	//
	// \note `ext` is interpreted as bytes in locale encoding.
	bool set_extension(const std::string& ext);

	// Return `false` and do nothing if Filepath is empty, append extension and
	// return `true` otherwise.
	//
	// The difference from `set_extension()` is that `set_extension()` replaces
	// the existing extension while `add_extension()` appends to it. For the
	// path "foo.tar", `set_extension("gz")` will turn the path into "foo.gz",
	// while `add_extension("gz")` will turn the path into "foo.tar.gz".
	//
	// \note `ext` is interpreted as bytes in locale string.
	bool add_extension(const std::string& ext);

	// Return `true` if Filepath start with `base`, `false` otherwise.
	//
	// Only considers whole path components to match, i.e. "/foo" is **not**
	// a prefix of "/foobar/baz".
	bool starts_with(const Filepath& base) const;

	/// Returns the final component of the path, if there is one.
	std::optional<Filepath> file_name() const;

private:
	rust::Box<filepath::bridged::PathBuf> rs_object;
};

inline Filepath operator""_path(const char* filepath, size_t filepath_length)
{
	return newsboat::Filepath::from_locale_string(std::string(filepath, filepath_length));
}

} // namespace newsboat

// Used in Catch2's INFO macro.
inline std::ostream& operator<<(std::ostream& out, const newsboat::Filepath& p)
{
	out << p.display();
	return out;
}

#endif /* NEWSBOAT_FILEPATH_H_ */

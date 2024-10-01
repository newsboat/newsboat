#ifndef NEWSBOAT_FILEPATH_H_
#define NEWSBOAT_FILEPATH_H_

#include "libnewsboat-ffi/src/filepath.rs.h"
#include "3rd-party/optional.hpp"

#include <cstdint>
#include <ostream>
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

	Filepath(const char* input)
		: rs_object(std::move(Filepath::from_locale_string(std::string(input)).rs_object))
	{
	}

	Filepath(const std::string& input)
		: rs_object(std::move(Filepath::from_locale_string(input).rs_object))
	{
	}

	Filepath(rust::Box<filepath::bridged::PathBuf> rs_object)
		: rs_object(std::move(rs_object))
	{
	}

	operator std::string() const
	{
		return to_locale_string();
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

	Filepath(Filepath&&) = default;
	Filepath& operator=(Filepath&&) = default;

	Filepath(const Filepath& filepath);
	Filepath& operator=(const Filepath& filepath);

	bool operator==(const Filepath&) const;
	bool operator!=(const Filepath&) const;

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
	// \note `ext` is interpreted as bytes in locale encoding.
	bool set_extension(const std::string& ext);

	// Return `true` if Filepath start with `str`, `false` otherwise.
	bool starts_with(const std::string& str) const;

	/// Returns the final component of the path, if there is one.
	nonstd::optional<Filepath> file_name() const;

private:
	rust::Box<filepath::bridged::PathBuf> rs_object;
};

} // namespace newsboat

// Used in Catch2's INFO macro.
inline std::ostream& operator<<(std::ostream& out, const newsboat::Filepath& p)
{
	out << p.display();
	return out;
}

#ifdef ENABLE_IMPLICIT_FILEPATH_CONVERSIONS
inline bool operator==(const std::string& lhs, const newsboat::Filepath& rhs)
{
	return lhs == rhs.to_locale_string();
}

#include "3rd-party/optional.hpp"
inline bool operator==(const nonstd::optional<std::string>& lhs,
	const newsboat::Filepath& rhs)
{
	return lhs && lhs.value() == rhs;
}
#endif

#endif /* NEWSBOAT_FILEPATH_H_ */

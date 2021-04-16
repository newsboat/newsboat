#ifndef NEWSBOAT_UTF8STRING_H_
#define NEWSBOAT_UTF8STRING_H_

#include <string>

namespace newsboat {

/// A string that's guaranteed to contain valid UTF-8.
class Utf8String {
public:
	/// Construct an object from a string literal, which is assumed to be in
	/// UTF-8 since Newsboat's source code is in UTF-8.
	///
	/// \note This actually allows construction from a char array, which is not
	/// intentional — it's just that C++ won't let us disallow that. Hopefully
	/// that will never happen.
	// Lifted from https://stackoverflow.com/a/13724458/2350060
	template<size_t N> Utf8String(const char (&input)[N])
	// -1 to exclude the terminating NUL
		: Utf8String(std::string(input, N-1))
	{
	}

	/// Construct an object from a UTF-8 `std::string`.
	///
	/// \note This doesn't check if the string is actually in UTF-8.
	static Utf8String from_utf8(std::string input);

	/// Construct an object from a string in the locale charset.
	///
	/// Invalid characters will be replaced by a question mark.
	static Utf8String from_locale_charset(std::string input);

	/// Return inner UTF-8 string.
	const std::string& to_utf8() const
	{
		return inner;
	}

	/// Return the string re-coded to the locale charset.
	///
	/// Invalid characters will be replaces by a question mark.
	std::string to_locale_charset() const;

	Utf8String(const Utf8String&) = default;
	Utf8String(Utf8String&&) = default;
	Utf8String& operator=(const Utf8String&) = default;
	Utf8String& operator=(Utf8String&&) = default;

	friend bool operator==(const Utf8String& lhs, const Utf8String& rhs);
	friend bool operator!=(const Utf8String& lhs, const Utf8String& rhs);

private:
	explicit Utf8String(std::string input);

	// UTF-8 encoded string
	std::string inner;
};

inline bool operator==(const Utf8String& lhs, const Utf8String& rhs)
{
	return lhs.inner == rhs.inner;
}

inline bool operator!=(const Utf8String& lhs, const Utf8String& rhs)
{
	return lhs.inner != rhs.inner;
}

} // namespace newsboat

#endif /* NEWSBOAT_UTF8STRING_H_ */


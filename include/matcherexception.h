#ifndef NEWSBOAT_MATCHEREXCEPTON_H_
#define NEWSBOAT_MATCHEREXCEPTON_H_

#include <stdexcept>
#include <string>

#include "utf8string.h"

namespace newsboat {

struct MatcherErrorFfi;

class MatcherException : public std::exception {
public:
	// Numbers here MUST match constants in rust/libnewsboat-ffi/src/matchererror.rs
	enum class Type : std::uint8_t { ATTRIB_UNAVAIL = 0, INVALID_REGEX = 1 };

	MatcherException(Type et,
		const std::string& info,
		const std::string& info2 = "")
		: type_(et)
		, addinfo(Utf8String::from_utf8(info))
		, addinfo2(Utf8String::from_utf8(info2))
	{
	}

	~MatcherException() throw() override {}
	const char* what() const throw() override;

	static MatcherException from_rust_error(MatcherErrorFfi error);

	// Getters for testing purposes. Ugly, but alas.
	Type type() const
	{
		return type_;
	}

	std::string info() const
	{
		return addinfo.to_utf8();
	}

	std::string info2() const
	{
		return addinfo2.to_utf8();
	}

private:
	Type type_;
	Utf8String addinfo;
	Utf8String addinfo2;
};

/// A description of an error returned by Rust. This can be converted into
/// `MatcherException` object with `MatcherException::from_rust_error`
struct MatcherErrorFfi {
	MatcherException::Type type;
	char* info;
	char* info2;
};

} // namespace newsboat

#endif /* NEWSBOAT_MATCHEREXCEPTON_H_ */

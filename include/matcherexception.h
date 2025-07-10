#ifndef NEWSBOAT_MATCHEREXCEPTON_H_
#define NEWSBOAT_MATCHEREXCEPTON_H_

#include <cstdint>
#include <string>

namespace Newsboat {

struct MatcherErrorFfi;

class MatcherException : public std::exception {
public:
	// Numbers here MUST match constants in rust/libNewsboat-ffi/src/matchererror.rs
	enum class Type : std::uint8_t { ATTRIB_UNAVAIL = 0, INVALID_REGEX = 1 };

	MatcherException(Type et,
		const std::string& info,
		const std::string& info2 = "")
		: type_(et)
		, addinfo(info)
		, addinfo2(info2)
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
		return addinfo;
	}

	std::string info2() const
	{
		return addinfo2;
	}

private:
	Type type_;
	std::string addinfo;
	std::string addinfo2;
};

/// A description of an error returned by Rust. This can be converted into
/// `MatcherException` object with `MatcherException::from_rust_error`
struct MatcherErrorFfi {
	MatcherException::Type type;
	char* info;
	char* info2;
};

} // namespace Newsboat

#endif /* NEWSBOAT_MATCHEREXCEPTON_H_ */

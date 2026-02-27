#ifndef NEWSBOAT_MATCHEREXCEPTON_H_
#define NEWSBOAT_MATCHEREXCEPTON_H_

#include <string>

#include "libnewsboat-ffi/src/matchererror.rs.h"

namespace newsboat {

struct MatcherErrorFfi;

class MatcherException : public std::exception {
public:
	using Type = matchererror::bridged::Type;

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

	static MatcherException from_rust_error(const matchererror::bridged::MatcherError& error);

	// Getters for testing purposes. Ugly, but alas.
	Type type() const
	{
		return type_;
	}

	const std::string& info() const
	{
		return addinfo;
	}

	const std::string& info2() const
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

} // namespace newsboat

#endif /* NEWSBOAT_MATCHEREXCEPTON_H_ */

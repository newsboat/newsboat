#ifndef NEWSBOAT_STRPRINTF_H_
#define NEWSBOAT_STRPRINTF_H_

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace Newsboat {

namespace strprintf {
namespace detail {
template<typename T, typename... Args>
std::string fmt_impl(const std::string& format, const T& argument,
	Args... args);
}

std::pair<std::string, std::string> split_format(
	const std::string& printf_format);

std::string fmt(const std::string& format);

template<typename... Args>
std::string fmt(const std::string& format, const char* argument, Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const int32_t argument, Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const std::uint32_t argument,
	Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const int64_t argument, Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const uint64_t argument,
	Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const void* argument, Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const std::nullptr_t argument,
	Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const float argument, Args... args)
{
	// Variadic functions (like snprintf) do not accept `float`, so let's
	// convert that.
	return detail::fmt_impl(format, static_cast<double>(argument), args...);
}

template<typename... Args>
std::string fmt(const std::string& format, const double argument, Args... args)
{
	return detail::fmt_impl(format, argument, args...);
}

template<typename... Args>
std::string fmt(const std::string& format,
	const std::string& argument,
	Args... args)
{
	return fmt(format, argument.c_str(), args...);
}

template<typename... Args>
std::string fmt(const std::string& format,
	const std::string* argument,
	Args... args)
{
	return fmt(format, argument->c_str(), args...);
}

namespace detail {
template<typename T, typename... Args>
std::string fmt_impl(const std::string& format, const T& argument, Args... args)
{
	std::string local_format, remaining_format;
	std::tie(local_format, remaining_format) = split_format(format);

	char buffer[1024];
	std::string result;
	unsigned int len = 1 +
		snprintf(buffer,
			sizeof(buffer),
			local_format.c_str(),
			argument);
	// snprintf returns the length of the formatted string. If it's
	// longer than the buffer size, we have to enlarge the buffer in
	// order to get the whole result.
	//
	// `<=` is correct since we've added 1 to `len`, above. We have to
	// do it because `buffer` has to fit not only the string but the
	// terminating null byte as well.
	if (len <= sizeof(buffer)) {
		result = buffer;
	} else {
		std::vector<char> buf(len);
		snprintf(
			buf.data(), len, local_format.c_str(), argument);
		result = buf.data();
	}

	return result + fmt(remaining_format, args...);
}
}
};

} // namespace Newsboat

#endif /* NEWSBOAT_STRPRINTF_H_ */

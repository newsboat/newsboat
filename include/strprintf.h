#ifndef NEWSBOAT_STRPRINTF_H_
#define NEWSBOAT_STRPRINTF_H_

#include <memory>
#include <string>
#include <vector>

namespace newsboat {

namespace strprintf {
	namespace {
		template<typename T, typename... Args>
		std::string
		fmt_impl(const std::string& format, const T& argument, Args... args);
	}

	std::pair<std::string, std::string> split_format(
		const std::string& printf_format);

	std::string fmt(const std::string& format);

	template<typename... Args>
	std::string
	fmt(const std::string& format, const char* argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const int argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const unsigned int argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const long int argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const long unsigned int argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const void* argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const std::nullptr_t argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const double argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
	}

	template<typename... Args>
	std::string
	fmt(const std::string& format, const float argument, Args... args)
	{
		return fmt_impl(format, argument, args...);
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

	namespace {
		template<typename T, typename... Args>
		std::string
		fmt_impl(const std::string& format, const T& argument, Args... args)
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

} // namespace newsboat

#endif /* NEWSBOAT_STRPRINTF_H_ */

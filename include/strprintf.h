#ifndef NEWSBOAT_STRPRINTF_H_
#define NEWSBOAT_STRPRINTF_H_

#include <memory>
#include <string>

namespace newsboat {

namespace strprintf {
	std::pair<std::string, std::string> split_format(
		const std::string& printf_format);

	std::string fmt(const std::string& format);

	template<typename T, typename... Args> std::string fmt(const std::string& format, const T& argument, Args... args);
	template<typename... Args> std::string fmt(const std::string& format, const std::string& argument, Args... args);
	template<typename... Args> std::string fmt(const std::string& format, const std::string* argument, Args... args);

	template<typename T, typename... Args>
	std::string
	fmt(const std::string& format, const T& argument, Args... args)
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
			std::unique_ptr<char> buf(new char[len]);
			snprintf(
				buf.get(), len, local_format.c_str(), argument);
			result = buf.get();
		}

		return result + fmt(remaining_format, args...);
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
		return fmt(format, *argument, args...);
	}

};

} // namespace newsboat

#endif /* NEWSBOAT_STRPRINTF_H_ */

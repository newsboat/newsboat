#ifndef NEWSBEUTER_STRPRINTF_H_
#define NEWSBEUTER_STRPRINTF_H_

#include <memory>
#include <string>

namespace newsbeuter {

class strprintf {
	public:
		static std::pair<std::string, std::string>
			split_format(const std::string& printf_format);

		static std::string fmt(const std::string& format) {
			return format;
		}

		template<typename... Args>
		static std::string fmt(
				const std::string& format, const std::string& argument, Args... args)
		{
			return fmt(format, argument.c_str(), args...);
		}

		template<typename T, typename... Args>
		static std::string fmt(
				const std::string& format, const T& argument, Args... args)
		{
			std::string local_format, remaining_format;
			std::tie(local_format, remaining_format) = split_format(format);

			char buffer[1024];
			std::string result;
			unsigned int len = 1 + snprintf(
					buffer, sizeof(buffer), local_format.c_str(), argument);
			if (len <= sizeof(buffer)) {
				result = buffer;
			} else {
				std::unique_ptr<char> buf(new char[len]);
				snprintf(buf.get(), len, local_format.c_str(), argument);
				result = *buf;
			}

			return result + fmt(remaining_format, args...);
		}
};

}

#endif /* NEWSBEUTER_STRPRINTF_H_ */

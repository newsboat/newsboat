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
			char buffer[1024];
			std::string result;
			// Empty string is a dummy value that we pass in order to silence
			// Clang's warning about format not being a literal.
			//
			// The thing is, at this point we know *for sure* that the format
			// either contains no formats at all, or only escaped percent signs
			// (which don't require any additional arguments to snprintf). It's
			// just the way fmt recurses. The only reason we're calling
			// snprintf at all is to process these escaped percent signs, if
			// any. So we don't need additional parameters.
			unsigned int len = 1 + snprintf(
					buffer, sizeof(buffer), format.c_str(), "");
			if (len <= sizeof(buffer)) {
				result = buffer;
			} else {
				std::unique_ptr<char> buf(new char[len]);
				snprintf(buf.get(), len, format.c_str(), "");
				result = *buf;
			}
			return result;
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

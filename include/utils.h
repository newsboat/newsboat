#ifndef UTIL_H_
#define UTIL_H_

#include <vector>
#include <string>

#include <rss.h>

namespace newsbeuter {

class utils {
	public:
		static std::vector<std::string> tokenize(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_spaced(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_nl(const std::string& str, std::string delimiters = "\r\n");
		static std::vector<std::string> tokenize_quoted(const std::string& str, std::string delimiters = " \r\n\t");

		static bool try_fs_lock(const std::string& lock_file, pid_t & pid);
		static void remove_fs_lock(const std::string& lock_file);

		static std::string convert_text(const std::string& text, const std::string& tocode, const std::string& fromcode);

		static std::string get_command_output(const std::string& cmd);
		static void extract_filter(const std::string& line, std::string& filter, std::string& url);
		static std::string retrieve_url(const std::string& url, const char * user_agent = 0);
		static std::string run_filter(const std::string& cmd, const std::string& input);
		static void run_command(const std::string& cmd, const std::string& param); // used for notifications only
};

}

#endif /*UTIL_H_*/

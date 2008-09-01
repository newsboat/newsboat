#ifndef UTIL_H_
#define UTIL_H_

#include <vector>
#include <string>

#include <logger.h>
#include <rss.h>

namespace newsbeuter {

class utils {
	public:
		static std::vector<std::string> tokenize(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_spaced(const std::string& str, std::string delimiters = " \r\n\t");
		static std::vector<std::string> tokenize_nl(const std::string& str, std::string delimiters = "\r\n");
		static std::vector<std::string> tokenize_quoted(const std::string& str, std::string delimiters = " \r\n\t");

		static std::vector<std::wstring> wtokenize(const std::wstring& str, std::wstring delimiters = L" \r\n\t");

		static bool try_fs_lock(const std::string& lock_file, pid_t & pid);
		static void remove_fs_lock(const std::string& lock_file);

		static std::string convert_text(const std::string& text, const std::string& tocode, const std::string& fromcode);

		static std::string get_command_output(const std::string& cmd);
		static void extract_filter(const std::string& line, std::string& filter, std::string& url);
		static std::string retrieve_url(const std::string& url, const char * user_agent = 0, const char * auth = 0, int download_timeout = 30);
		static void run_command(const std::string& cmd, const std::string& param); // used for notifications only
		static std::string run_program(char * argv[], const std::string& input);

		static std::string resolve_tilde(const std::string& );
		static std::string replace_all(std::string str, const std::string& from, const std::string& to);

		static std::wstring str2wstr(const std::string& str);
		static std::string wstr2str(const std::wstring& wstr);

		static std::wstring utf8str2wstr(const std::string& utf8str);

		static std::string to_s(unsigned int u);

		static std::string absolute_url(const std::string& url, const std::string& link);

		static std::string strprintf(const char * format, ...);

		static std::string get_useragent(configcontainer * cfgcont);

		static size_t strwidth(const std::string& s);

		static inline unsigned int max(unsigned int a, unsigned int b) { return (a > b) ? a : b; }

		static unsigned int to_u(const std::string& str);

		static bool is_valid_color(const std::string& color);
		static bool is_valid_attribute(const std::string& attrib);

	private:
		static void append_escapes(std::string& str, char c);

};

class scope_measure {
	public:
		scope_measure(const std::string& func, loglevel ll = LOG_DEBUG);
		~scope_measure();
		void stopover(const std::string& son = "");
	private:
		struct timeval tv1, tv2;
		std::string funcname;
		loglevel lvl;
};

}

#endif /*UTIL_H_*/

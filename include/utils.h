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
		static void planet_generate_html(std::vector<rss_feed>& feeds, std::vector<rss_item>& items, const std::string& tmplfile, const std::string& outfile);

		static bool try_fs_lock(const std::string& lock_file, pid_t & pid);
		static void remove_fs_lock(const std::string& lock_file);
};

}

#endif /*UTIL_H_*/

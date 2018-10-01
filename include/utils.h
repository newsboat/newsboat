#ifndef NEWSBOAT_UTIL_H_
#define NEWSBOAT_UTIL_H_

#include <curl/curl.h>
#include <libxml/parser.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "configcontainer.h"
#include "logger.h"

namespace newsboat {

// wrapped curl handle for Exception safety and so on
// see also: https://github.com/gsauthof/ccurl
class CurlHandle {
private:
	CURL* h;
	CurlHandle(const CurlHandle&);
	CurlHandle& operator=(const CurlHandle&);

public:
	CurlHandle()
		: h(0)
	{
		h = curl_easy_init();
		if (!h)
			throw std::runtime_error("Can't obtain curl handle");
	}
	~CurlHandle()
	{
		curl_easy_cleanup(h);
	}
	CURL* ptr()
	{
		return h;
	}
};

class Utils {
public:
	static std::vector<std::string> tokenize(const std::string& str,
		std::string delimiters = " \r\n\t");
	static std::vector<std::string> tokenize_spaced(const std::string& str,
		std::string delimiters = " \r\n\t");
	static std::vector<std::string> tokenize_nl(const std::string& str,
		std::string delimiters = "\r\n");
	static std::vector<std::string> tokenize_quoted(const std::string& str,
		std::string delimiters = " \r\n\t");

	static std::string consolidate_whitespace(const std::string& str,
		std::string whitespace = " \r\n\t");

	static std::vector<std::wstring> wtokenize(const std::wstring& str,
		std::wstring delimiters = L" \r\n\t");

	static std::string translit(const std::string& tocode,
		const std::string& fromcode);
	static std::string convert_text(const std::string& text,
		const std::string& tocode,
		const std::string& fromcode);

	static std::string get_command_output(const std::string& cmd);
	static void extract_filter(const std::string& line,
		std::string& filter,
		std::string& url);
	static std::string retrieve_url(const std::string& url,
		ConfigContainer* cfgcont = nullptr,
		const std::string& authinfo = "",
		const std::string* postdata = nullptr,
		CURL* easyhandle = nullptr);
	static void run_command(const std::string& cmd,
		const std::string& param); // used for notifications only
	static std::string run_program(char* argv[], const std::string& input);

	static std::string resolve_tilde(const std::string&);
	static std::string replace_all(std::string str,
		const std::string& from,
		const std::string& to);

	static std::wstring str2wstr(const std::string& str);
	static std::string wstr2str(const std::wstring& wstr);

	static std::wstring clean_nonprintable_characters(std::wstring text);

	static std::wstring utf8str2wstr(const std::string& utf8str);

	static std::string absolute_url(const std::string& url,
		const std::string& link);

	static std::string get_useragent(ConfigContainer* cfgcont);

	static size_t strwidth(const std::string& s);
	static size_t strwidth_Stfl(const std::string& str);
	static size_t wcswidth_Stfl(const std::wstring& str, size_t size);

	static std::string substr_with_width(const std::string& str,
		const size_t max_width);

	static unsigned int max(unsigned int a, unsigned int b)
	{
		return (a > b) ? a : b;
	}

	static unsigned int to_u(const std::string& str)
	{
		return to_u(str, 0);
	}
	static unsigned int to_u(const std::string& str,
		const unsigned int default_value);

	static bool is_valid_color(const std::string& color);
	static bool is_valid_attribute(const std::string& attrib);

	static std::vector<std::pair<unsigned int, unsigned int>>
	partition_indexes(unsigned int start,
		unsigned int end,
		unsigned int parts);

	static std::string join(const std::vector<std::string>& strings,
		const std::string& separator);

	static std::string censor_url(const std::string& url);

	static std::string quote_for_Stfl(std::string str);

	static void trim_end(std::string& str);

	static void trim(std::string& str);

	static std::string quote(const std::string& str);

	static unsigned int get_random_value(unsigned int max);

	static std::string quote_if_necessary(const std::string& str);

	static void set_common_curl_options(CURL* handle, ConfigContainer* cfg);

	static curl_proxytype get_proxy_type(const std::string& type);
	static unsigned long get_auth_method(const std::string& type);

	static bool is_special_url(const std::string& url);
	static bool is_http_url(const std::string& url);
	static bool is_query_url(const std::string& url);
	static bool is_filter_url(const std::string& url);
	static bool is_exec_url(const std::string& url);

	static std::string get_content(xmlNode* node);
	static std::string get_basename(const std::string& url);

	static std::string escape_url(const std::string& url);
	static std::string unescape_url(const std::string& url);
	static void initialize_ssl_implementation(void);

	static unsigned int gentabs(const std::string& str);

	static int mkdir_parents(const std::string& pathname,
		mode_t mode = 0755);

	static std::string make_title(const std::string& url);

	static int run_interactively(const std::string& command,
		const std::string& caller);

	static std::string getcwd();

	static int strnaturalcmp(const std::string& a, const std::string& b);

	static void remove_soft_hyphens(std::string& text);

	static bool is_valid_podcast_type(const std::string& mimetype);

	static std::string get_default_browser();

private:
	static void append_escapes(std::string& str, char c);
};

class ScopeMeasure {
public:
	ScopeMeasure(const std::string& func, Level ll = Level::DEBUG);
	~ScopeMeasure();
	void stopover(const std::string& son = "");

private:
	struct timeval tv1, tv2;
	std::string funcname;
	Level lvl;
};

} // namespace newsboat

#endif /* NEWSBOAT_UTIL_H_ */

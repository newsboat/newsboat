#ifndef UTIL_H_
#define UTIL_H_

#include <vector>
#include <string>
#include <stdexcept>

#include <logger.h>
#include <curl/curl.h>
#include <configcontainer.h>
#include <libxml/parser.h>

namespace newsbeuter {

// wrapped curl handle for exception safety and so on
// see also: https://github.com/gsauthof/ccurl
class curl_handle {
	private:
		CURL *h;
		curl_handle(const curl_handle &);
		curl_handle &operator=(const curl_handle &);
	public:
		curl_handle()
			: h(0)
		{
			h = curl_easy_init();
			if (!h)
				throw std::runtime_error("Can't obtain curl handle");
		}
		~curl_handle()
		{
			curl_easy_cleanup(h);
		}
		CURL *ptr() { return h; }
};

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
		static std::string retrieve_url(const std::string& url, configcontainer * cfgcont = NULL, const char * authinfo = NULL, const std::string* postdata = NULL);
		static void run_command(const std::string& cmd, const std::string& param); // used for notifications only
		static std::string run_program(char * argv[], const std::string& input);

		static std::string resolve_tilde(const std::string& );
		static std::string replace_all(std::string str, const std::string& from, const std::string& to);

		static std::wstring str2wstr(const std::string& str);
		static std::string wstr2str(const std::wstring& wstr);

		static std::wstring clean_nonprintable_characters(std::wstring text);

		static std::wstring utf8str2wstr(const std::string& utf8str);

		template<class T>
		static std::string to_string(T var);

		static std::string absolute_url(const std::string& url, const std::string& link);

		static std::string strprintf(const char * format, ...);

		static std::string get_useragent(configcontainer * cfgcont);

		static size_t strwidth(const std::string& s);
		static size_t strwidth_stfl(const std::string& str);
		static size_t wcswidth_stfl(const std::wstring& str, size_t size);

		static inline unsigned int max(unsigned int a, unsigned int b) { return (a > b) ? a : b; }

		static unsigned int to_u(const std::string& str);

		static bool is_valid_color(const std::string& color);
		static bool is_valid_attribute(const std::string& attrib);

		static std::vector<std::pair<unsigned int, unsigned int> > partition_indexes(unsigned int start, unsigned int end, unsigned int parts);

		static std::string join(const std::vector<std::string>& strings, const std::string& separator);

		static std::string censor_url(const std::string& url);

		static std::string quote_for_stfl(std::string str);

		static void trim_end(std::string& str);

		static void trim(std::string& str);

		static std::string quote(const std::string& str);

		static unsigned int get_random_value(unsigned int max);

		static std::string quote_if_necessary(const std::string& str);

		static void set_common_curl_options(CURL * handle, configcontainer * cfg);

		static curl_proxytype get_proxy_type(const std::string& type);
		static unsigned long get_auth_method(const std::string& type);

		static bool is_special_url(const std::string& url);

		static bool is_http_url(const std::string& url);

		static std::string get_content(xmlNode * node);
		static std::string get_prop(xmlNode * node, const char * prop, const char * ns = NULL);

		static std::string escape_url(const std::string& url);
		static std::string unescape_url(const std::string& url);
		static void initialize_ssl_implementation(void);

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

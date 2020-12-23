#ifndef NEWSBOAT_UTIL_H_
#define NEWSBOAT_UTIL_H_

#include <cstdint>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "3rd-party/expected.hpp"
#include "3rd-party/optional.hpp"

#include "configcontainer.h"
#include "logger.h"

#include "utils.rs.h"

namespace newsboat {

// Forward declaration for a type from htmlrenderer.h
enum class LinkType;

namespace utils {

enum class HTTPMethod {
	GET = 0,
	POST,
	PUT,
	DELETE
};

std::string strip_comments(const std::string& line);
std::vector<std::string> tokenize(const std::string& str,
	std::string delimiters = " \r\n\t");
std::vector<std::string> tokenize_spaced(const std::string& str,
	std::string delimiters = " \r\n\t");
std::vector<std::string> tokenize_nl(const std::string& str,
	std::string delimiters = "\r\n");
std::vector<std::string> tokenize_quoted(const std::string& str,
	std::string delimiters = " \r\n\t");
nonstd::optional<std::string> extract_token_quoted(std::string& str,
	std::string delimiters = " \r\n\t");

std::string consolidate_whitespace(const std::string& str);

std::string translit(const std::string& tocode,
	const std::string& fromcode);
std::string convert_text(const std::string& text,
	const std::string& tocode,
	const std::string& fromcode);

/// Converts input string from UTF-8 to the locale's encoding (as detected by
/// nl_langinfo(CODESET)).
std::string utf8_to_locale(const std::string& text);

std::string get_command_output(const std::string& cmd);
std::string http_method_str(const HTTPMethod method);
std::string retrieve_url(const std::string& url,
	ConfigContainer* cfgcont = nullptr,
	const std::string& authinfo = "",
	const std::string* body = nullptr,
	const HTTPMethod method = HTTPMethod::GET,
	CURL* easyhandle = nullptr);
std::string run_program(const char* argv[], const std::string& input);

std::string resolve_tilde(const std::string&);
std::string resolve_relative(const std::string&, const std::string&);
std::string replace_all(std::string str,
	const std::string& from,
	const std::string& to);
std::string replace_all(const std::string& str,
	const std::vector<std::pair<std::string, std::string>> from_to_pairs);

std::wstring str2wstr(const std::string& str);
std::string wstr2str(const std::wstring& wstr);

std::wstring clean_nonprintable_characters(std::wstring text);

std::string absolute_url(const std::string& url,
	const std::string& link);

std::string get_useragent(ConfigContainer* cfgcont);

std::string substr_with_width(const std::string& str,
	const size_t max_width);

std::string substr_with_width_stfl(const std::string& str,
	const size_t max_width);

unsigned int to_u(const std::string& str,
	const unsigned int default_value = 0);

std::vector<std::pair<unsigned int, unsigned int>> partition_indexes(
		unsigned int start,
		unsigned int end,
		unsigned int parts);

std::string join(const std::vector<std::string>& strings,
	const std::string& separator);

std::string censor_url(const std::string& url);

std::string quote_for_stfl(std::string str);

void trim_end(std::string& str);

void trim(std::string& str);

std::string quote(const std::string& str);

std::string quote_if_necessary(const std::string& str);

void set_common_curl_options(CURL* handle, ConfigContainer* cfg);

curl_proxytype get_proxy_type(const std::string& type);

std::string get_content(xmlNode* node);
std::string get_basename(const std::string& url);

std::string unescape_url(const std::string& url);
void initialize_ssl_implementation(void);

int mkdir_parents(const std::string& pathname,
	mode_t mode = 0755);

std::string make_title(const std::string& url);

nonstd::optional<std::uint8_t> run_interactively(const std::string& command,
	const std::string& caller);

nonstd::optional<std::uint8_t> run_non_interactively(const std::string& command,
	const std::string& caller);

std::string getcwd();

enum class ReadTextFileErrorKind {
	CantOpen,
	LineError
};
struct ReadTextFileError {
	ReadTextFileErrorKind kind;
	std::string message;
};
// We define an alias for this to work around a bug in AStyle: the poor
// formatter can't decide how to split the function declaration into multiple
// lines, and re-formats it differently on each invocation.
using ReadTextFileResult =
	nonstd::expected<std::vector<std::string>, ReadTextFileError>;
ReadTextFileResult read_text_file(const std::string& filename);

void remove_soft_hyphens(std::string& text);

bool is_valid_podcast_type(const std::string& mimetype);

nonstd::optional<LinkType> podcast_mime_to_link_type(const std::string&
	mimetype);

std::string get_default_browser();

/// The tag and Git commit ID the program was built from, or a pre-defined
/// value from config.h if there is no Git directory.
std::string program_version();

/// Threadsafe combination of strftime() and localtime()
std::string mt_strf_localtime(const std::string& format, time_t t);
}

} // namespace newsboat

#endif /* NEWSBOAT_UTIL_H_ */

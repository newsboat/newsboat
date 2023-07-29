#ifndef NEWSBOAT_UTIL_H_
#define NEWSBOAT_UTIL_H_

#include <cstdint>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "3rd-party/expected.hpp"

#include "configcontainer.h"
#include "filepath.h"

#include "libnewsboat-ffi/src/utils.rs.h" // IWYU pragma: export

namespace newsboat {

class CurlHandle;

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
std::optional<std::string> extract_token_quoted(std::string& str,
	std::string delimiters = " \r\n\t");

std::string consolidate_whitespace(const std::string& str);

std::string translit(const std::string& tocode,
	const std::string& fromcode);

/// Converts input string from UTF-8 to the locale's encoding (as detected by
/// nl_langinfo(CODESET)).
std::string utf8_to_locale(const std::string& text);
/// Converts input string from the locale's encoding (as detected by
/// nl_langinfo(CODESET)) to UTF-8.
std::string locale_to_utf8(const std::string& text);

std::string convert_text(const std::string& text, const std::string& tocode,
	const std::string& fromcode);

std::string get_command_output(const std::string& cmd);
std::string http_method_str(const HTTPMethod method);
std::string link_type_str(LinkType type);

std::string retrieve_url(const std::string& url,
	ConfigContainer& cfgcont,
	const std::string& authinfo = "",
	const std::string* body = nullptr,
	const HTTPMethod method = HTTPMethod::GET);
std::string retrieve_url(const std::string& url,
	CurlHandle& easyhandle,
	ConfigContainer& cfgcont,
	const std::string& authinfo = "",
	const std::string* body = nullptr,
	const HTTPMethod method = HTTPMethod::GET);
std::string run_program(const char* argv[], const std::string& input);

Filepath resolve_tilde(const Filepath&);
Filepath resolve_relative(const Filepath&, const Filepath&);
std::string replace_all(std::string str,
	const std::string& from,
	const std::string& to);
std::string replace_all(const std::string& str,
	const std::vector<std::pair<std::string, std::string>> from_to_pairs);

std::string to_lowercase(const std::string& input);

std::wstring str2wstr(const std::string& str);
std::string wstr2str(const std::wstring& wstr);

std::wstring clean_nonprintable_characters(std::wstring text);

std::string absolute_url(const std::string& url,
	const std::string& link);

std::string get_useragent(ConfigContainer& cfgcont);

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

void set_common_curl_options(CurlHandle& handle, ConfigContainer& cfg);

curl_proxytype get_proxy_type(const std::string& type);

std::string get_content(xmlNode* node);
std::string get_basename(const std::string& url);

std::string unescape_url(const std::string& url);
void initialize_ssl_implementation(void);

int mkdir_parents(const Filepath& pathname,
	mode_t mode = 0755);

std::string make_title(const std::string& url);

std::optional<std::uint8_t> run_interactively(const std::string& command,
	const std::string& caller);

std::optional<std::uint8_t> run_non_interactively(const std::string& command,
	const std::string& caller);

Filepath getcwd();

enum class ReadTextFileErrorKind {
	CantOpen,
	LineError
};
struct ReadTextFileError {
	ReadTextFileErrorKind kind;
	std::string message;
};
nonstd::expected<std::vector<std::string>, ReadTextFileError> read_text_file(
	const Filepath& filename);

void remove_soft_hyphens(std::string& text);

bool is_valid_podcast_type(const std::string& mimetype);

std::optional<LinkType> podcast_mime_to_link_type(const std::string&
	mimetype);

std::string string_from_utf8_lossy(const std::vector<std::uint8_t>& text);

void parse_rss_author_email(const std::vector<std::uint8_t>& text, std::string& name,
	std::string& email);

Filepath get_default_browser();

std::string md5hash(const std::string& input);

/// The tag and Git commit ID the program was built from, or a pre-defined
/// value from config.h if there is no Git directory.
std::string program_version();

/// Threadsafe combination of strftime() and localtime()
std::string mt_strf_localtime(const std::string& format, time_t t);

/// Preserves single quotes by enclosing each word in
/// single quotes and replaces "'" with "\'"
/// (e.g. "it's" -> "'it'\''s'")
std::string preserve_quotes(const std::string& s);

void wait_for_keypress();
}

} // namespace newsboat

#endif /* NEWSBOAT_UTIL_H_ */

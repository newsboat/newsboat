#include "utils.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <cwchar>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <langinfo.h>
#include <libgen.h>
#include <libxml/uri.h>
#include <locale>
#include <mutex>
#include <pwd.h>
#include <regex>
#include <sstream>
#include <stfl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <unordered_set>

#include "3rd-party/alphanum.hpp"
#include "config.h"
#include "logger.h"
#include "strprintf.h"

#if HAVE_GCRYPT
#include <errno.h>
#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <pthread.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

#if HAVE_OPENSSL && OPENSSL_VERSION_NUMBER < 0x01010000fL
#include <openssl/crypto.h>
#endif

#include "rs_utils.h"

namespace newsboat {

namespace utils {
void append_escapes(std::string& str, char c)
{
	switch (c) {
	case 'n':
		str.append("\n");
		break;
	case 'r':
		str.append("\r");
		break;
	case 't':
		str.append("\t");
		break;
	case '"':
		str.append("\"");
		break;
	// escaped backticks are passed through, still escaped. We un-escape
	// them in ConfigParser::evaluate_backticks
	case '`':
		str.append("\\`");
		break;
	case '\\':
		break;
	default:
		str.append(1, c);
		break;
	}
}
}

std::vector<std::string> utils::tokenize_quoted(const std::string& str,
	std::string delimiters)
{
	/*
	 * This function tokenizes strings, obeying quotes and throwing away
	 * comments that start with a '#'.
	 *
	 * e.g. line: foo bar "foo bar" "a test"
	 * is parsed to 4 elements:
	 * 	[0]: foo
	 * 	[1]: bar
	 * 	[2]: foo bar
	 * 	[3]: a test
	 *
	 * e.g. line: yes great "x\ny" # comment
	 * is parsed to 3 elements:
	 * 	[0]: yes
	 * 	[1]: great
	 * 	[2]: x
	 * 	y
	 *
	 * 	\", \r, \n, \t and \v are replaced with the literals that you
	 * know from C/C++ strings.
	 *
	 */
	bool attach_backslash = true;
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = last_pos;

	while (pos != std::string::npos && last_pos != std::string::npos) {
		if (str[last_pos] == '#') // stop as soon as we found a comment
			break;

		if (str[last_pos] == '"') {
			++last_pos;
			pos = last_pos;
			int backslash_count = 0;
			while (pos < str.length() &&
				(str[pos] != '"' || (backslash_count % 2))) {
				if (str[pos] == '\\') {
					++backslash_count;
				} else {
					backslash_count = 0;
				}
				++pos;
			}
			if (pos >= str.length()) {
				pos = std::string::npos;
				std::string token;
				while (last_pos < str.length()) {
					if (str[last_pos] == '\\') {
						if (str[last_pos - 1] == '\\') {
							if (attach_backslash) {
								token.append(
									"\\");
							}
							attach_backslash =
								!attach_backslash;
						}
					} else {
						if (str[last_pos - 1] == '\\') {
							append_escapes(token,
								str[last_pos]);
						} else {
							token.append(1,
								str[last_pos]);
						}
					}
					++last_pos;
				}
				tokens.push_back(token);
			} else {
				std::string token;
				while (last_pos < pos) {
					if (str[last_pos] == '\\') {
						if (str[last_pos - 1] == '\\') {
							if (attach_backslash) {
								token.append(
									"\\");
							}
							attach_backslash =
								!attach_backslash;
						}
					} else {
						if (str[last_pos - 1] == '\\') {
							append_escapes(token,
								str[last_pos]);
						} else {
							token.append(1,
								str[last_pos]);
						}
					}
					++last_pos;
				}
				tokens.push_back(token);
				++pos;
			}
		} else {
			pos = str.find_first_of(delimiters, last_pos);
			tokens.push_back(str.substr(last_pos, pos - last_pos));
		}
		last_pos = str.find_first_not_of(delimiters, pos);
	}

	return tokens;
}

std::vector<std::string> utils::tokenize(const std::string& str,
	std::string delimiters)
{
	/*
	 * This function tokenizes a string by the delimiters. Plain and simple.
	 */
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, last_pos);
	}
	return tokens;
}

std::vector<std::wstring> utils::wtokenize(const std::wstring& str,
	std::wstring delimiters)
{
	/*
	 * This function tokenizes a string by the delimiters. Plain and simple.
	 */
	std::vector<std::wstring> tokens;
	std::wstring::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::wstring::size_type pos = str.find_first_of(delimiters, last_pos);

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, last_pos);
	}
	return tokens;
}

std::vector<std::string> utils::tokenize_spaced(const std::string& str,
	std::string delimiters)
{
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	if (last_pos != 0) {
		tokens.push_back(str.substr(0, last_pos));
	}

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		if (last_pos > pos)
			tokens.push_back(str.substr(pos, last_pos - pos));
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

std::string utils::consolidate_whitespace(const std::string& str) {

	return RustString(rs_consolidate_whitespace(str.c_str()));
}

std::vector<std::string> utils::tokenize_nl(const std::string& str,
	std::string delimiters)
{
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);
	unsigned int i;

	LOG(Level::DEBUG, "utils::tokenize_nl: last_pos = %u", last_pos);
	if (last_pos != std::string::npos) {
		for (i = 0; i < last_pos; ++i) {
			tokens.push_back(std::string("\n"));
		}
	}

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		LOG(Level::DEBUG,
			"utils::tokenize_nl: substr = %s",
			str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		LOG(Level::DEBUG,
			"utils::tokenize_nl: pos - last_pos = %u",
			last_pos - pos);
		for (i = 0; last_pos != std::string::npos &&
			pos != std::string::npos && i < (last_pos - pos);
			++i) {
			tokens.push_back(std::string("\n"));
		}
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

std::string utils::translit(const std::string& tocode,
	const std::string& fromcode)
{
	std::string tlit = "//TRANSLIT";

	enum class TranslitState { UNKNOWN, SUPPORTED, UNSUPPORTED };

	static TranslitState state = TranslitState::UNKNOWN;

	// TRANSLIT is not needed when converting to unicode encodings
	if (tocode == "utf-8" || tocode == "WCHAR_T")
		return tocode;

	if (state == TranslitState::UNKNOWN) {
		iconv_t cd = ::iconv_open(
			(tocode + "//TRANSLIT").c_str(), fromcode.c_str());

		if (cd == reinterpret_cast<iconv_t>(-1)) {
			if (errno == EINVAL) {
				iconv_t cd = ::iconv_open(
					tocode.c_str(), fromcode.c_str());
				if (cd != reinterpret_cast<iconv_t>(-1)) {
					state = TranslitState::UNSUPPORTED;
				} else {
					fprintf(stderr,
						"iconv_open('%s', '%s') "
						"failed: %s",
						tocode.c_str(),
						fromcode.c_str(),
						strerror(errno));
					abort();
				}
			} else {
				fprintf(stderr,
					"iconv_open('%s//TRANSLIT', '%s') "
					"failed: %s",
					tocode.c_str(),
					fromcode.c_str(),
					strerror(errno));
				abort();
			}
		} else {
			state = TranslitState::SUPPORTED;
		}

		iconv_close(cd);
	}

	return ((state == TranslitState::SUPPORTED) ? (tocode + tlit)
						     : (tocode));
}

std::string utils::convert_text(const std::string& text,
	const std::string& tocode,
	const std::string& fromcode)
{
	std::string result;

	if (strcasecmp(tocode.c_str(), fromcode.c_str()) == 0)
		return text;

	iconv_t cd = ::iconv_open(
		translit(tocode, fromcode).c_str(), fromcode.c_str());

	if (cd == reinterpret_cast<iconv_t>(-1))
		return result;

	size_t inbytesleft;
	size_t outbytesleft;

	/*
	 * of all the Unix-like systems around there, only Linux/glibc seems to
	 * come with a SuSv3-conforming iconv implementation.
	 */
#if !defined(__linux__) && !defined(__GLIBC__) && !defined(__APPLE__) && \
	!defined(__OpenBSD__) && !defined(__FreeBSD__) &&                \
	!defined(__DragonFly__)
	const char* inbufp;
#else
	char* inbufp;
#endif
	char outbuf[16];
	char* outbufp = outbuf;

	outbytesleft = sizeof(outbuf);
	inbufp = const_cast<char*>(
		text.c_str()); // evil, but spares us some trouble
	inbytesleft = strlen(inbufp);

	do {
		char* old_outbufp = outbufp;
		int rc = ::iconv(
			cd, &inbufp, &inbytesleft, &outbufp, &outbytesleft);
		if (-1 == rc) {
			switch (errno) {
			case E2BIG:
				result.append(
					old_outbufp, outbufp - old_outbufp);
				outbufp = outbuf;
				outbytesleft = sizeof(outbuf);
				inbufp += strlen(inbufp) - inbytesleft;
				inbytesleft = strlen(inbufp);
				break;
			case EILSEQ:
			case EINVAL:
				result.append(
					old_outbufp, outbufp - old_outbufp);
				result.append("?");
				inbufp += strlen(inbufp) - inbytesleft + 1;
				inbytesleft = strlen(inbufp);
				break;
			default:
				break;
			}
		} else {
			result.append(old_outbufp, outbufp - old_outbufp);
		}
	} while (inbytesleft > 0);

	iconv_close(cd);

	return result;
}

std::string utils::get_command_output(const std::string& cmd)
{
	return RustString(rs_get_command_output(cmd.c_str()));
}

void utils::extract_filter(const std::string& line,
	std::string& filter,
	std::string& url)
{
	std::string::size_type pos = line.find_first_of(":", 0);
	std::string::size_type pos1 = line.find_first_of(":", pos + 1);
	filter = line.substr(pos + 1, pos1 - pos - 1);
	pos = pos1;
	url = line.substr(pos + 1, line.length() - pos);
	LOG(Level::DEBUG,
		"utils::extract_filter: %s -> filter: %s url: %s",
		line,
		filter,
		url);
}

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

std::string utils::retrieve_url(const std::string& url,
	ConfigContainer* cfgcont,
	const std::string& authinfo,
	const std::string* postdata,
	CURL* cached_handle)
{
	std::string buf;

	CURL* easyhandle;
	if (cached_handle) {
		easyhandle = cached_handle;
	} else {
		easyhandle = curl_easy_init();
	}
	set_common_curl_options(easyhandle, cfgcont);
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);

	if (postdata != nullptr) {
		curl_easy_setopt(easyhandle, CURLOPT_POST, 1);
		curl_easy_setopt(
			easyhandle, CURLOPT_POSTFIELDS, postdata->c_str());
	}

	if (!authinfo.empty()) {
		curl_easy_setopt(easyhandle,
			CURLOPT_HTTPAUTH,
			get_auth_method(
				cfgcont->get_configvalue("http-auth-method")));
		curl_easy_setopt(easyhandle, CURLOPT_USERPWD, authinfo.c_str());
	}

	curl_easy_perform(easyhandle);
	if (!cached_handle) {
		curl_easy_cleanup(easyhandle);
	}

	if (postdata != nullptr) {
		LOG(Level::DEBUG,
			"utils::retrieve_url(%s)[%s]: %s",
			url,
			postdata,
			buf);
	} else {
		LOG(Level::DEBUG, "utils::retrieve_url(%s)[-]: %s", url, buf);
	}

	return buf;
}

void utils::run_command(const std::string& cmd, const std::string& input)
{
	rs_run_command(cmd.c_str(), input.c_str());
}

std::string utils::run_program(char* argv[], const std::string& input)
{
	return RustString(rs_run_program(argv, input.c_str()));
}

std::string utils::resolve_tilde(const std::string& str)
{
	return RustString(rs_resolve_tilde(str.c_str()));
}

std::string utils::resolve_relative(const std::string& reference, const std::string &fname) {
	return RustString(rs_resolve_relative(reference.c_str(), fname.c_str()));
}

std::string utils::replace_all(std::string str,
	const std::string& from,
	const std::string& to)
{
	return RustString( rs_replace_all(str.c_str(), from.c_str(), to.c_str()) );
}

std::wstring utils::str2wstr(const std::string& str)
{
	const char* codeset = nl_langinfo(CODESET);
	struct stfl_ipool* ipool = stfl_ipool_create(codeset);
	std::wstring result = stfl_ipool_towc(ipool, str.c_str());
	stfl_ipool_destroy(ipool);
	return result;
}

std::string utils::wstr2str(const std::wstring& wstr)
{
	std::string codeset = nl_langinfo(CODESET);
	codeset = translit(codeset, "WCHAR_T");
	struct stfl_ipool* ipool = stfl_ipool_create(codeset.c_str());
	std::string result = stfl_ipool_fromwc(ipool, wstr.c_str());
	stfl_ipool_destroy(ipool);
	return result;
}

std::string utils::absolute_url(const std::string& url, const std::string& link)
{
	return RustString(rs_absolute_url(url.c_str(), link.c_str()));
}

std::string utils::get_useragent(ConfigContainer* cfgcont)
{
	std::string ua_pref = cfgcont->get_configvalue("user-agent");
	if (ua_pref.length() == 0) {
		struct utsname buf;
		uname(&buf);
		if (strcmp(buf.sysname, "Darwin") == 0) {
			/* Assume it is a Mac from the last decade or at least
			 * Mac-like */
			const char* PROCESSOR = "";
			if (strcmp(buf.machine, "x86_64") == 0 ||
				strcmp(buf.machine, "i386") == 0) {
				PROCESSOR = "Intel ";
			}
			return strprintf::fmt("%s/%s (Macintosh; %sMac OS X)",
				PROGRAM_NAME,
				PROGRAM_VERSION,
				PROCESSOR);
		}
		return strprintf::fmt("%s/%s (%s %s)",
			PROGRAM_NAME,
			PROGRAM_VERSION,
			buf.sysname,
			buf.machine);
	}
	return ua_pref;
}

unsigned int utils::to_u(const std::string& str,
	const unsigned int default_value)
{
	return rs_to_u(str.c_str(), default_value);
}

ScopeMeasure::ScopeMeasure(const std::string& func, Level ll)
	: funcname(func)
	, lvl(ll)
{
	gettimeofday(&tv1, nullptr);
}

void ScopeMeasure::stopover(const std::string& son)
{
	gettimeofday(&tv2, nullptr);
	unsigned long diff =
		(((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) -
		tv1.tv_usec;
	LOG(lvl,
		"ScopeMeasure: function `%s' (stop over `%s') took %lu.%06lu "
		"s so "
		"far",
		funcname,
		son,
		diff / 1000000,
		diff % 1000000);
}

ScopeMeasure::~ScopeMeasure()
{
	gettimeofday(&tv2, nullptr);
	unsigned long diff =
		(((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) -
		tv1.tv_usec;
	LOG(Level::INFO,
		"ScopeMeasure: function `%s' took %lu.%06lu s",
		funcname,
		diff / 1000000,
		diff % 1000000);
}

bool utils::is_valid_color(const std::string& color)
{
	return rs_is_valid_color(color.c_str());
}

bool utils::is_valid_attribute(const std::string& attrib)
{
	return rs_is_valid_attribute(attrib.c_str());
}

std::vector<std::pair<unsigned int, unsigned int>> utils::partition_indexes(
	unsigned int start,
	unsigned int end,
	unsigned int parts)
{
	std::vector<std::pair<unsigned int, unsigned int>> partitions;
	unsigned int count = end - start + 1;
	unsigned int size = count / parts;

	for (unsigned int i = 0; i < parts - 1; i++) {
		partitions.push_back(std::pair<unsigned int, unsigned int>(
			start, start + size - 1));
		start += size;
	}

	partitions.push_back(std::pair<unsigned int, unsigned int>(start, end));
	return partitions;
}

size_t utils::strwidth(const std::string& str)
{
	return rs_strwidth(str.c_str());
}

size_t utils::strwidth_stfl(const std::string& str)
{
	return rs_strwidth_stfl(str.c_str());
}

size_t utils::wcswidth_stfl(const std::wstring& str, size_t size)
{
	size_t reduce_count = 0;
	size_t len = std::min(str.length(), size);
	if (len > 1) {
		for (size_t idx = 0; idx < len - 1; ++idx) {
			if (str[idx] == L'<' && str[idx + 1] != L'>') {
				reduce_count += 3;
				idx += 3;
			}
		}
	}

	int width = wcswidth(str.c_str(), size);
	if (width < 0) {
		LOG(Level::ERROR, "oh, oh, wcswidth just failed");
		return str.length() - reduce_count;
	}

	return width - reduce_count;
}

std::string utils::substr_with_width(const std::string& str,
	const size_t max_width)
{
	// Returns a longest substring fits to the given width.
	// Returns an empty string if `str` is an empty string or `max_width` is
	// zero,
	//
	// Each chararacter width is calculated with wcwidth(3). If wcwidth()
	// returns < 1, the character width is treated as 0. A STFL tag (e.g.
	// `<b>`, `<foobar>`, `</>`) width is treated as 0, but escaped
	// less-than (`<>`) width is treated as 1.

	if (str.empty() || max_width == 0) {
		return std::string("");
	}

	const std::wstring wstr = utils::str2wstr(str);
	size_t total_width = 0;
	bool in_bracket = false;
	std::wstring result;
	std::wstring tagbuf;

	for (const auto& wc : wstr) {
		if (in_bracket) {
			tagbuf += wc;
			if (wc == L'>') { // tagbuf is escaped less-than or tag
				in_bracket = false;
				if (tagbuf == L"<>") {
					if (total_width + 1 > max_width) {
						break;
					}
					result += L"<>"; // escaped less-than
					tagbuf.clear();
					total_width++;
				} else {
					result += tagbuf;
					tagbuf.clear();
				}
			}
		} else {
			if (wc == L'<') {
				in_bracket = true;
				tagbuf += wc;
			} else {
				int w = wcwidth(wc);
				if (w < 1) {
					w = 0;
				}
				if (total_width + w > max_width) {
					break;
				}
				total_width += w;
				result += wc;
			}
		}
	}
	return utils::wstr2str(result);
}

std::string utils::join(const std::vector<std::string>& strings,
	const std::string& separator)
{
	std::string result;

	for (const auto& str : strings) {
		result.append(str);
		result.append(separator);
	}

	if (result.length() > 0)
		result.erase(
			result.length() - separator.length(), result.length());

	return result;
}

bool utils::is_special_url(const std::string& url)
{
	return rs_is_special_url(url.c_str());
}

bool utils::is_http_url(const std::string& url)
{
	return rs_is_http_url(url.c_str());
}

bool utils::is_query_url(const std::string& url)
{
	return rs_is_query_url(url.c_str());
}

bool utils::is_filter_url(const std::string& url)
{
	return rs_is_filter_url(url.c_str());
}

bool utils::is_exec_url(const std::string& url)
{
	return rs_is_exec_url(url.c_str());
}

std::string utils::censor_url(const std::string& url)
{
	return RustString(rs_censor_url(url.c_str()));
}

std::string utils::quote_for_stfl(std::string str)
{
	unsigned int len = str.length();
	for (unsigned int i = 0; i < len; ++i) {
		if (str[i] == '<') {
			str.insert(i + 1, ">");
			++len;
		}
	}
	return str;
}

void utils::trim(std::string& str)
{
	str = RustString(rs_trim(str.c_str()));
}

void utils::trim_end(std::string& str)
{
	str = RustString(rs_trim_end(str.c_str()));
}

std::string utils::quote(const std::string& str)
{
	return RustString(rs_quote(str.c_str()));
}

unsigned int utils::get_random_value(unsigned int max)
{
	return rs_get_random_value(max);
}

std::string utils::quote_if_necessary(const std::string& str)
{
	return RustString(rs_quote_if_necessary(str.c_str()));
}

void utils::set_common_curl_options(CURL* handle, ConfigContainer* cfg)
{
	if (cfg) {
		if (cfg->get_configvalue_as_bool("use-proxy")) {
			const std::string proxy = cfg->get_configvalue("proxy");
			if (proxy != "")
				curl_easy_setopt(
					handle, CURLOPT_PROXY, proxy.c_str());

			const std::string proxyauth =
				cfg->get_configvalue("proxy-auth");
			const std::string proxyauthmethod =
				cfg->get_configvalue("proxy-auth-method");
			if (proxyauth != "") {
				curl_easy_setopt(handle,
					CURLOPT_PROXYAUTH,
					get_auth_method(proxyauthmethod));
				curl_easy_setopt(handle,
					CURLOPT_PROXYUSERPWD,
					proxyauth.c_str());
			}

			const std::string proxytype =
				cfg->get_configvalue("proxy-type");
			if (proxytype != "") {
				LOG(Level::DEBUG,
					"utils::set_common_curl_options: "
					"proxytype "
					"= %s",
					proxytype);
				curl_easy_setopt(handle,
					CURLOPT_PROXYTYPE,
					get_proxy_type(proxytype));
			}
		}

		const std::string useragent = utils::get_useragent(cfg);
		curl_easy_setopt(handle, CURLOPT_USERAGENT, useragent.c_str());

		const unsigned int dl_timeout =
			cfg->get_configvalue_as_int("download-timeout");
		curl_easy_setopt(handle, CURLOPT_TIMEOUT, dl_timeout);

		const std::string cookie_cache =
			cfg->get_configvalue("cookie-cache");
		if (cookie_cache != "") {
			curl_easy_setopt(handle,
				CURLOPT_COOKIEFILE,
				cookie_cache.c_str());
			curl_easy_setopt(handle,
				CURLOPT_COOKIEJAR,
				cookie_cache.c_str());
		}

		curl_easy_setopt(handle,
			CURLOPT_SSL_VERIFYHOST,
			cfg->get_configvalue_as_bool("ssl-verifyhost") ? 2 : 0);
		curl_easy_setopt(handle,
			CURLOPT_SSL_VERIFYPEER,
			cfg->get_configvalue_as_bool("ssl-verifypeer"));
	}

	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(handle, CURLOPT_ENCODING, "gzip, deflate");

	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

	const char* curl_ca_bundle = ::getenv("CURL_CA_BUNDLE");
	if (curl_ca_bundle != nullptr) {
		curl_easy_setopt(handle, CURLOPT_CAINFO, curl_ca_bundle);
	}
}

std::string utils::get_content(xmlNode* node)
{
	std::string retval;
	if (node) {
		xmlChar* content = xmlNodeGetContent(node);
		if (content) {
			retval = (const char*)content;
			xmlFree(content);
		}
	}
	return retval;
}

std::string utils::get_basename(const std::string& url)
{
	std::string retval;
	xmlURIPtr uri = xmlParseURI(url.c_str());
	if (uri && uri->path) {
		std::string path(uri->path);
		// check for path ending with an empty filename
		if (path[path.length() - 1] != '/') {
			char* base = basename(uri->path);
			if (base) {
				// check for empty path
				if (base[0] != '/') {
					retval = std::string(base);
				}
			}
		}
		xmlFreeURI(uri);
	}
	return retval;
}

unsigned long utils::get_auth_method(const std::string& type)
{
	if (type == "any")
		return CURLAUTH_ANY;
	if (type == "basic")
		return CURLAUTH_BASIC;
	if (type == "digest")
		return CURLAUTH_DIGEST;
#ifdef CURLAUTH_DIGEST_IE
	if (type == "digest_ie")
		return CURLAUTH_DIGEST_IE;
#else
#warning \
	"proxy-auth-method digest_ie not added due to libcurl older than 7.19.3"
#endif
	if (type == "gssnegotiate")
		return CURLAUTH_GSSNEGOTIATE;
	if (type == "ntlm")
		return CURLAUTH_NTLM;
	if (type == "anysafe")
		return CURLAUTH_ANYSAFE;
	if (type != "") {
		LOG(Level::USERERROR,
			"you configured an invalid proxy authentication "
			"method: %s",
			type);
	}
	return CURLAUTH_ANY;
}

curl_proxytype utils::get_proxy_type(const std::string& type)
{
	if (type == "http")
		return CURLPROXY_HTTP;
	if (type == "socks4")
		return CURLPROXY_SOCKS4;
	if (type == "socks5")
		return CURLPROXY_SOCKS5;
	if (type == "socks5h")
		return CURLPROXY_SOCKS5_HOSTNAME;
#ifdef CURLPROXY_SOCKS4A
	if (type == "socks4a")
		return CURLPROXY_SOCKS4A;
#endif

	if (type != "") {
		LOG(Level::USERERROR,
			"you configured an invalid proxy type: %s",
			type);
	}
	return CURLPROXY_HTTP;
}

std::string utils::unescape_url(const std::string& url)
{
	char* ptr = rs_unescape_url(url.c_str());
	if (ptr == nullptr) {
		LOG(Level::DEBUG, "Rust failed to unescape url: %s", url );
		throw std::runtime_error("unescaping url failed");
	} else {
		return RustString(ptr);
	}
}

std::wstring utils::clean_nonprintable_characters(std::wstring text)
{
	for (size_t idx = 0; idx < text.size(); ++idx) {
		if (!iswprint(text[idx]))
			text[idx] = L'\uFFFD';
	}
	return text;
}

unsigned int utils::gentabs(const std::string& str)
{
	int tabcount = 4 - (utils::strwidth(str) / 8);
	if (tabcount <= 0) {
		tabcount = 1;
	}
	return tabcount;
}

/* Like mkdir(), but creates ancestors (parent directories) if they don't
 * exist. */
int utils::mkdir_parents(const std::string& p, mode_t mode)
{
	int result = -1;

	/* Have to copy the path because we're going to modify it */
	const size_t length = p.length() + 1;
	char* pathname = (char*)malloc(length);
	if (pathname == nullptr) {
		// errno is already set by malloc(), so simply return with an error
		return -1;
	}
	strncpy(pathname, p.c_str(), length);
	/* This pointer will run through the whole string looking for '/'.
	 * We move it by one if path starts with slash because if we don't, the
	 * first call to access() will fail (because of empty path) */
	char* curr = pathname + (*pathname == '/' ? 1 : 0);

	while (*curr) {
		if (*curr == '/') {
			*curr = '\0';
			result = access(pathname, F_OK);
			if (result == -1) {
				result = mkdir(pathname, mode);
				if (result != 0 && errno != EEXIST)
					break;
			}
			*curr = '/';
		}
		curr++;
	}

	if (result == 0) {
		result = mkdir(p.c_str(), mode);
	}

	free(pathname);
	return result;
}

std::string utils::make_title(const std::string& const_url)
{
	return RustString(rs_make_title(const_url.c_str()));
}

int utils::run_interactively(const std::string& command,
	const std::string& caller)
{
	LOG(Level::DEBUG, "%s: running `%s'", caller, command);

	int status = ::system(command.c_str());

	if (status == -1) {
		LOG(Level::DEBUG,
			"%s: couldn't create a child process",
			caller);
	} else if (status == 127) {
		LOG(Level::DEBUG, "%s: couldn't run shell", caller);
	}

	return status;
}

std::string utils::getcwd()
{
	// Linux seem to have its MAX_PATH set somewhere around this value, so
	// should be a nice default
	std::vector<char> result(4096, '\0');

	while (true) {
		char* ret = ::getcwd(result.data(), result.size());
		if (ret == nullptr && errno == ERANGE) {
			result.resize(result.size() * 2);
		} else {
			break;
		}
	}

	return std::string(result.data());
}

int utils::strnaturalcmp(const std::string& a, const std::string& b)
{
	return doj::alphanum_comp(a, b);
}

void utils::remove_soft_hyphens(std::string& text)
{
	/* Remove all soft-hyphens as they can behave unpredictably (see
	 * https://github.com/akrennmair/newsbeuter/issues/259#issuecomment-259609490)
	 * and inadvertently render as hyphens */

	std::string::size_type pos = text.find("\u00AD");
	while (pos != std::string::npos) {
		text.erase(pos, 2);
		pos = text.find("\u00AD", pos);
	}
}

bool utils::is_valid_podcast_type(const std::string& mimetype)
{
	return rs_is_valid_podcast_type(mimetype.c_str());
}

	/*
	 * See
	 * http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
	 * for a reason why we do this.
	 *
	 * These callbacks are deprecated as of OpenSSL 1.1.0; see the
	 * changelog: https://www.openssl.org/news/changelog.html#x6
	 */

#if HAVE_OPENSSL && OPENSSL_VERSION_NUMBER < 0x01010000fL
static std::mutex* openssl_mutexes = nullptr;
static int openssl_mutexes_size = 0;

static void
openssl_mth_locking_function(int mode, int n, const char* file, int line)
{
	if (n < 0 || n >= openssl_mutexes_size) {
		LOG(Level::ERROR,
			"openssl_mth_locking_function: index is out of bounds "
			"(called by %s:%d)",
			file,
			line);
		return;
	}
	if (mode & CRYPTO_LOCK) {
		LOG(Level::DEBUG, "OpenSSL lock %d: %s:%d", n, file, line);
		openssl_mutexes[n].lock();
	} else {
		LOG(Level::DEBUG, "OpenSSL unlock %d: %s:%d", n, file, line);
		openssl_mutexes[n].unlock();
	}
}

static unsigned long openssl_mth_id_function(void)
{
	return (unsigned long)pthread_self();
}
#endif

void utils::initialize_ssl_implementation(void)
{
#if HAVE_OPENSSL && OPENSSL_VERSION_NUMBER < 0x01010000fL
	openssl_mutexes_size = CRYPTO_num_locks();
	openssl_mutexes = new std::mutex[openssl_mutexes_size];
	CRYPTO_set_id_callback(openssl_mth_id_function);
	CRYPTO_set_locking_callback(openssl_mth_locking_function);
#endif

#if HAVE_GCRYPT
	gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
	gnutls_global_init();
#endif
}

std::string utils::get_default_browser()
{
	return RustString(rs_get_default_browser());
}

} // namespace newsboat

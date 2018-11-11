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
	FILE* f = popen(cmd.c_str(), "r");
	std::string buf;
	if (f) {
		char cbuf[1024];
		size_t s;
		while ((s = fread(cbuf, 1, sizeof(cbuf), f)) > 0) {
			buf.append(cbuf, s);
		}
		pclose(f);
	}
	return buf;
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
	int rc = fork();
	switch (rc) {
	case -1:
		break;
	case 0: { // child:
		int fd = ::open("/dev/null", O_RDWR);
		if (fd == -1) {
			LOG(Level::DEBUG,
				"utils::run_command: error opening /dev/null: "
				"(%i) "
				"%s",
				errno,
				strerror(errno));
			exit(1);
		}
		close(0);
		close(1);
		close(2);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		LOG(Level::DEBUG, "utils::run_command: %s '%s'", cmd, input);
		execlp(cmd.c_str(), cmd.c_str(), input.c_str(), nullptr);
		LOG(Level::DEBUG,
			"utils::run_command: execlp of %s failed: %s",
			cmd,
			strerror(errno));
		exit(1);
	}
	default:
		break;
	}
}

std::string utils::run_program(char* argv[], const std::string& input)
{
	std::string buf;
	int ipipe[2];
	int opipe[2];
	if (pipe(ipipe) != 0) {
		return "";
	}
	if (pipe(opipe) != 0) {
		return "";
	}

	int rc = fork();
	switch (rc) {
	case -1:
		break;
	case 0: { // child:
		close(ipipe[1]);
		close(opipe[0]);
		dup2(ipipe[0], 0);
		dup2(opipe[1], 1);
		close(2);

		int errfd = ::open("/dev/null", O_WRONLY);
		if (errfd != -1)
			dup2(errfd, 2);

		execvp(argv[0], argv);
		exit(1);
	}
	default: {
		close(ipipe[0]);
		close(opipe[1]);
		ssize_t written = 0;
		written = write(ipipe[1], input.c_str(), input.length());
		if (written != -1) {
			close(ipipe[1]);
			char cbuf[1024];
			int rc2;
			while ((rc2 = read(opipe[0], cbuf, sizeof(cbuf))) > 0) {
				buf.append(cbuf, rc2);
			}
		} else {
			close(ipipe[1]);
		}
		close(opipe[0]);
	} break;
	}
	return buf;
}

std::string utils::resolve_tilde(const std::string& str)
{
	const char* homedir;
	std::string filepath;

	if (!(homedir = ::getenv("HOME"))) {
		struct passwd* spw = ::getpwuid(::getuid());
		if (spw) {
			homedir = spw->pw_dir;
		} else {
			homedir = "";
		}
	}

	if (strcmp(homedir, "") != 0) {
		if (str == "~") {
			filepath.append(homedir);
		} else if (str.substr(0, 2) == "~/") {
			filepath.append(homedir);
			filepath.append(1, '/');
			filepath.append(str.substr(2, str.length() - 2));
		} else {
			filepath.append(str);
		}
	} else {
		filepath.append(str);
	}

	return filepath;
}

std::string utils::replace_all(std::string str,
	const std::string& from,
	const std::string& to)
{
	return RustString( rs_replace_all(str.c_str(), from.c_str(), to.c_str()) );
}

std::wstring utils::utf8str2wstr(const std::string& utf8str)
{
	stfl_ipool* pool = stfl_ipool_create("utf-8");
	std::wstring wstr = stfl_ipool_towc(pool, utf8str.c_str());
	stfl_ipool_destroy(pool);
	return wstr;
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
	xmlChar* newurl = xmlBuildURI(
		(const xmlChar*)link.c_str(), (const xmlChar*)url.c_str());
	std::string retval;
	if (newurl) {
		retval = (const char*)newurl;
		xmlFree(newurl);
	} else {
		retval = link;
	}
	return retval;
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
	std::wstring wstr = str2wstr(str);
	int width = wcswidth(wstr.c_str(), wstr.length());
	if (width < 1)                // a non-printable character found?
		return wstr.length(); // return a sane width (which might be
				      // larger than necessary)
	return width;                 // exact width
}

size_t utils::strwidth_stfl(const std::string& str)
{
	size_t reduce_count = 0;
	size_t len = str.length();
	if (len > 1) {
		for (size_t idx = 0; idx < len - 1; ++idx) {
			if (str[idx] == '<' && str[idx + 1] != '>') {
				reduce_count += 3;
				idx += 3;
			}
		}
	}

	return strwidth(str) - reduce_count;
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
	return is_query_url(url) || is_filter_url(url) || is_exec_url(url);
}

bool utils::is_http_url(const std::string& url)
{
	return url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://";
}

bool utils::is_query_url(const std::string& url)
{
	return url.substr(0, 6) == "query:";
}

bool utils::is_filter_url(const std::string& url)
{
	return url.substr(0, 7) == "filter:";
}

bool utils::is_exec_url(const std::string& url)
{
	return url.substr(0, 5) == "exec:";
}

std::string utils::censor_url(const std::string& url)
{
	std::string rv(url);
	if (!url.empty() && !utils::is_special_url(url)) {
		const char* myuri = url.c_str();
		xmlURIPtr uri = xmlParseURI(myuri);
		if (uri) {
			if (uri->user) {
				xmlFree(uri->user);
				uri->user =
					(char*)xmlStrdup((const xmlChar*)"*:*");
			}
			xmlChar* uristr = xmlSaveUri(uri);

			rv = (const char*)uristr;
			xmlFree(uristr);
			xmlFreeURI(uri);
		}
	}
	return rv;
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
	std::string rv = replace_all(str, "\"", "\\\"");
	rv.insert(0, "\"");
	rv.append("\"");
	return rv;
}

unsigned int utils::get_random_value(unsigned int max)
{
	return rs_get_random_value(max);
}

std::string utils::quote_if_necessary(const std::string& str)
{
	std::string result;
	if (str.find_first_of(" ", 0) == std::string::npos) {
		result = str;
	} else {
		result = utils::replace_all(str, "\"", "\\\"");
		result.insert(0, "\"");
		result.append("\"");
	}
	return result;
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

std::string utils::escape_url(const std::string& url)
{
	CURL* easyhandle = curl_easy_init();
	char* output = curl_easy_escape(easyhandle, url.c_str(), 0);
	if (!output) {
		LOG(Level::DEBUG, "Libcurl failed to escape url: %s", url);
		throw std::runtime_error("escaping url failed");
	}
	std::string s = output;
	curl_free(output);
	curl_easy_cleanup(easyhandle);
	return s;
}

std::string utils::unescape_url(const std::string& url)
{
	CURL* easyhandle = curl_easy_init();
	char* output = curl_easy_unescape(easyhandle, url.c_str(), 0, NULL);
	if (!output) {
		LOG(Level::DEBUG, "Libcurl failed to escape url: %s", url);
		throw std::runtime_error("escaping url failed");
	}
	std::string s = output;
	curl_free(output);
	curl_easy_cleanup(easyhandle);
	return s;
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
	/* Sometimes it is possible to construct the title from the URL
	 * This attempts to do just that. eg:
	 * http://domain.com/story/yy/mm/dd/title-with-dashes?a=b
	 */
	std::string url = (std::string&)const_url;
	// Strip out trailing slashes
	while (url.length() > 0 && url.back() == '/') {
		url.erase(url.length() - 1);
	}
	// get to the final part of the URI's path
	std::string::size_type pos_of_slash = url.find_last_of('/');
	// extract just the juicy part 'title-with-dashes?a=b'
	std::string path = url.substr(pos_of_slash + 1);
	// find where query part of URI starts
	std::string::size_type pos_of_qmrk = path.find_first_of('?');
	// throw away the query part 'title-with-dashes'
	std::string title = path.substr(0, pos_of_qmrk);
	// Throw away common webpage suffixes: .html, .php, .aspx, .htm
	std::regex rx("\\.html$|\\.htm$|\\.php$|\\.aspx$");
	title = std::regex_replace(title, rx, "");
	// if there is nothing left, just give up
	if (title.empty())
		return title;
	// 'title with dashes'
	std::replace(title.begin(), title.end(), '-', ' ');
	std::replace(title.begin(), title.end(), '_', ' ');
	//'Title with dashes'
	if (title.at(0) >= 'a' && title.at(0) <= 'z') {
		title[0] -= 'a' - 'A';
	}
	// Un-escape any percent-encoding, e.g. "It%27s%202017%21" -> "It's
	// 2017!"
	auto const result = xmlURIUnescapeString(title.c_str(), 0, nullptr);
	if (result) {
		title = result;
		xmlFree(result);
	}
	return title;
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
	const std::regex acceptable_rx{"(audio|video)/.*",
		std::regex_constants::ECMAScript |
			std::regex_constants::optimize};

	const std::unordered_set<std::string> acceptable = {"application/ogg"};

	const bool found = acceptable.find(mimetype) != acceptable.end();
	const bool matches = std::regex_match(mimetype, acceptable_rx);

	return found || matches;
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

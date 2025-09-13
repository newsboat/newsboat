#include "utils.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <cwchar>
#include <fcntl.h>
#include <iconv.h>
#include <langinfo.h>
#include <libxml/uri.h>
#include <mutex>
#include <ncurses.h>
#include <pwd.h>
#include <sstream>
#include <stfl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#if HAVE_GCRYPT
#include <gnutls/gnutls.h>
#if GNUTLS_VERSION_NUMBER < 0x020b00
#include <errno.h>
#include <gcrypt.h>
#include <pthread.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif
#endif

#if HAVE_OPENSSL
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <openssl/crypto.h>
#endif
#endif

#include "config.h"
#include "curldatareceiver.h"
#include "curlhandle.h"
#include "links.h"
#include "logger.h"
#include "strprintf.h"

using HTTPMethod = newsboat::utils::HTTPMethod;

namespace newsboat {

std::string utils::strip_comments(const std::string& line)
{
	return std::string(utils::bridged::strip_comments(line));
}

std::vector<std::string> utils::tokenize_quoted(const std::string& str,
	std::string delimiters)
{
	const auto tokens = utils::bridged::tokenize_quoted(str, delimiters);

	std::vector<std::string> result;
	for (const auto& token : tokens) {
		result.push_back(std::string(token));
	}
	return result;
}

std::optional<std::string> utils::extract_token_quoted(std::string& str,
	std::string delimiters)
{
	rust::String remaining = str;
	rust::String token;
	if (utils::bridged::extract_token_quoted(remaining, delimiters, token)) {
		str = std::string(remaining);
		return std::string(token);
	}
	str = std::string(remaining);
	return {};
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
		if (last_pos > pos) {
			tokens.push_back(str.substr(pos, last_pos - pos));
		}
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

std::string utils::consolidate_whitespace(const std::string& str)
{
	return std::string(utils::bridged::consolidate_whitespace(str));
}

std::vector<std::string> utils::tokenize_nl(const std::string& str,
	std::string delimiters)
{
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);
	unsigned int i;

	LOG(Level::DEBUG,
		"utils::tokenize_nl: last_pos = %" PRIu64,
		static_cast<uint64_t>(last_pos));
	if (last_pos != std::string::npos) {
		for (i = 0; i < last_pos; ++i) {
			tokens.push_back(std::string("\n"));
		}
	} else {
		for (i = 0; i < str.length(); ++i) {
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
			"utils::tokenize_nl: pos - last_pos = %" PRIu64,
			static_cast<uint64_t>(last_pos - pos));
		for (i = 0; last_pos != std::string::npos &&
			pos != std::string::npos && i < (last_pos - pos);
			++i) {
			tokens.push_back(std::string("\n"));
		}
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

std::string utils::translit(const std::string& tocode, const std::string& fromcode)
{
	return std::string(utils::bridged::translit(tocode, fromcode));
}

std::string utils::utf8_to_locale(const std::string& text)
{
	const auto result = utils::bridged::utf8_to_locale(text);
	return std::string(reinterpret_cast<const char*>(result.data()), result.size());
}

std::string utils::locale_to_utf8(const std::string& text)
{
	const auto text_slice =
		rust::Slice<const unsigned char>(
			reinterpret_cast<const unsigned char*>(text.c_str()),
			text.length());
	return std::string(utils::bridged::locale_to_utf8(text_slice));
}

std::string utils::convert_text(const std::string& text, const std::string& tocode,
	const std::string& fromcode)
{
	const auto text_slice =
		rust::Slice<const unsigned char>(
			reinterpret_cast<const unsigned char*>(text.c_str()),
			text.length());

	const auto result = utils::bridged::convert_text(text_slice, tocode, fromcode);

	return std::string(reinterpret_cast<const char*>(result.data()), result.size());
}

std::string utils::get_command_output(const std::string& cmd)
{
	return std::string(utils::bridged::get_command_output(cmd));
}

std::string utils::http_method_str(const HTTPMethod method)
{
	std::string str = "";

	switch (method) {
	case HTTPMethod::GET:
		str = "GET";
		break;
	case HTTPMethod::POST:
		str = "POST";
		break;
	case HTTPMethod::PUT:
		str = "PUT";
		break;
	case HTTPMethod::DELETE:
		str = "DELETE";
		break;
	}

	return str;
}

std::string utils::link_type_str(LinkType type)
{
	// Different from HtmlRenderer::type2str because the string returned
	// isn't localized
	switch (type) {
	case LinkType::HREF:
		return "link";
	case LinkType::IMG:
		return "image";
	case LinkType::EMBED:
		return "embed_flash";
	case LinkType::IFRAME:
		return "iframe";
	case LinkType::VIDEO:
		return "video";
	case LinkType::AUDIO:
		return "audio";
	default:
		return "unknown (bug)";
	}
}

std::string utils::retrieve_url(const std::string& url,
	ConfigContainer& cfgcont,
	const std::string& authinfo,
	const std::string* body,
	const HTTPMethod method /* = GET */)
{
	CurlHandle handle;
	return retrieve_url(url, handle, cfgcont, authinfo, body, method);
}

std::string utils::retrieve_url(const std::string& url,
	CurlHandle& easyhandle,
	ConfigContainer& cfgcont,
	const std::string& authinfo,
	const std::string* body,
	const HTTPMethod method /* = GET */)
{
	set_common_curl_options(easyhandle, cfgcont);
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_URL, url.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(easyhandle);

	switch (method) {
	case HTTPMethod::GET:
		break;
	case HTTPMethod::POST:
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_POST, 1);
		break;
	case HTTPMethod::PUT:
	case HTTPMethod::DELETE:
		const std::string method_str = http_method_str(method);
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_CUSTOMREQUEST, method_str.c_str());
		break;
	}

	if (body != nullptr) {
		curl_easy_setopt(
			easyhandle.ptr(), CURLOPT_POSTFIELDS, body->c_str());
	}

	if (!authinfo.empty()) {
		const auto auth_method = cfgcont.get_configvalue("http-auth-method");
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_HTTPAUTH, get_auth_method(auth_method));
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_USERPWD, authinfo.c_str());
	}

	// Error handling as per https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html
	char errbuf[CURL_ERROR_SIZE];
	// Please note that we clobber CURLOPT_ERRORBUFFER here in case of cached handles
	// Currently, this is the only place we do this, so this should be fine.
	// There does not seem to be a way to query curlopts, so we cannot save the old errorbuf value here...
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_ERRORBUFFER, errbuf);
	errbuf[0] = '\0';

	CURLcode res = curl_easy_perform(easyhandle.ptr());

	std::stringstream logprefix;
	logprefix << "utils::retrieve_url(" << http_method_str(method) << " " << url << ")"
		<< "[" << (body != nullptr ? body->c_str() : "-") << "]";

	std::string buf = curlDataReceiver->get_data();
	if (res != CURLE_OK) {
		std::string errmsg(errbuf);
		if (errmsg.empty()) {
			errmsg = curl_easy_strerror(res);
		}
		if (!errmsg.empty() && errmsg.back() == '\n') {
			// Prettify: drop superflous newlines introduced by libcurl
			errmsg.pop_back();
		}

		LOG(Level::ERROR, "%s: LibCURL error (%d): %s", logprefix.str(), res, errmsg);
		buf = "";
	} else {
		LOG(Level::DEBUG, "%s: %s", logprefix.str(), buf);
	}

	// Reset ERRORBUFFER: has to be valid for the whole lifetime of the handle
	// NULL is the default value of this property according to man (3) CURLOPT_ERRORBUFFER
	// See the clobbering note above.
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_ERRORBUFFER, NULL);

	return buf;
}

std::string utils::run_program(const char* argv[], const std::string& input)
{
	std::vector<rust::Str> slices;
	for (; *argv; ++argv) {
		slices.emplace_back(*argv);
	}

	const auto rs_argv = rust::Slice<const rust::Str>(slices.data(), slices.size());

	return std::string(utils::bridged::run_program(rs_argv, input));
}

Filepath utils::resolve_tilde(const Filepath& path)
{
	auto output = filepath::bridged::create_empty();
	utils::bridged::resolve_tilde(path, *output);
	return output;
}

Filepath utils::resolve_relative(const Filepath& reference,
	const Filepath& fname)
{
	auto output = filepath::bridged::create_empty();
	utils::bridged::resolve_relative(reference, fname, *output);
	return output;
}

std::string utils::replace_all(std::string str,
	const std::string& from,
	const std::string& to)
{
	return std::string(utils::bridged::replace_all(str, from, to));
}

std::string utils::replace_all(const std::string& str,
	const std::vector<std::pair<std::string, std::string>>& from_to_pairs)
{
	std::size_t cur_index = 0;
	std::string output;
	while (cur_index != str.size()) {
		std::size_t first_match = std::string::npos;
		std::pair<std::string, std::string> first_match_pair;
		for (const auto& p : from_to_pairs) {
			auto match = str.find(p.first, cur_index);
			if (match != std::string::npos && match < first_match) {
				first_match = match;
				first_match_pair = p;
			}
		}
		if (first_match == std::string::npos) {
			output += str.substr(cur_index);
			break;
		} else {
			output += str.substr(cur_index, first_match - cur_index);
			cur_index = first_match + first_match_pair.first.size();
			output += first_match_pair.second;
		}
	}
	return output;
}

std::string utils::to_lowercase(const std::string& input)
{
	std::string output;
	std::transform(input.begin(), input.end(), std::back_inserter(output),
	[](unsigned char c) {
		return std::tolower(c);
	});

	return output;
}

std::string utils::preserve_quotes(const std::string& str)
{
	std::string escaped_string = "";
	std::vector<std::string> string_tokenized = tokenize_spaced(str, "'");
	for (std::string string_chunk : string_tokenized) {
		if (string_chunk[0] == '\'') {
			for (size_t i = 0; i < string_chunk.length(); i++) {
				escaped_string += "\\\'";
			}
		} else {
			escaped_string += "'" + string_chunk + "'";
		}
	}
	return escaped_string;
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
	return std::string(utils::bridged::absolute_url(url, link));
}

std::string utils::get_useragent(ConfigContainer& cfgcont)
{
	std::string ua_pref = cfgcont.get_configvalue("user-agent");
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
					utils::program_version(),
					PROCESSOR);
		}
		return strprintf::fmt("%s/%s (%s %s)",
				PROGRAM_NAME,
				utils::program_version(),
				buf.sysname,
				buf.machine);
	}
	return ua_pref;
}

unsigned int utils::to_u(const std::string& str,
	const unsigned int default_value)
{
	return bridged::to_u(str, default_value);
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

std::string utils::substr_with_width(const std::string& str,
	const size_t max_width)
{
	return std::string(utils::bridged::substr_with_width(str, max_width));
}

std::string utils::substr_with_width_stfl(const std::string& str,
	const size_t max_width)
{
	return std::string(utils::bridged::substr_with_width_stfl(str, max_width));
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

std::string utils::censor_url(const std::string& url)
{
	return std::string(utils::bridged::censor_url(url));
}

std::string utils::quote_for_stfl(std::string str)
{
	return std::string(utils::bridged::quote_for_stfl(str));
}

void utils::trim(std::string& str)
{
	str = std::string(utils::bridged::trim(str));
}

void utils::trim_end(std::string& str)
{
	str = std::string(utils::bridged::trim_end(str));
}

std::string utils::quote(const std::string& str)
{
	return std::string(utils::bridged::quote(str));
}

std::string utils::quote_if_necessary(const std::string& str)
{
	return std::string(utils::bridged::quote_if_necessary(str));
}

void utils::set_common_curl_options(CurlHandle& handle, ConfigContainer& cfg)
{
	if (cfg.get_configvalue_as_bool("use-proxy")) {
		const std::string proxy = cfg.get_configvalue("proxy");
		if (proxy != "")
			curl_easy_setopt(
				handle.ptr(), CURLOPT_PROXY, proxy.c_str());

		const std::string proxyauth =
			cfg.get_configvalue("proxy-auth");
		const std::string proxyauthmethod =
			cfg.get_configvalue("proxy-auth-method");
		if (proxyauth != "") {
			curl_easy_setopt(handle.ptr(),
				CURLOPT_PROXYAUTH,
				get_auth_method(proxyauthmethod));
			curl_easy_setopt(handle.ptr(),
				CURLOPT_PROXYUSERPWD,
				proxyauth.c_str());
		}

		const std::string proxytype =
			cfg.get_configvalue("proxy-type");
		if (proxytype != "") {
			LOG(Level::DEBUG,
				"utils::set_common_curl_options: "
				"proxytype "
				"= %s",
				proxytype);
			curl_easy_setopt(handle.ptr(),
				CURLOPT_PROXYTYPE,
				get_proxy_type(proxytype));
		}
	}

	const std::string useragent = utils::get_useragent(cfg);
	curl_easy_setopt(handle.ptr(), CURLOPT_USERAGENT, useragent.c_str());

	const unsigned int dl_timeout =
		cfg.get_configvalue_as_int("download-timeout");
	curl_easy_setopt(handle.ptr(), CURLOPT_TIMEOUT, dl_timeout);

	const Filepath cookie_cache = cfg.get_configvalue_as_filepath("cookie-cache");
	if (cookie_cache != Filepath{}) {
		curl_easy_setopt(handle.ptr(),
			CURLOPT_COOKIEFILE,
			cookie_cache.to_locale_string().c_str());
		curl_easy_setopt(handle.ptr(),
			CURLOPT_COOKIEJAR,
			cookie_cache.to_locale_string().c_str());
	}

	curl_easy_setopt(handle.ptr(),
		CURLOPT_SSL_VERIFYHOST,
		cfg.get_configvalue_as_bool("ssl-verifyhost") ? 2 : 0);
	curl_easy_setopt(handle.ptr(),
		CURLOPT_SSL_VERIFYPEER,
		cfg.get_configvalue_as_bool("ssl-verifypeer"));

	curl_easy_setopt(handle.ptr(), CURLOPT_NOSIGNAL, 1);
	// Accept all of curl's built-in encodings
	curl_easy_setopt(handle.ptr(), CURLOPT_ACCEPT_ENCODING, "");

	curl_easy_setopt(handle.ptr(), CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle.ptr(), CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(handle.ptr(), CURLOPT_FAILONERROR, 1);

	const char* curl_ca_bundle = ::getenv("CURL_CA_BUNDLE");
	if (curl_ca_bundle != nullptr) {
		curl_easy_setopt(handle.ptr(), CURLOPT_CAINFO, curl_ca_bundle);
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
	return std::string(utils::bridged::get_basename(url));
}

curl_proxytype utils::get_proxy_type(const std::string& type)
{
	if (type == "http") {
		return static_cast<curl_proxytype>(CURLPROXY_HTTP);
	}
	if (type == "socks4") {
		return static_cast<curl_proxytype>(CURLPROXY_SOCKS4);
	}
	if (type == "socks5") {
		return static_cast<curl_proxytype>(CURLPROXY_SOCKS5);
	}
	if (type == "socks5h") {
		return static_cast<curl_proxytype>(CURLPROXY_SOCKS5_HOSTNAME);
	}
#ifdef CURLPROXY_SOCKS4A
	if (type == "socks4a") {
		return static_cast<curl_proxytype>(CURLPROXY_SOCKS4A);
	}
#endif

	if (type != "") {
		LOG(Level::USERERROR,
			"you configured an invalid proxy type: %s",
			type);
	}
	return static_cast<curl_proxytype>(CURLPROXY_HTTP);
}

std::string utils::unescape_url(const std::string& url)
{
	bool success = false;
	const auto result = utils::bridged::unescape_url(url, success);
	if (!success) {
		LOG(Level::DEBUG, "Rust failed to unescape url: %s", url );
		throw std::runtime_error("unescaping url failed");
	} else {
		return std::string(result);
	}
}

std::wstring utils::clean_nonprintable_characters(std::wstring text)
{
	for (size_t idx = 0; idx < text.size(); ++idx) {
		if (!iswprint(text[idx])) {
			text[idx] = L'\uFFFD';
		}
	}
	return text;
}

/* Like mkdir(), but creates ancestors (parent directories) if they don't
 * exist. */
int utils::mkdir_parents(const Filepath& p, mode_t mode)
{
	return utils::bridged::mkdir_parents(p, static_cast<std::uint32_t>(mode));
}

std::string utils::make_title(const std::string& const_url)
{
	return std::string(utils::bridged::make_title(const_url));
}

std::optional<std::uint8_t> utils::run_interactively(
	const std::string& command,
	const std::string& caller)
{
	std::uint8_t exit_code = 0;
	if (bridged::run_interactively(command, caller, exit_code)) {
		return exit_code;
	}

	return std::nullopt;
}

std::optional<std::uint8_t> utils::run_non_interactively(
	const std::string& command,
	const std::string& caller)
{
	std::uint8_t exit_code = 0;
	if (bridged::run_non_interactively(command, caller, exit_code)) {
		return exit_code;
	}

	return std::nullopt;
}

Filepath utils::getcwd()
{
	auto path = filepath::bridged::create_empty();
	utils::bridged::getcwd(*path);
	return path;
}

nonstd::expected<std::vector<std::string>, utils::ReadTextFileError> utils::read_text_file(
	const Filepath& filename)
{
	rust::Vec<rust::String> c;
	std::uint64_t error_line_number{};
	rust::String error_reason;
	const bool result = bridged::read_text_file(
			filename,
			c,
			error_line_number,
			error_reason);

	if (result) {
		std::vector<std::string> contents;
		for (const auto& line : c) {
			contents.push_back(std::string(line));
		}
		return contents;
	} else {
		ReadTextFileError error;

		if (error_line_number == 0) {
			error.kind = ReadTextFileErrorKind::CantOpen;
			error.message = strprintf::fmt(_("Failed to open file: %s"),
					std::string(error_reason));
		} else {
			error.kind = ReadTextFileErrorKind::LineError;
			error.message = strprintf::fmt(_("Failed to read line %u: %s"),
					error_line_number, std::string(error_reason));
		}

		return nonstd::make_unexpected(error);
	}
}

void utils::remove_soft_hyphens(std::string& text)
{
	rust::String tmp(text);
	utils::bridged::remove_soft_hyphens(tmp);
	text = std::string(tmp);
}

bool utils::is_valid_podcast_type(const std::string& mimetype)
{
	return utils::bridged::is_valid_podcast_type(mimetype);
}

std::optional<LinkType> utils::podcast_mime_to_link_type(
	const std::string& mimetype)
{
	std::int64_t result = 0;
	if (utils::bridged::podcast_mime_to_link_type(mimetype, result)) {
		return static_cast<LinkType>(result);
	}

	return std::nullopt;
}

std::string utils::string_from_utf8_lossy(const std::vector<std::uint8_t>& text)
{
	auto input = rust::Slice<std::uint8_t const>(text.data(), text.size());
	auto result = utils::bridged::string_from_utf8_lossy(input);
	return std::string(result);
}

void utils::parse_rss_author_email(const std::vector<std::uint8_t>& text,
	std::string& name, std::string& email)
{
	auto input = rust::Slice<std::uint8_t const>(text.data(), text.size());
	rust::String name_rs;
	rust::String email_rs;
	utils::bridged::parse_rss_author_email(input, name_rs, email_rs);
	name = std::string(name_rs);
	email = std::string(email_rs);
}

/*
 * See
 * http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
 * for a reason why we do this.
 *
 * These callbacks are deprecated as of OpenSSL 1.1.0; see the
 * changelog: https://www.openssl.org/news/changelog.html#x6
 */
#if HAVE_OPENSSL && OPENSSL_VERSION_NUMBER < 0x10100000L
static std::mutex* openssl_mutexes = nullptr;
static int openssl_mutexes_size = 0;

static void openssl_mth_locking_function(int mode, int n, const char* file,
	int line)
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

/*
 * GnuTLS 2.11.0+ uses the system availabe locking procedures and discourages
 * setting thread locks manually. See the changelog:
 * https://gitlab.com/gnutls/gnutls/-/blob/master/NEWS?ref_type=heads#L4521
 * Mind the "<=" typo in the suggestion.
 */
void utils::initialize_ssl_implementation(void)
{
#if HAVE_OPENSSL && OPENSSL_VERSION_NUMBER < 0x10100000L
	openssl_mutexes_size = CRYPTO_num_locks();
	openssl_mutexes = new std::mutex[openssl_mutexes_size];
	CRYPTO_set_id_callback(openssl_mth_id_function);
	CRYPTO_set_locking_callback(openssl_mth_locking_function);
#endif

#if HAVE_GCRYPT && GNUTLS_VERSION_NUMBER < 0x020b00
	gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
}

Filepath utils::get_default_browser()
{
	auto path = filepath::bridged::create_empty();
	utils::bridged::get_default_browser(*path);
	return path;
}

std::string utils::md5hash(const std::string& input)
{
	return std::string(utils::bridged::md5hash(input));
}

std::string utils::program_version()
{
	return std::string(utils::bridged::program_version());
}

std::string utils::mt_strf_localtime(const std::string& format, time_t t)
{
	// localtime() returns a pointer to static memory, so we need to protect
	// its caller with a mutex to ensure that no two threads concurrently
	// access that static memory. In Newsboat, the only caller for localtime()
	// is strftime(), that's why this function bakes the two together.

	static std::mutex mtx;
	std::lock_guard<std::mutex> guard(mtx);

	const size_t BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE];
	const size_t written = strftime(buffer,
			BUFFER_SIZE,
			format.c_str(),
			localtime(&t));

	return std::string(buffer, written);
}

void utils::wait_for_keypress()
{
	initscr();
	cbreak(); // Disable line buffering
	raw(); // Make sure we return from getch on Ctrl+C instead of calling the signal handler
	timeout(-1); // Make getch wait indefinitely
	getch();
	endwin(); // Restore terminal settings
}

} // namespace newsboat

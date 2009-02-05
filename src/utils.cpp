#include <utils.h>
#include <logger.h>
#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
#include <errno.h>
#include <pwd.h>
#include <libgen.h>
#include <sys/utsname.h>

#include <sstream>
#include <locale>
#include <cwchar>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

#include <curl/curl.h>

#include <langinfo.h>
#include <stfl.h>
#include <libxml/uri.h>

namespace newsbeuter {

std::vector<std::string> utils::tokenize_quoted(const std::string& str, std::string delimiters) {
	/*
	 * This function tokenizes strings, obeying quotes and throwing away comments that start
	 * with a '#'.
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
	 * 	\", \r, \n, \t and \v are replaced with the literals that you know from C/C++ strings.
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
			while (pos < str.length() && (str[pos] != '"' || (backslash_count%2))) {
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
						if (str[last_pos-1] == '\\') {
							if (attach_backslash) {
								token.append("\\");
							}
							attach_backslash = !attach_backslash;
						}
					} else {
						if (str[last_pos-1] == '\\') {
							append_escapes(token, str[last_pos]);
						} else {
							token.append(1, str[last_pos]);
						}
					}
					++last_pos;
				}
				tokens.push_back(token);
			} else {
				std::string token;
				while (last_pos < pos) {
					if (str[last_pos] == '\\') {
						if (str[last_pos-1] == '\\') {
							if (attach_backslash) {
								token.append("\\");
							}
							attach_backslash = !attach_backslash;
						}
					} else {
						if (str[last_pos-1] == '\\') {
							append_escapes(token, str[last_pos]);
						} else {
							token.append(1, str[last_pos]);
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

	LOG(LOG_DEBUG, "utils::tokenize_quoted: tokenizing '%s' resulted in %u elements", str.c_str(), tokens.size());

	return tokens;
}


std::vector<std::string> utils::tokenize(const std::string& str, std::string delimiters) {
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

std::vector<std::wstring> utils::wtokenize(const std::wstring& str, std::wstring delimiters) {
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

std::vector<std::string> utils::tokenize_spaced(const std::string& str, std::string delimiters) {
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	if (last_pos != 0) {
		tokens.push_back(std::string(" "));
	}

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		if (last_pos > pos)
			tokens.push_back(std::string(" "));
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

std::vector<std::string> utils::tokenize_nl(const std::string& str, std::string delimiters) {
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);
	unsigned int i;

	LOG(LOG_DEBUG,"utils::tokenize_nl: last_pos = %u",last_pos);
	if (last_pos != std::string::npos) {
		for (i=0;i<last_pos;++i) {
			tokens.push_back(std::string("\n"));
		}
	}

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		LOG(LOG_DEBUG,"utils::tokenize_nl: substr = %s", str.substr(last_pos, pos - last_pos).c_str());
		last_pos = str.find_first_not_of(delimiters, pos);
		LOG(LOG_DEBUG,"utils::tokenize_nl: pos - last_pos = %u", last_pos - pos);
		for (i=0;last_pos != std::string::npos && pos != std::string::npos && i<(last_pos - pos);++i) {
			tokens.push_back(std::string("\n"));
		}
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

void utils::remove_fs_lock(const std::string& lock_file) {
	LOG(LOG_DEBUG, "utils::remove_fs_lock: removed lockfile %s", lock_file.c_str());
	::unlink(lock_file.c_str());
}

bool utils::try_fs_lock(const std::string& lock_file, pid_t & pid) {
	int fd;
	// pid == 0 indicates that something went majorly wrong during locking
	pid = 0;

	LOG(LOG_DEBUG, "utils::try_fs_lock: trying to lock %s", lock_file.c_str());

	// first, we open (and possibly create) the lock file
	fd = ::open(lock_file.c_str(), O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		return false;

	// then we lock it (T_LOCK returns immediately if locking is not possible)
	if (lockf(fd, F_TLOCK, 0) == 0) {
		std::string pidtext = utils::to_s(getpid());
		// locking successful -> truncate file and write own PID into it
		ftruncate(fd, 0);
		write(fd, pidtext.c_str(), pidtext.length());
		return true;
	}

	// locking was not successful -> read PID of locking process from it
	fd = ::open(lock_file.c_str(), O_RDONLY);
	if (fd >= 0) {
		char buf[32];
		int len = read(fd, buf, sizeof(buf)-1);
		unsigned int upid = 0;
		buf[len] = '\0';
		sscanf(buf, "%u", &upid);
		pid = upid;
		close(fd);
	}
	return false;
}


std::string utils::convert_text(const std::string& text, const std::string& tocode, const std::string& fromcode) {
	std::string result;

	if (tocode == fromcode)
		return text;

	iconv_t cd = ::iconv_open((tocode + "//TRANSLIT").c_str(), fromcode.c_str());

	if (cd == reinterpret_cast<iconv_t>(-1))
		return result;

	size_t inbytesleft;
	size_t outbytesleft;

/*
 * of all the Unix-like systems around there, only Linux/glibc seems to 
 * come with a SuSv3-conforming iconv implementation.
 */
#ifndef __linux
	const char * inbufp;
#else
	char * inbufp;
#endif
	char outbuf[16];
	char * outbufp = outbuf;

	outbytesleft = sizeof(outbuf);
	inbufp = const_cast<char *>(text.c_str()); // evil, but spares us some trouble
	inbytesleft = strlen(inbufp);

	do {
		char * old_outbufp = outbufp;
		int rc = ::iconv(cd, &inbufp, &inbytesleft, &outbufp, &outbytesleft);
		if (-1 == rc) {
			switch (errno) {
				case E2BIG:
					result.append(old_outbufp, outbufp - old_outbufp);
					outbufp = outbuf;
					outbytesleft = sizeof(outbuf);
					inbufp += strlen(inbufp) - inbytesleft;
					inbytesleft = strlen(inbufp);
					break;
				case EILSEQ:
				case EINVAL:
					result.append(old_outbufp, outbufp - old_outbufp);
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

std::string utils::get_command_output(const std::string& cmd) {
	FILE * f = popen(cmd.c_str(), "r");
	std::string buf;
	char cbuf[1024];
	size_t s;
	if (f) {
		while ((s = fread(cbuf, 1, sizeof(cbuf), f)) > 0) {
			buf.append(cbuf, s);
		}
		pclose(f);
	}
	return buf;
}

void utils::extract_filter(const std::string& line, std::string& filter, std::string& url) {
	std::string::size_type pos = line.find_first_of(":", 0);
	std::string::size_type pos1 = line.find_first_of(":", pos + 1);
	filter = line.substr(pos+1, pos1 - pos - 1);
	pos = pos1;
	url = line.substr(pos+1, line.length() - pos);
	LOG(LOG_DEBUG, "utils::extract_filter: %s -> filter: %s url: %s", line.c_str(), filter.c_str(), url.c_str());
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = static_cast<std::string *>(userp);
	pbuf->append(static_cast<const char *>(buffer), size * nmemb);
	return size * nmemb;
}

std::string utils::retrieve_url(const std::string& url, const char * user_agent, const char * auth, int download_timeout) {
	std::string buf;

	CURL * easyhandle = curl_easy_init();
	if (user_agent) {
		curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, user_agent);
	}
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easyhandle, CURLOPT_ENCODING, "gzip, deflate");
	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, download_timeout);

	if (auth) {
		curl_easy_setopt(easyhandle, CURLOPT_USERPWD, auth);
		curl_easy_setopt(easyhandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	}

	curl_easy_perform(easyhandle);
	curl_easy_cleanup(easyhandle);

	LOG(LOG_DEBUG, "utils::retrieve_url(%s): %s", url.c_str(), buf.c_str());

	return buf;
}

void utils::run_command(const std::string& cmd, const std::string& input) {
	int rc = fork();
	switch (rc) {
		case -1: break;
		case 0: { // child:
			int fd = ::open("/dev/null", O_RDWR);
			close(0);
			close(1);
			close(2);
			dup2(fd, 0);
			dup2(fd, 1);
			dup2(fd, 2);
			LOG(LOG_DEBUG, "utils::run_command: %s '%s'", cmd.c_str(), input.c_str());
			execlp(cmd.c_str(), cmd.c_str(), input.c_str(), NULL);
			LOG(LOG_DEBUG, "utils::run_command: execlp of %s failed: %s", cmd.c_str(), strerror(errno));
			exit(1);
		}
		default:
			break;
	}
}

std::string utils::run_program(char * argv[], const std::string& input) {
	std::string buf;
	int ipipe[2]; int opipe[2];
	pipe(ipipe);  pipe(opipe);

	int errfd = ::open("/dev/null", O_WRONLY);

	int rc = fork();
	switch (rc) {
		case -1: break;
		case 0: { // child:
				close(ipipe[1]);   close(opipe[0]);
				dup2(ipipe[0], 0); dup2(opipe[1], 1);
				close(2);
				if (errfd != -1) dup2(errfd, 2);

				execvp(argv[0], argv);
				exit(1);
			}
			break;
		default: {
				close(ipipe[0]); close(opipe[1]);
				write(ipipe[1], input.c_str(), input.length());
				close(ipipe[1]);
				char cbuf[1024];
				int rc2;
				while ((rc2 = read(opipe[0], cbuf, sizeof(cbuf))) > 0) {
					buf.append(cbuf, rc2);
				}
				close(opipe[0]);
			}
			break;
	}
	return buf;
}

std::string utils::resolve_tilde(const std::string& str) {
	const char * homedir;
	std::string filepath;

	if (!(homedir = ::getenv("HOME"))) {
		struct passwd * spw = ::getpwuid(::getuid());
		if (spw) {
				homedir = spw->pw_dir;
		} else {
				homedir = "";
		}
	}

	if (strcmp(homedir,"")!=0) {
		if (str == "~") {
			filepath.append(homedir);
		} else if (str.substr(0,2) == "~/") {
			filepath.append(homedir);
			filepath.append(1,'/');
			filepath.append(str.substr(2,str.length()-2));
		} else {
			filepath.append(str);
		}
	} else {
		filepath.append(str);
	}

	return filepath;
}

std::string utils::replace_all(std::string str, const std::string& from, const std::string& to) {
	LOG(LOG_DEBUG,"utils::replace_all: before str = %s", str.c_str());
	std::string::size_type s = str.find(from);
	while (s != std::string::npos) {
		str.replace(s,from.length(), to);
		s = str.find(from, s + to.length());
	}
	LOG(LOG_DEBUG,"utils::replace_all: after str = %s", str.c_str());
	return str;
}

std::wstring utils::utf8str2wstr(const std::string& utf8str) {
	stfl_ipool * pool = stfl_ipool_create("utf-8");
	std::wstring wstr = stfl_ipool_towc(pool, utf8str.c_str());
	stfl_ipool_destroy(pool);
	return wstr;
}

std::wstring utils::str2wstr(const std::string& str) {
	const char * codeset = nl_langinfo(CODESET);
	struct stfl_ipool * ipool = stfl_ipool_create(codeset);
	std::wstring result = stfl_ipool_towc(ipool, str.c_str());
	stfl_ipool_destroy(ipool);
	return result;
}

std::string utils::wstr2str(const std::wstring& wstr) {
	const char * codeset = nl_langinfo(CODESET);
	struct stfl_ipool * ipool = stfl_ipool_create(codeset);
	std::string result = stfl_ipool_fromwc(ipool, wstr.c_str());
	stfl_ipool_destroy(ipool);
	return result;
}

std::string utils::to_s(unsigned int u) {
	std::ostringstream os;
	os << u;
	return os.str();
}

std::string utils::absolute_url(const std::string& url, const std::string& link) {
	xmlChar * newurl = xmlBuildURI((const xmlChar *)link.c_str(), (const xmlChar *)url.c_str());
	std::string retval;
	if (newurl) {
		retval = (const char *)newurl;
		xmlFree(newurl);
	} else {
		retval = link;
	}
	return retval;
}

std::string utils::strprintf(const char * format, ...) {
	if (!format)
		return std::string("");

	va_list ap;
	va_start(ap, format);

	unsigned int len = vsnprintf(NULL, 0, format, ap);

	va_end(ap);
	va_start(ap, format);

	char * buf = new char[len + 1];
	vsnprintf(buf, len + 1, format, ap);
	va_end(ap);

	std::string ret(buf);
	delete[] buf;

	return ret;
}

std::string utils::get_useragent(configcontainer * cfgcont) {
	std::string ua_pref = cfgcont->get_configvalue("user-agent");
	if (ua_pref.length() == 0) {
		struct utsname buf;
		uname(&buf);
		return utils::strprintf("%s/%s (%s %s; %s; %s) %s", PROGRAM_NAME, PROGRAM_VERSION, buf.sysname, buf.release, buf.machine, PROGRAM_URL, curl_version());
	}
	return ua_pref;
}

unsigned int utils::to_u(const std::string& str) {
	std::istringstream is(str);
	unsigned int u = 0;
	is >> u;
	return u;
}

scope_measure::scope_measure(const std::string& func, loglevel ll) : lvl(ll) {
	funcname = func;
	gettimeofday(&tv1, NULL);
}

void scope_measure::stopover(const std::string& son) {
	gettimeofday(&tv2, NULL);
	unsigned long diff = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) - tv1.tv_usec;
	LOG(lvl, "scope_measure: function `%s' (stop over `%s') took %lu.%06lu s so far", funcname.c_str(), son.c_str(), diff / 1000000, diff % 1000000);
}

scope_measure::~scope_measure() {
	gettimeofday(&tv2, NULL);
	unsigned long diff = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) - tv1.tv_usec;
	LOG(LOG_INFO, "scope_measure: function `%s' took %lu.%06lu s", funcname.c_str(), diff / 1000000, diff % 1000000);
}

void utils::append_escapes(std::string& str, char c) {
	switch (c) {
		case 'n': str.append("\n"); break;
		case 'r': str.append("\r"); break;
		case 't': str.append("\t"); break;
		case '"': str.append("\""); break;
		case '\\': break;
		default: str.append(1, c); break;
	}
}

bool utils::is_valid_color(const std::string& color) {
	const char * colors[] = { "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "default", NULL };
	for (unsigned int i=0;colors[i];i++) {
		if (color == colors[i])
			return true;
	}
	return false;
}

bool utils::is_valid_attribute(const std::string& attrib) {
	const char * attribs[] = { "standout", "underline", "reverse", "blink", "dim", "bold", "protect", "invis", "default", NULL };
	for (unsigned int i=0;attribs[i];i++) {
		if (attrib == attribs[i])
			return true;
	}
	return false;
}

std::vector<std::pair<unsigned int, unsigned int> > utils::partition_indexes(unsigned int start, unsigned int end, unsigned int parts) {
	std::vector<std::pair<unsigned int, unsigned int> > partitions;
	unsigned int count = end - start + 1;
	unsigned int size = count / parts;

	for (unsigned int i=0;i<parts-1;i++) {
		partitions.push_back(std::pair<unsigned int, unsigned int>(start, start + size - 1));
		start += size;
	}

	partitions.push_back(std::pair<unsigned int, unsigned int>(start, end));
	return partitions;
}

size_t utils::strwidth(const std::string& str) {
	std::wstring wstr = str2wstr(str);
	return wcswidth(wstr.c_str(), wstr.length());
}

std::string utils::join(const std::vector<std::string>& strings, const std::string& separator) {
	std::string result;

	for (std::vector<std::string>::const_iterator it=strings.begin();it!=strings.end();it++) {
		result.append(*it);
		result.append(separator);
	}

	if (result.length() > 0)
		result.erase(result.length()-separator.length(), result.length());

	return result;
}

std::string utils::censor_url(const std::string& url) {
	std::string rv;
	if (url.length() > 0) {
		const char * myuri = url.c_str();
		xmlURIPtr uri = xmlParseURI(myuri);
		if (uri) {
			if (uri->user) {
				xmlFree(uri->user);
				uri->user = (char *)xmlStrdup((const xmlChar *)"*:*");
			}
			xmlChar * uristr = xmlSaveUri(uri);
			
			rv = (const char *)uristr;
			xmlFree(uristr);
			xmlFreeURI(uri);
		} else
			return url;
	}
	return rv;
}

std::string utils::quote_for_stfl(std::string str) {
	unsigned int len = str.length();
	for (unsigned int i=0;i<len;++i) {
		if (str[i] == '<') {
			str.insert(i+1, ">");
			++len;
		}
	}
	return str;
}

void utils::trim_end(std::string& str) {
	std::string::size_type pos = str.length()-1;
	while (str.length()>0 && (str[pos] == '\n' || str[pos] == '\r')) {
		str.erase(pos);
		pos--;
	}
}

std::string utils::quote(const std::string& str) {
	std::string rv = replace_all(str, "\"", "\\\"");
	rv.insert(0, "\"");
	rv.append("\"");
	return rv;
}

unsigned int utils::get_random_value(unsigned int max) {
	static bool initialized = false;
	unsigned int var;
	if (!initialized) {
		initialized = true;
		srand(~(time(NULL) ^ getpid() ^ getppid() ^ var));
	}
	return static_cast<unsigned int>(rand() % max);
}

}

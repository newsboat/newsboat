#include <utils.h>
#include <logger.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
#include <errno.h>

#include <curl/curl.h>

namespace newsbeuter {

std::vector<std::string> utils::tokenize_quoted(const std::string& str, std::string delimiters) {
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
							switch (str[last_pos]) {
								case 'n': token.append("\n"); break;
								case 'r': token.append("\r"); break;
								case 't': token.append("\t"); break;
								case '"': token.append("\""); break;
								case '\\': break;
								default: token.append(1, str[last_pos]); break;
							}
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
							switch (str[last_pos]) {
								case 'n': token.append("\n"); break;
								case 'r': token.append("\r"); break;
								case 't': token.append("\t"); break;
								case '"': token.append("\""); break;
								case '\\': break;
								default: token.append(1, str[last_pos]); break;
							}
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

	GetLogger().log(LOG_DEBUG, "utils::tokenize_quoted: tokenizing '%s' resulted in %u elements", str.c_str(), tokens.size());

	return tokens;
}

	
std::vector<std::string> utils::tokenize(const std::string& str, std::string delimiters) {
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

	GetLogger().log(LOG_DEBUG,"utils::tokenize_nl: last_pos = %u",last_pos);
	for (i=0;i<last_pos;++i) {
		tokens.push_back(std::string("\n"));
	}

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		GetLogger().log(LOG_DEBUG,"utils::tokenize_nl: substr = %s", str.substr(last_pos, pos - last_pos).c_str());
		last_pos = str.find_first_not_of(delimiters, pos);
		GetLogger().log(LOG_DEBUG,"utils::tokenize_nl: pos - last_pos = %u", last_pos - pos);
		for (i=0;last_pos != std::string::npos && pos != std::string::npos && i<(last_pos - pos);++i) {
			tokens.push_back(std::string("\n"));
		}
		pos = str.find_first_of(delimiters, last_pos);
	}

	return tokens;
}

void utils::remove_fs_lock(const std::string& lock_file) {
	GetLogger().log(LOG_DEBUG, "removed lockfile %s", lock_file.c_str());
	::unlink(lock_file.c_str());
}

bool utils::try_fs_lock(const std::string& lock_file, pid_t & pid) {
	int fd;
	// pid == 0 indicates that something went majorly wrong during locking
	pid = 0;

	// first, we open (and possibly create) the lock file
	fd = ::open(lock_file.c_str(), O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		return false;

	// then we lock it (T_LOCK returns immediately if locking is not possible)
	if (lockf(fd, F_TLOCK, 0) == 0) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%u", getpid());
		// locking successful -> truncate file and write own PID into it
		ftruncate(fd, 0);
		write(fd, buf, strlen(buf));
		return true;
	}

	// locking was not successful -> read PID of locking process from it
	fd = ::open(lock_file.c_str(), O_RDONLY);
	if (fd >= 0) {
		char buf[32];
		int len = read(fd, buf, sizeof(buf)-1);
		buf[len] = '\0';
		sscanf(buf, "%u", &pid);
		close(fd);
	}
	return false;
}


std::string utils::convert_text(const std::string& text, const std::string& tocode, const std::string& fromcode) {
	std::string result;

	if (tocode == fromcode)
		return text;

	iconv_t cd = ::iconv_open((tocode + "//TRANSLIT").c_str(), fromcode.c_str());

	if (cd == (iconv_t)-1)
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
					GetLogger().log(LOG_DEBUG, "utils::convert_text: hit EILSEQ/EINVAL: result = `%s'", result.c_str());
					inbufp += strlen(inbufp) - inbytesleft + 1;
					GetLogger().log(LOG_DEBUG, "utils::convert_text: new inbufp: `%s'", inbufp);
					inbytesleft = strlen(inbufp);
					break;
			}
		} else {
			result.append(old_outbufp, outbufp - old_outbufp);
		}
	} while (inbytesleft > 0);

	GetLogger().log(LOG_DEBUG, "utils::convert_text: before: %s", text.c_str());
	GetLogger().log(LOG_DEBUG, "utils::convert_text: after:  %s", result.c_str());

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
	GetLogger().log(LOG_DEBUG, "utils::extract_filter: %s -> filter: %s url: %s", line.c_str(), filter.c_str(), url.c_str());
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = (std::string *)userp;
	pbuf->append((const char *)buffer, size * nmemb);
	return size * nmemb;
}

std::string utils::retrieve_url(const std::string& url, const char * user_agent) {
	std::string buf;

	CURL * easyhandle = curl_easy_init();
	if (user_agent) {
		curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, user_agent);
	}
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);
	curl_easy_perform(easyhandle);

	GetLogger().log(LOG_DEBUG, "utils::retrieve_url(%s): %s", url.c_str(), buf.c_str());

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
			execlp(cmd.c_str(), cmd.c_str(), input.c_str(), NULL);
			exit(1);
		}
	}
}

std::string utils::run_filter(const std::string& cmd, const std::string& input) {
	std::string buf;
	int ipipe[2];
	int opipe[2];
	pipe(ipipe);
	pipe(opipe);

	int rc = fork();
	switch (rc) {
		case -1: break;
		case 0: { // child:
				close(ipipe[1]);
				close(opipe[0]);
				dup2(ipipe[0], 0);
				dup2(opipe[1], 1);
				execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
				exit(1);
			}
			break;
		default: {
				close(ipipe[0]);
				close(opipe[1]);
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

}

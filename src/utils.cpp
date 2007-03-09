#include <utils.h>
#include <logger.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>
#include <errno.h>

namespace newsbeuter {

std::vector<std::string> utils::tokenize_quoted(const std::string& str, std::string delimiters) {
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = last_pos;

	while (pos != std::string::npos && last_pos != std::string::npos) {
		if (str[last_pos] == '#') // stop as soon as we found a comment
			break;

		if (str[last_pos] == '"') {
			++last_pos;
			pos = last_pos;
			while (pos < str.length() && (str[pos] != '"' || str[pos-1] == '\\'))
				++pos;
			if (pos >= str.length()) {
				pos = std::string::npos;
				std::string token;
				while (last_pos < str.length()) {
					if (str[last_pos] == '\\') {
						if (str[last_pos-1] == '\\')
							token.append("\\");
					} else {
						if (str[last_pos-1] == '\\') {
							switch (str[last_pos]) {
								case 'n': token.append("\n"); break;
								case 'r': token.append("\r"); break;
								case 't': token.append("\t"); break;
								case '"': token.append("\""); break;
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
						if (str[last_pos-1] == '\\')
							token.append("\\");
					} else {
						if (str[last_pos-1] == '\\') {
							switch (str[last_pos]) {
								case 'n': token.append("\n"); break;
								case 'r': token.append("\r"); break;
								case 't': token.append("\t"); break;
								case '"': token.append("\""); break;
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
	if ((fd = ::open(lock_file.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0) {
		pid = ::getpid();
		::write(fd,&pid,sizeof(pid));
		::close(fd);
		GetLogger().log(LOG_DEBUG,"wrote lockfile %s with pid = %u",lock_file.c_str(), pid);
		return true;
	} else {
		pid = 0;
		if ((fd = ::open(lock_file.c_str(), O_RDONLY)) >=0) {
			::read(fd,&pid,sizeof(pid));
			::close(fd);
			GetLogger().log(LOG_DEBUG,"found lockfile %s", lock_file.c_str());
		} else {
			GetLogger().log(LOG_DEBUG,"found lockfile %s, but couldn't open it for reading from it", lock_file.c_str());
		}
		return false;
	}
}


std::string utils::convert_text(const std::string& text, const std::string& tocode, const std::string& fromcode) {
	std::string result;

	if (tocode == fromcode)
		return text;

	iconv_t cd = ::iconv_open(tocode.c_str(), fromcode.c_str());

	if (cd == (iconv_t)-1)
		return result;

	size_t inbytesleft;
	size_t outbytesleft;
	char * inbufp;
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

	return result;
}


}

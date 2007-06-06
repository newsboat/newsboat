#include <fstream>

#include <urlreader.h>
#include <utils.h>

using namespace newsbeuter;

urlreader::urlreader(const std::string& file) : filename(file) {
	reload();
}

urlreader::~urlreader() { }

std::vector<std::string>& urlreader::get_urls() {
	return urls;
}

void urlreader::reload() {
	std::fstream f;
	f.open(filename.c_str(),std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f,line);
			if (!f.eof() && line.length() > 0 && line[0] != '#') {
				std::vector<std::string> tokens = utils::tokenize_quoted(line);
				if (tokens.size() > 0) {
					std::string url = tokens[0];
					urls.push_back(url);
					tokens.erase(tokens.begin());
					if (tokens.size() > 0) {
						tags[url] = tokens;
						for (std::vector<std::string>::iterator it=tokens.begin();it!=tokens.end();++it) {
							alltags.insert(*it);
						}
					}
				}
			}
		} while (!f.eof());
	}
}

void urlreader::load_config(const std::string& file) {
	filename = file;
	reload();
}

void urlreader::write_config() {
	std::fstream f;
	f.open(filename.c_str(),std::fstream::out);
	if (f.is_open()) {
		for (std::vector<std::string>::iterator it=urls.begin(); it != urls.end(); ++it) {
			f << *it;
			if (tags[*it].size() > 0) {
				for (std::vector<std::string>::iterator jt=tags[*it].begin();jt!=tags[*it].end();++jt) {
					f << " \"" << *jt << "\"";
				}
			}
			f << std::endl;
		}
	}
}

std::vector<std::string>& urlreader::get_tags(const std::string& url) {
	return tags[url];
}

std::vector<std::string> urlreader::get_alltags() {
	std::vector<std::string> tmptags;
	for (std::set<std::string>::iterator it=alltags.begin();it!=alltags.end();++it) {
		tmptags.push_back(*it);
	}
	return tmptags;
}

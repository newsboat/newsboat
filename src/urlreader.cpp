#include <fstream>

#include <urlreader.h>

using namespace noos;

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
			if (!f.eof() && line.length() > 0 && line[0] != '#')
				urls.push_back(line);
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
			f << *it << std::endl;
		}
	}
}

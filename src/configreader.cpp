#include <fstream>

#include <configreader.h>

using namespace noos;

configreader::configreader(const std::string& file) : filename(file) {
	reload();
}

configreader::~configreader() { }

const std::vector<std::string>& configreader::get_urls() {
	return urls;
}

void configreader::reload() {
	std::fstream f;
	f.open(filename.c_str(),std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f,line);
			if (!f.eof())
				urls.push_back(line);
		} while (!f.eof());
	}
}

void configreader::load_config(const std::string& file) {
	filename = file;
	reload();
}

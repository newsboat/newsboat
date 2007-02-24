#include <queueloader.h>
#include <fstream>

namespace podbeuter {

queueloader::queueloader(const std::string& file, pb_controller * c) : queuefile(file), ctrl(c) {
}

void queueloader::load(std::vector<download>& downloads) {
	std::fstream f;
	f.open(queuefile.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f, line);
			if (!f.eof() && line.length() > 0) {
				download d(ctrl);
				d.set_filename(get_filename(line));
				d.set_url(line);
				downloads.push_back(d);
			}
		} while (!f.eof());
	}
}

std::string queueloader::get_filename(const std::string& str) {
	std::string fn;
	std::string dlpath = ctrl->get_dlpath();
	if (dlpath.substr(0,2) == "~/") {
		char * homedir = ::getenv("HOME");
		if (homedir) {
			fn.append(homedir);
			fn.append("/");
			fn.append(dlpath.substr(2,dlpath.length()-2));
		} else {
			fn = ".";
		}
	} else {
		fn = dlpath;
	}
	fn.append("/");
	char * base = basename(str.c_str());
	if (!base || strlen(base) == 0) {
		char buf[128];
		time_t t = time(NULL);
		strftime(buf, sizeof(buf), "%Y-%b-%d-%H%M%S.unknown", localtime(&t));
		fn.append(buf);
	} else {
		fn.append(base);
	}
	return fn;
}

}

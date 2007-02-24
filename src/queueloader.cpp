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
				d.set_filename(line);
				d.set_url(line);
				downloads.push_back(d);
			}
		} while (!f.eof());
	}
}

}

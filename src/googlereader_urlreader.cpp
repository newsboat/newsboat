#include <google_api.h>

namespace newsbeuter {

googlereader_urlreader::googlereader_urlreader(configcontainer * c, remote_api * a) : cfg(c), api(a) { }

googlereader_urlreader::~googlereader_urlreader() { }

void googlereader_urlreader::write_config() {
	// NOTHING
}

void googlereader_urlreader::reload() {
	// TODO: implement
}

std::string googlereader_urlreader::get_source() {
	// TODO: implement
	return "Google Reader";
}

}

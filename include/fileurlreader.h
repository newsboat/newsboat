#ifndef NEWSBOAT_FILEURLREADER_H_
#define NEWSBOAT_FILEURLREADER_H_

#include <string>

#include "urlreader.h"

namespace newsboat {

class file_urlreader : public urlreader {
public:
	explicit file_urlreader(const std::string& file = "");
	void write_config() override;
	void reload() override;
	void load_config(const std::string& file);
	std::string get_source() override;

private:
	std::string filename;
};

}

#endif /* NEWSBOAT_FILEURLREADER_H_ */

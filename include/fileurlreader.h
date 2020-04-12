#ifndef NEWSBOAT_FILEURLREADER_H_
#define NEWSBOAT_FILEURLREADER_H_

#include <string>

#include "urlreader.h"

namespace newsboat {

class FileUrlReader : public UrlReader {
public:
	explicit FileUrlReader(const std::string& file = "");
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	const std::string filename;
};

}

#endif /* NEWSBOAT_FILEURLREADER_H_ */

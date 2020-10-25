#ifndef NEWSBOAT_FILEURLREADER_H_
#define NEWSBOAT_FILEURLREADER_H_

#include <string>

#include "urlreader.h"

namespace newsboat {

class FileUrlReader : public UrlReader {
public:
	explicit FileUrlReader(const std::string& file = "");

	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

	/// \brief Write URLs back to the input file.
	///
	/// This method is used after importing feeds from OPML.
	void write_config();

private:
	const std::string filename;
};

}

#endif /* NEWSBOAT_FILEURLREADER_H_ */

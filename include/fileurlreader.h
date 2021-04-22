#ifndef NEWSBOAT_FILEURLREADER_H_
#define NEWSBOAT_FILEURLREADER_H_

#include <string>

#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class FileUrlReader : public UrlReader {
public:
	explicit FileUrlReader(const std::string& file = "");

	/// \brief Load URLs from the urls file.
	///
	/// \return A non-value on success, an error message otherwise.
	nonstd::optional<std::string> reload() override;

	std::string get_source() override;

	/// \brief Write URLs back to the urls file.
	///
	/// \return A non-value on success, an error message otherwise.
	nonstd::optional<std::string> write_config();

private:
	const Utf8String filename;
};

}

#endif /* NEWSBOAT_FILEURLREADER_H_ */

#ifndef NEWSBOAT_FILEURLREADER_H_
#define NEWSBOAT_FILEURLREADER_H_

#include <string>

#include "urlreader.h"

namespace newsboat {

class FileUrlReader : public UrlReader {
public:
	explicit FileUrlReader(const Filepath& file = "");

	/// \brief Load URLs from the urls file.
	///
	/// \return A non-value on success, a structure with error info otherwise.
	std::optional<utils::ReadTextFileError> reload() override;

	std::string get_source() const override;

	void add_url(const std::string& url, const std::vector<std::string>& url_tags);

	/// \brief Write URLs back to the urls file.
	///
	/// \return A non-value on success, an error message otherwise.
	std::optional<std::string> write_config();

private:
	const Filepath filename;
};

}

#endif /* NEWSBOAT_FILEURLREADER_H_ */

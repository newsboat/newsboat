#ifndef NEWSBOAT_URLREADER_H_
#define NEWSBOAT_URLREADER_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "filepath.h"
#include "utils.h"

namespace newsboat {

/// \brief Base class for classes that supply Newsboat with feed URLs.
class UrlReader {
public:
	UrlReader() = default;
	virtual ~UrlReader() = default;

	/// \brief Re-read the input file.
	///
	/// \note This overwrites the contents of `urls` and `tags`, so
	/// make sure to save your modifications with `write_config()`.
	virtual std::optional<utils::ReadTextFileError> reload() = 0;

	/// \brief User-visible description of where URLs come from.
	///
	/// This can be a path (e.g. ~/.newsboat/urls), a URL (e.g. the value of
	/// `opml-url` setting), or a name of the remote API in use (e.g. "Tiny
	/// Tiny RSS").
	virtual std::string get_source() const = 0;

	/// \brief A list of feed URLs.
	const std::vector<std::string>& get_urls() const;

	/// \brief Tags of feed that has url `url`.
	std::vector<std::string>& get_tags(const std::string& url);

	/// \brief List of all extant tags.
	std::vector<std::string> get_alltags() const;

protected:
	void load_query_urls_from_file(Filepath file);

	std::vector<std::string> urls;
	std::map<std::string, std::vector<std::string>> tags;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLREADER_H_ */

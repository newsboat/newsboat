#ifndef NEWSBOAT_URLREADER_H_
#define NEWSBOAT_URLREADER_H_

#include <optional>
#include <string>
#include <vector>

#include "feedorigin.h"
#include "filepath.h"
#include "utils.h"

namespace newsboat {

/// \brief Base class for classes that supply Newsboat with feed URLs.
class UrlReader {
public:
	struct FeedUrl {
		std::string url;
		FeedOrigin origin;
		std::vector<std::string> tags;
	};

	UrlReader() = default;
	virtual ~UrlReader() = default;

	/// \brief Re-read the input file.
	virtual std::optional<utils::ReadTextFileError> reload() = 0;

	/// \brief User-visible description of where URLs come from.
	///
	/// This can be a path (e.g. ~/.newsboat/urls), a URL (e.g. the value of
	/// `opml-url` setting), or a name of the remote API in use (e.g. "Tiny
	/// Tiny RSS").
	virtual std::string get_source() const = 0;

	/// \brief A list of feed URLs with their origin and tags
	const std::vector<UrlReader::FeedUrl>& get_urls() const;

	/// \brief Get entry of feed that has url `url`
	/// (or nullptr if it does not exist)
	FeedUrl* get_entry(const std::string& url);

	/// \brief List of all extant tags.
	std::vector<std::string> get_alltags() const;

protected:
	void load_query_urls_from_file(Filepath file);

	std::vector<FeedUrl> feed_urls;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLREADER_H_ */

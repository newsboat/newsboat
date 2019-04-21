#ifndef NEWSBOAT_URLREADER_H_
#define NEWSBOAT_URLREADER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace newsboat {

/// \brief Base class for classes that supply Newsboat with feed URLs.
class UrlReader {
public:
	UrlReader() = default;
	virtual ~UrlReader() = default;

	/// \brief Write URLs back to the input file.
	///
	/// This method is used after importing feeds from OPML.
	virtual void write_config() = 0;

	/// \brief Re-read the input file.
	///
	/// \note This overwrites the contents of `urls`, `tags`, and `alltags`, so
	/// make sure to save your modifications with `write_config()`.
	virtual void reload() = 0;

	/// \brief User-visible description of where URLs come from.
	///
	/// This can be a path (e.g. ~/.newsboat/urls), a URL (e.g. the value of
	/// `opml-url` setting), or a name of the remote API in use (e.g. "Tiny
	/// Tiny RSS").
	virtual std::string get_source() = 0;

	/// \brief A list of feed URLs.
	std::vector<std::string>& get_urls();

	/// \brief Tags of feed that has url `url`.
	std::vector<std::string>& get_tags(const std::string& url);

	/// \brief List of all extant tags.
	std::vector<std::string> get_alltags();

protected:
	std::vector<std::string> urls;
	std::map<std::string, std::vector<std::string>> tags;
	std::set<std::string> alltags;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLREADER_H_ */

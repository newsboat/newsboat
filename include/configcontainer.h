#ifndef NEWSBOAT_CONFIGCONTAINER_H_
#define NEWSBOAT_CONFIGCONTAINER_H_

#include "libnewsboat-ffi/src/configcontainer.rs.h" // IWYU pragma: export

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "3rd-party/expected.hpp"
#include "configactionhandler.h"
#include "filepath.h"

namespace newsboat {

class ConfigData;
class ConfigParser;

enum class FeedSortMethod {
	NONE,
	FIRST_TAG,
	TITLE,
	ARTICLE_COUNT,
	UNREAD_ARTICLE_COUNT,
	LAST_UPDATED,
	LATEST_UNREAD
};

enum class ArtSortMethod { TITLE, FLAGS, AUTHOR, LINK, GUID, DATE, RANDOM };

enum class SortDirection { ASC, DESC };

struct FeedSortStrategy {
	FeedSortMethod sm = FeedSortMethod::NONE;
	SortDirection sd = SortDirection::DESC;

	bool operator==(const FeedSortStrategy& other) const
	{
		return sm == other.sm && sd == other.sd;
	}

	bool operator!=(const FeedSortStrategy& other) const
	{
		return !(*this == other);
	}
};

struct ArticleSortStrategy {
	ArtSortMethod sm = ArtSortMethod::DATE;
	SortDirection sd = SortDirection::ASC;

	bool operator==(const ArticleSortStrategy& other) const
	{
		return sm == other.sm && sd == other.sd;
	}

	bool operator!=(const ArticleSortStrategy& other) const
	{
		return !(*this == other);
	}
};

class ConfigContainer : public ConfigActionHandler {
public:
	ConfigContainer();
	~ConfigContainer() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;

	bool get_configvalue_as_bool(const std::string& key) const;
	int get_configvalue_as_int(const std::string& key) const;
	Filepath get_configvalue_as_filepath(const std::string& key) const;
	std::string get_configvalue(const std::string& key) const;
	nonstd::expected<void, std::string> set_configvalue(const std::string& key,
		const std::string& value);
	void reset_to_default(const std::string& key);
	void toggle(const std::string& key);
	std::vector<std::string> get_suggestions(const std::string& fragment) const;
	FeedSortStrategy get_feed_sort_strategy() const;
	ArticleSortStrategy get_article_sort_strategy() const;

	static const std::string PARTIAL_FILE_SUFFIX;

private:
	rust::Box<configcontainer::bridged::ConfigContainer> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGCONTAINER_H_ */

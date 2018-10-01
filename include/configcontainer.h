#ifndef NEWSBOAT_CONFIGCONTAINER_H_
#define NEWSBOAT_CONFIGCONTAINER_H_

#include <unordered_set>

#include "configparser.h"

namespace newsboat {

enum class configdata_t { INVALID, BOOL, INT, STR, PATH, ENUM };

enum class feed_sort_method_t {
	NONE,
	FIRST_TAG,
	TITLE,
	ARTICLE_COUNT,
	UNREAD_ARTICLE_COUNT,
	LAST_UPDATED
};

enum class art_sort_method_t { TITLE, FLAGS, AUTHOR, LINK, GUID, DATE };

enum class sort_direction_t { ASC, DESC };

struct FeedSortStrategy {
	feed_sort_method_t sm;
	sort_direction_t sd;
};

struct ArticleSortStrategy {
	art_sort_method_t sm;
	sort_direction_t sd;

	bool operator==(const ArticleSortStrategy& other) const
	{
		return sm == other.sm && sd == other.sd;
	}

	bool operator!=(const ArticleSortStrategy& other) const
	{
		return !(*this == other);
	}
};

struct configdata {
	configdata(const std::string& v = "",
		configdata_t t = configdata_t::INVALID,
		bool m = false)
		: value(v)
		, default_value(v)
		, type(t)
		, enum_values()
		, multi_option(m)
	{
	}

	configdata(const std::string& v,
		const std::unordered_set<std::string>& values)
		: value(v)
		, default_value(v)
		, type(configdata_t::ENUM)
		, enum_values(values)
		, multi_option(false)
	{
	}

	std::string value;
	std::string default_value;
	configdata_t type;
	const std::unordered_set<std::string> enum_values;
	bool multi_option;
};

class ConfigContainer : public ConfigActionHandler {
public:
	ConfigContainer();
	~ConfigContainer() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;

	bool get_configvalue_as_bool(const std::string& key);
	int get_configvalue_as_int(const std::string& key);
	std::string get_configvalue(const std::string& key);
	void set_configvalue(const std::string& key, const std::string& value);
	void reset_to_default(const std::string& key);
	void toggle(const std::string& key);
	std::vector<std::string> get_suggestions(const std::string& fragment);
	FeedSortStrategy get_feed_sort_strategy();
	ArticleSortStrategy get_article_sort_strategy();

	static const std::string PARTIAL_FILE_SUFFIX;

private:
	std::map<std::string, configdata> config_data;

	bool is_bool(const std::string& s);
	bool is_int(const std::string& s);
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGCONTAINER_H_ */

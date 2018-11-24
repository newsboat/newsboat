#include "queuemanager.h"

#include <fstream>
#include <libxml/uri.h>

#include "configpaths.h"
#include "fmtstrformatter.h"
#include "rss.h"
#include "stflpp.h"
#include "utils.h"

namespace newsboat {

QueueManager::QueueManager(ConfigContainer* cfg_, ConfigPaths* paths_)
	: cfg(cfg_)
	, paths(paths_)
{}

void QueueManager::enqueue_url(std::shared_ptr<RssItem> item,
	std::shared_ptr<RssFeed> feed)
{
	const std::string& url = item->enclosure_url();
	bool url_found = false;
	std::fstream f;
	f.open(paths->queue_file(), std::fstream::in);
	if (f.is_open()) {
		do {
			std::string line;
			getline(f, line);
			if (!f.eof() && !line.empty()) {
				std::vector<std::string> fields =
					utils::tokenize_quoted(line);
				if (!fields.empty() && fields[0] == url) {
					url_found = true;
					break;
				}
			}
		} while (!f.eof());
		f.close();
	}
	if (!url_found) {
		f.open(paths->queue_file(),
			std::fstream::app | std::fstream::out);
		const std::string filename =
			generate_enqueue_filename(item, feed);
		f << url << " " << Stfl::quote(filename) << std::endl;
		f.close();
	}
}

std::string get_hostname_from_url(const std::string& url)
{
	xmlURIPtr uri = xmlParseURI(url.c_str());
	std::string hostname;
	if (uri) {
		hostname = uri->server;
		xmlFreeURI(uri);
	}
	return hostname;
}

std::string QueueManager::generate_enqueue_filename(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed)
{
	const std::string& url = item->enclosure_url();
	const std::string& title = item->title();
	const time_t pubDate = item->pubDate_timestamp();

	std::string dlformat = cfg->get_configvalue("download-path");
	if (dlformat[dlformat.length() - 1] != NEWSBEUTER_PATH_SEP[0]) {
		dlformat.append(NEWSBEUTER_PATH_SEP);
	}

	const std::string filemask = cfg->get_configvalue("download-filename-format");
	dlformat.append(filemask);

	auto time_formatter = [&pubDate](const char* format) {
		char pubDate_formatted[1024];
		strftime(pubDate_formatted,
			sizeof(pubDate_formatted),
			format,
			localtime(&pubDate));
		return std::string(pubDate_formatted);
	};

	const std::string base = utils::get_basename(url);
	std::string extension;
	const std::size_t pos = base.rfind('.');
	if (pos != std::string::npos) {
		extension.append(base.substr(pos + 1));
	}

	FmtStrFormatter fmt;
	fmt.register_fmt('n', feed->title());
	fmt.register_fmt('h', get_hostname_from_url(url));
	fmt.register_fmt('u', base);
	fmt.register_fmt('F', time_formatter("%F"));
	fmt.register_fmt('m', time_formatter("%m"));
	fmt.register_fmt('b', time_formatter("%b"));
	fmt.register_fmt('d', time_formatter("%d"));
	fmt.register_fmt('H', time_formatter("%H"));
	fmt.register_fmt('M', time_formatter("%M"));
	fmt.register_fmt('S', time_formatter("%S"));
	fmt.register_fmt('y', time_formatter("%y"));
	fmt.register_fmt('Y', time_formatter("%Y"));
	fmt.register_fmt('t', title);
	fmt.register_fmt('e', extension);

	if (feed->rssurl() != item->feedurl() &&
		item->get_feedptr() != nullptr) {
		std::string feedtitle = utils::quote_for_stfl(
			item->get_feedptr()->title());
		utils::remove_soft_hyphens(feedtitle);
		fmt.register_fmt('N', feedtitle);
	} else {
		fmt.register_fmt('N', feed->title());
    }

	const std::string dlpath = fmt.do_format(dlformat);
	return dlpath;
}

void QueueManager::autoenqueue(std::shared_ptr<RssFeed> feed)
{
	if (!cfg->get_configvalue_as_bool("podcast-auto-enqueue")) {
		return;
	}

	std::lock_guard<std::mutex> lock(feed->item_mutex);
	for (const auto& item : feed->items()) {
		if (!item->enqueued() && item->enclosure_url().length() > 0) {
			LOG(Level::DEBUG,
				"QueueManager::autoenqueue: enclosure_url = "
				"`%s' "
				"enclosure_type = `%s'",
				item->enclosure_url(),
				item->enclosure_type());
			if (utils::is_http_url(item->enclosure_url())) {
				LOG(Level::INFO,
					"QueueManager::autoenqueue: enqueuing "
					"`%s'",
					item->enclosure_url());
				enqueue_url(item, feed);
				item->set_enqueued(true);
			}
		}
	}
}

} // namespace newsboat

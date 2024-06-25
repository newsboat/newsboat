#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "queuemanager.h"

#include <fstream>
#include <libxml/uri.h>

#include "fmtstrformatter.h"
#include "logger.h"
#include "rssfeed.h"
#include "utils.h"

namespace newsboat {

QueueManager::QueueManager(ConfigContainer* cfg_, Filepath queue_file)
	: cfg(cfg_)
	, queue_file(std::move(queue_file))
{}

EnqueueResult QueueManager::enqueue_url(RssItem& item, RssFeed& feed)
{
	const std::string& url = item.enclosure_url();
	const Filepath filename = generate_enqueue_filename(item, feed);

	std::fstream f;
	f.open(queue_file, std::fstream::in);
	if (f.is_open()) {
		do {
			std::string line;
			getline(f, line);
			if (!f.eof() && !line.empty()) {
				const auto fields = utils::tokenize_quoted(line);
				if (fields.size() >= 1 && fields[0] == url) {
					return {EnqueueStatus::URL_QUEUED_ALREADY, url};
				}
				if (fields.size() >= 2 && fields[1] == filename) {
					return {EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY, filename};
				}
			}
		} while (!f.eof());
		f.close();
	}

	f.open(queue_file, std::fstream::app | std::fstream::out);
	if (!f.is_open()) {
		return {EnqueueStatus::QUEUE_FILE_OPEN_ERROR, queue_file};
	}
	f << url << " " << utils::quote(filename) << std::endl;
	f.close();

	item.set_enqueued(true);

	return {EnqueueStatus::QUEUED_SUCCESSFULLY, ""};
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

Filepath QueueManager::generate_enqueue_filename(
	RssItem& item,
	RssFeed& feed)
{
	const std::string& url = item.enclosure_url();
	const std::string& title = utils::utf8_to_locale(item.title());
	const time_t pubDate = item.pubDate_timestamp();

	std::string dlformat = cfg->get_configvalue("download-path");
	if (dlformat[dlformat.length() - 1] != NEWSBEUTER_PATH_SEP) {
		dlformat.push_back(NEWSBEUTER_PATH_SEP);
	}

	const std::string filemask = cfg->get_configvalue("download-filename-format");
	dlformat.append(filemask);

	const std::string base = utils::get_basename(url);
	std::string extension;
	const std::size_t pos = base.rfind('.');
	if (pos != std::string::npos) {
		extension.append(base.substr(pos + 1));
	}

	FmtStrFormatter fmt;
	fmt.register_fmt('n', utils::replace_all(feed.title(), "/", "_"));
	fmt.register_fmt('h', get_hostname_from_url(url));
	fmt.register_fmt('u', base);
	fmt.register_fmt('F', utils::mt_strf_localtime("%F", pubDate));
	fmt.register_fmt('m', utils::mt_strf_localtime("%m", pubDate));
	fmt.register_fmt('b', utils::mt_strf_localtime("%b", pubDate));
	fmt.register_fmt('d', utils::mt_strf_localtime("%d", pubDate));
	fmt.register_fmt('H', utils::mt_strf_localtime("%H", pubDate));
	fmt.register_fmt('M', utils::mt_strf_localtime("%M", pubDate));
	fmt.register_fmt('S', utils::mt_strf_localtime("%S", pubDate));
	fmt.register_fmt('y', utils::mt_strf_localtime("%y", pubDate));
	fmt.register_fmt('Y', utils::mt_strf_localtime("%Y", pubDate));
	fmt.register_fmt('t', utils::replace_all(title, "/", "_"));
	fmt.register_fmt('e', utils::replace_all(extension, "/", "_"));

	if (feed.rssurl() != item.feedurl() &&
		item.get_feedptr() != nullptr) {
		std::string feedtitle = item.get_feedptr()->title();
		utils::remove_soft_hyphens(feedtitle);
		fmt.register_fmt('N', utils::replace_all(feedtitle, "/", "_"));
	} else {
		fmt.register_fmt('N', utils::replace_all(feed.title(), "/", "_"));
	}

	const std::string dlpath = fmt.do_format(dlformat);
	return dlpath;
}

EnqueueResult QueueManager::autoenqueue(RssFeed& feed)
{
	std::lock_guard<std::mutex> lock(feed.item_mutex);
	for (const auto& item : feed.items()) {
		if (item->enqueued() || item->enclosure_url().empty()) {
			continue;
		}

		const auto enclosure_type = item->enclosure_type();
		const auto enclosure_url = item->enclosure_url();

		if (!enclosure_type.empty() && !utils::is_valid_podcast_type(enclosure_type)) {
			LOG(Level::DEBUG, "QueueManager::autoenqueue: Skipping enclosure with url `%s'"
				" because of invalid podcast type `%s'", enclosure_url, enclosure_type);
			continue;
		}

		LOG(Level::DEBUG,
			"QueueManager::autoenqueue: enclosure_url = `%s' enclosure_type = `%s'",
			enclosure_url,
			enclosure_type);
		if (utils::is_http_url(item->enclosure_url())) {
			LOG(Level::INFO,
				"QueueManager::autoenqueue: enqueuing `%s'",
				item->enclosure_url());
			const auto result = enqueue_url(*item, feed);
			switch (result.status) {
			case EnqueueStatus::QUEUED_SUCCESSFULLY:
			case EnqueueStatus::URL_QUEUED_ALREADY:
				// Not an issue, continue processing rest of items
				break;
			case EnqueueStatus::QUEUE_FILE_OPEN_ERROR:
			case EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY:
				// Let caller of `autoenqueue` handle the issue
				return result;
			}
		}
	}

	return {EnqueueStatus::QUEUED_SUCCESSFULLY, ""};
}

} // namespace newsboat

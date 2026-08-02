#include "../include/cacheurlreader.h"

namespace newsboat {

CacheUrlReader::CacheUrlReader(FileUrlReader file_reader,
	RemoteApi* api) : file_reader(file_reader), api(api)
{
}

CacheUrlReader::~CacheUrlReader() {}

std::optional<utils::ReadTextFileError> CacheUrlReader::reload()
{
	urls.clear();
	tags.clear();

	auto file_res = file_reader.reload();
	if (file_res.has_value()) {
		return file_res;
	}

	auto file_urls = file_reader.get_urls();
	auto cached_urls = api->get_subscribed_urls();

	urls = file_urls;

	for (auto url : cached_urls) {
		urls.push_back({url.first, FeedOrigin{std::nullopt}});
	}

    return std::nullopt;
}

std::string CacheUrlReader::get_source() const
{
	return "Cache";
}

} // namespace newsboat
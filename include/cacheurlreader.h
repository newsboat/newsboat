#ifndef NEWSBOAT_CACHEURLREADER_H_
#define NEWSBOAT_CACHEURLREADER_H_

#include <memory>

#include "urlreader.h"
#include "remoteapi.h"
#include "fileurlreader.h"
#include "cache.h"
#include "utils.h"

namespace newsboat {

class CacheUrlReader : public UrlReader {
public:
	CacheUrlReader(FileUrlReader file_reader, RemoteApi* api);
	~CacheUrlReader();
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	FileUrlReader file_reader;
	RemoteApi* api;
};

}

#endif /* NEWSBOAT_CACHEURLREADER_H_ */

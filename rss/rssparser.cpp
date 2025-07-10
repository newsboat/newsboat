#include "rssparser.h"

#include <cstring>
#include <ctime>
#include <libxml/tree.h>

#include "utils.h"

namespace rsspp {

std::string RssParser::w3cdtf_to_rfc822(const std::string& w3cdtf)
{
	if (w3cdtf.empty()) {
		return "";
	}

	struct tm stm;
	memset(&stm, 0, sizeof(stm));
	stm.tm_mday = 1;

	char* ptr = strptime(w3cdtf.c_str(), "%Y", &stm);

	if (ptr != nullptr) {
		ptr = strptime(ptr, "-%m", &stm);
	} else {
		return "";
	}

	if (ptr != nullptr) {
		ptr = strptime(ptr, "-%d", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, "T%H", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, ":%M", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, ":%S", &stm);
	}

	int offs = 0;
	if (ptr != nullptr) {
		if (ptr[0] == '+' || ptr[0] == '-') {
			unsigned int hour, min;
			if (sscanf(ptr + 1, "%02u:%02u", &hour, &min) == 2) {
				offs = 60 * 60 * hour + 60 * min;
				if (ptr[0] == '+') {
					offs = -offs;
				}
				stm.tm_gmtoff = offs;
			}
		} else if (ptr[0] == 'Z') {
			stm.tm_gmtoff = 0;
		}
	}

	// tm_isdst will force mktime to consider DST, like localtime(), but
	// then the offset will be zeroed out, since that was manually added
	// https://github.com/akrennmair/newsbeuter/issues/369
	stm.tm_isdst = -1;

	const time_t local_time = mktime(&stm);
	if (local_time == -1) {
		return "";
	}

	const time_t gmttime = local_time + offs;
	return Newsboat::utils::mt_strf_localtime("%a, %d %b %Y %H:%M:%S +0000",
			gmttime);
}

} // namespace rsspp

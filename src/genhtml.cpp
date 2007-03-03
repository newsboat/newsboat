#include <iostream>
#include <fstream>
#include <utils.h>
#include <htmltmpl.h>

namespace newsbeuter {

void utils::planet_generate_html(std::vector<rss_feed>& feeds, std::vector<rss_item>& items, const std::string& tmplfile, const std::string& outfile ) {
	planet::htmltmpl tmpl = planet::htmltmpl::parse_file(tmplfile);
	std::fstream f;
	f.open(outfile.c_str(), std::fstream::out);
	if (f.is_open()) {
		planet::varmap variables;
		f << tmpl.to_html(feeds, items, variables);
		f.close();
	}
}


}

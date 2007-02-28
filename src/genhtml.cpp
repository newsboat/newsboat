#include <iostream>
#include <fstream>
#include <utils.h>

namespace newsbeuter {

void utils::planet_generate_html(std::vector<rss_feed>& feeds, std::vector<rss_item>& items, const std::string& outfile ) {
	std::fstream f;
	f.open(outfile.c_str(), std::fstream::out);
	if (f.is_open()) {
		f << "<html><title>planet</title><body>" << std::endl;

		f << "<h1>Feeds</h1>" << std::endl << "<ul>" << std::endl;
		for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
			f << "<li><a href=\"" << it->link() << "\">" << it->title() << "</a> (<a href=\"" << it->rssurl() << "\">rss</a>)</li>" << std::endl;
		}
		f << "</ul>" << std::endl;

		f << "<h1>Articles</h1>" << std::endl;

		for (std::vector<rss_item>::iterator it=items.begin();it!=items.end();++it) {
			f << "<h2><a href=\"" << it->link() << "\">" << it->title() << "</a></h2>" << std::endl;
			f << "Written by: " << it->author() << "<br />Date: " << it->pubDate() << "<br />" << std::endl;
			f << "<p>" << it->description() << "</p>" << std::endl;
			f << "<hr />" << std::endl;
		}

		f << "</body></html>" << std::endl;
	}
}


}

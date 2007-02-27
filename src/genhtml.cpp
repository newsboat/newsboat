#include <iostream>
#include <utils.h>

namespace newsbeuter {

void utils::planet_generate_html(std::vector<rss_feed>& feeds, std::vector<rss_item>& items) {
	std::cout << "<html><title>planet</title><body>" << std::endl;

	std::cout << "<h1>Feeds</h1>" << std::endl << "<ul>" << std::endl;
	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
		std::cout << "<li><a href=\"" << it->link() << "\">" << it->title() << "</a> (<a href=\"" << it->rssurl() << "\">rss</a>)</li>" << std::endl;
	}
	std::cout << "</ul>" << std::endl;

	std::cout << "<h1>Articles</h1>" << std::endl;

	for (std::vector<rss_item>::iterator it=items.begin();it!=items.end();++it) {
		std::cout << "<h2><a href=\"" << it->link() << "\">" << it->title() << "</a></h2>" << std::endl;
		std::cout << "Written by: " << it->author() << "<br />Date: " << it->pubDate() << "<br />" << std::endl;
		std::cout << "<p>" << it->description() << "</p>" << std::endl;
		std::cout << "<hr />" << std::endl;
	}

	std::cout << "</body></html>" << std::endl;
}


}

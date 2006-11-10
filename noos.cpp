#include <iostream>

#include <rss.h>

using namespace noos;

int main(int argc, char * argv[]) {

	if (argc < 2) {
		std::cout << "usage: " << argv[0] << " <URL>" << std::endl;
		return 1;
	}

	rss_parser parser(argv[1]);

	parser.parse();

	rss_feed& feed = parser.get_feed();

	std::cout << "Title: " << feed.title() << std::endl << "Description: " << feed.description() << std::endl;
	std::cout << "Link: " << feed.link() << std::endl << std::endl;

	std::vector<rss_item>& items = feed.items();

	for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it) {
		std::cout << "  * Title: " << it->title() << std::endl;
		std::cout << "  * Link: " << it->link() << std::endl;
		std::cout << "  * Description: " << it->description() << std::endl << std::endl;
	}

	return 0;
}

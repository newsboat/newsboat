#include <iostream>

#include <rss.h>

#include "stfl/test.h"

using namespace noos;

class my_handler : public rss_handler {
	public:
		my_handler() { }
		virtual void handle(const raptor_statement * st) {
			std::cout << "in handle" << std::endl;

		}

};

int main(int argc, char * argv[]) {

	raptor_init();

	rss_uri uri("rss.xml");
	rss_parser parser(uri);
	parser.set_handler(new my_handler());

	parser.parse();

	raptor_finish();

	return 0;
}

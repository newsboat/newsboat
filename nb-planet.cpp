#include <planet.h>
#include <locale.h>
#include <config.h>
#include <cerrno>

#include <iostream>

using namespace nbplanet;

int main(int argc, char * argv[]) {
	if (!setlocale(LC_CTYPE,"") || !setlocale(LC_MESSAGES,"")) {
		std::cerr << "setlocale failed: " << strerror(errno) << std::endl;
		return 1;	
	}
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
	
	planet p;
	p.run(argc,argv);

	return 0;
}

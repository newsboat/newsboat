#include <xmlpullparser.h>
#include <fstream>
#include <iostream>

using namespace std;
using namespace noos;

#ifdef _TESTPP
int main(int argc, char * argv[]) {
	if (argc < 2)
		return 1;
	xmlpullparser xpp;
	fstream f(argv[1]);
	xpp.setInput(f);

	for (xmlpullparser::event e = xpp.next(); e != xmlpullparser::END_DOCUMENT; e = xpp.next()) {
		switch (e) {
			case xmlpullparser::START_TAG:
				cout << "start tag: " << xpp.getText() << endl;
				for (unsigned int i=0;i<xpp.getAttributeCount();++i) {
					cout << "  attribute: " << xpp.getAttributeName(i) << " = " << xpp.getAttributeValue(i) << endl;
				}
				break;
			case xmlpullparser::TEXT:
				cout << "text: " << xpp.getText() << endl;
				break;
			case xmlpullparser::END_TAG:
				cout << "end tag: " << xpp.getText() << endl;
				break;
		}
	}

	return 0;
}
#endif

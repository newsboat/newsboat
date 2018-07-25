#ifndef NEWSBOAT_RELOADER_H_
#define NEWSBOAT_RELOADER_H_

namespace newsboat {

class controller;

class Reloader {
	controller* ctrl;

public:
	Reloader(controller* c);
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADER_H_ */

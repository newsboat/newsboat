#ifndef PODBEUTER_VIEW__H
#define PODBEUTER_VIEW__H

namespace podbeuter {

class pb_controller;

class pb_view {
	public:
		// TODO: implement
		pb_view(pb_controller * c = 0);
		~pb_view();
		void run();
	private:
		pb_controller * ctrl;
};

}

#endif

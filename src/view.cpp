#include "feedlist.h"
#include "itemlist.h"
#include "itemview.h"

extern "C" {
#include <stfl.h>
}

#include <view.h>

using namespace noos;

view::view(controller * c) : ctrl(c) { 
	feedlist_form = stfl_create(feedlist_str);
	itemlist_form = stfl_create(itemlist_str);
	itemview_form = stfl_create(itemview_str);
}

view::~view() {
	stfl_reset();
	stfl_free(feedlist_form);
	stfl_free(itemlist_form);
	stfl_free(itemview_form);
}

void view::run_feedlist() {

}

void view::run_itemlist() {

}

void view::run_itemview() {

}

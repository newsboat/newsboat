%include "typemaps.i"
%include "std_string.i"
%module Newsbeuter
%{
#include <view.h>
#include <controller.h>
using namespace newsbeuter;
%}


%include "include/rss.h"
%include "include/view.h"
%include "include/controller.h"

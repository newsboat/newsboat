#ifndef NEWSBOAT_LISTWIDGET_H_
#define NEWSBOAT_LISTWIDGET_H_

#include "listmovementcontrol.h"
#include "listwidgetbackend.h"

namespace Newsboat {

using ListWidget = ListMovementControl<ListWidgetBackend>;

} // namespace Newsboat

#endif /* NEWSBOAT_LISTWIDGET_H_ */

#ifndef NEWSBOAT_LISTWIDGET_H_
#define NEWSBOAT_LISTWIDGET_H_

#include "listmovementcontrol.h"
#include "listwidgetbackend.h"
#include "newlistwidgetbackend.h"

namespace newsboat {

using OldListWidget = ListMovementControl<ListWidgetBackend>;
using NewListWidget = ListMovementControl<NewListWidgetBackend>;

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGET_H_ */

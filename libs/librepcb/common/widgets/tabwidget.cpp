/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "tabwidget.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TabWidget::TabWidget(QWidget* parent) noexcept : QTabWidget(parent) {
  tabBar()->installEventFilter(this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool TabWidget::eventFilter(QObject* o, QEvent* e) {
  // Handle middle mouse click on closable tabs
  bool isTabBar = o == tabBar();
  bool isMouseButtonPress = e->type() == QEvent::MouseButtonPress;
  bool tabsAreClosable = tabBar()->tabsClosable();
  if (isTabBar && isMouseButtonPress && tabsAreClosable) {
    auto me = static_cast<QMouseEvent*>(e);
    if (me->buttons() == Qt::MiddleButton) {
      auto tabIdx = tabBar()->tabAt(me->pos());
      if (tabIdx >= 0) {
        emit tabCloseRequested(tabIdx);
        return true;
      }
    }
  }

  return QTabWidget::eventFilter(o, e);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

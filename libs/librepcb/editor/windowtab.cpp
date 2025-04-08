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
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WindowTab::WindowTab(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), onUiDataChanged(*this), mApp(app) {
}

WindowTab::~WindowTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowTab::setUiData(const ui::TabData& data) noexcept {
  if (data.action != ui::Action::None) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    const auto a = data.action;
    QMetaObject::invokeMethod(
        this, [this, a]() { triggerAsync(a); }, Qt::QueuedConnection);
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void WindowTab::triggerAsync(ui::Action a) noexcept {
  switch (a) {
    case ui::Action::TabClose: {
      emit closeRequested();
      break;
    }
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

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
#include "hometab.h"

#include "utils/editortoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

HomeTab::HomeTab(GuiApplication& app, QObject* parent) noexcept
  : WindowTab(app, nullptr, -1, parent) {
}

HomeTab::~HomeTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData HomeTab::getBaseUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Home,  // Type
      slint::SharedString(),  // Title
      -1,  // Project index
      ui::RuleCheckState::NotAvailable,  // Rule check state
      nullptr,  // Rule check messages
      slint::SharedString(),  // Rule check execution error
      false,  // Can save
      false,  // Can export graphics
      false,  // Can undo
      slint::SharedString(),  // Undo text
      false,  // Can redo
      slint::SharedString(),  // Redo text
      false,  // Can cut
      false,  // Can copy
      false,  // Can paste
      false,  // Can remove
      false,  // Can rotate
      false,  // Can mirror
      false,  // Can move/align
      false,  // Can snap to grid
      false,  // Can reset texts
      false,  // Can lock placement
      false,  // Can edit properties
  };
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

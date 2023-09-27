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
#include "boardgui.h"

#include "openedproject.h"

#include <librepcb/core/project/board/board.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardGui::BoardGui(OpenedProject& project, Board& board) noexcept
  : QObject(&project), mProject(project), mBoard(&board) {
}

BoardGui::~BoardGui() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QString& BoardGui::getName() const noexcept {
  Q_ASSERT(mBoard);
  return *mBoard->getName();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb

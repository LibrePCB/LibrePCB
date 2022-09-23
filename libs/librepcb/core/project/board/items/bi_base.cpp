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
#include "bi_base.h"

#include "../../../graphics/graphicsscene.h"
#include "../../project.h"
#include "../board.h"
#include "../graphicsitems/bgi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Base::BI_Base(Board& board) noexcept
  : QObject(&board), mBoard(board), mIsAddedToBoard(false), mIsSelected(false) {
}

BI_Base::~BI_Base() noexcept {
  Q_ASSERT(!mIsAddedToBoard);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Project& BI_Base::getProject() const noexcept {
  return mBoard.getProject();
}

Circuit& BI_Base::getCircuit() const noexcept {
  return mBoard.getProject().getCircuit();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Base::setSelected(bool selected) noexcept {
  mIsSelected = selected;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Base::addToBoard(QGraphicsItem* item) noexcept {
  Q_ASSERT(!mIsAddedToBoard);
  if (item) {
    mBoard.getGraphicsScene().addItem(*item);
  }
  mIsAddedToBoard = true;
}

void BI_Base::removeFromBoard(QGraphicsItem* item) noexcept {
  Q_ASSERT(mIsAddedToBoard);
  if (item) {
    mBoard.getGraphicsScene().removeItem(*item);
  }
  mIsAddedToBoard = false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

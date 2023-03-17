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
#include "bi_hole.h"

#include "../board.h"
#include "../boarddesignrules.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Hole::BI_Hole(Board& board, const Hole& hole)
  : BI_Base(board),
    onEdited(*this),
    mHole(new Hole(hole)),
    mStopMaskOffset(),
    mOnEditedSlot(*this, &BI_Hole::holeEdited) {
  updateStopMaskOffset();

  mHole->onEdited.attach(mOnEditedSlot);
  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Hole::updateStopMaskOffset);
}

BI_Hole::~BI_Hole() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& BI_Hole::getUuid() const noexcept {
  return mHole->getUuid();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Hole::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
}

void BI_Hole::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Hole::holeEdited(const Hole& hole, Hole::Event event) noexcept {
  Q_UNUSED(hole);

  switch (event) {
    case Hole::Event::UuidChanged:
      break;
    case Hole::Event::PathChanged:
      onEdited.notify(Event::PathChanged);
      updateStopMaskOffset();
      break;
    case Hole::Event::DiameterChanged:
      onEdited.notify(Event::DiameterChanged);
      updateStopMaskOffset();
      break;
    case Hole::Event::StopMaskConfigChanged:
      updateStopMaskOffset();
      break;
    default:
      qWarning() << "Unhandled switch-case in BI_Hole::holeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BI_Hole::updateStopMaskOffset() noexcept {
  tl::optional<Length> offset;
  if (!mHole->getStopMaskConfig().isEnabled()) {
    offset = tl::nullopt;
  } else if (auto manualOffset = mHole->getStopMaskConfig().getOffset()) {
    offset = *manualOffset;
  } else {
    offset = *mBoard.getDesignRules().getStopMaskClearance().calcValue(
        *mHole->getDiameter());
  }

  if (offset != mStopMaskOffset) {
    mStopMaskOffset = offset;
    onEdited.notify(Event::StopMaskOffsetChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

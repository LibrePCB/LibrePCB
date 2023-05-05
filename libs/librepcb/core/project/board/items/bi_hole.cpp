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

BI_Hole::BI_Hole(Board& board, const BoardHoleData& data)
  : BI_Base(board), onEdited(*this), mData(data), mStopMaskOffset() {
  updateStopMaskOffset();

  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Hole::updateStopMaskOffset);
}

BI_Hole::~BI_Hole() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BI_Hole::setDiameter(const PositiveLength& diameter) noexcept {
  if (mData.setDiameter(diameter)) {
    onEdited.notify(Event::DiameterChanged);
    updateStopMaskOffset();
    mBoard.invalidatePlanes();
    return true;
  } else {
    return false;
  }
}

bool BI_Hole::setPath(const NonEmptyPath& path) noexcept {
  if (mData.setPath(path)) {
    onEdited.notify(Event::PathChanged);
    mBoard.invalidatePlanes();
    return true;
  } else {
    return false;
  }
}

bool BI_Hole::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (mData.setStopMaskConfig(config)) {
    updateStopMaskOffset();
    return true;
  } else {
    return false;
  }
}

bool BI_Hole::setLocked(bool locked) noexcept {
  if (mData.setLocked(locked)) {
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Hole::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  mBoard.invalidatePlanes();
}

void BI_Hole::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  mBoard.invalidatePlanes();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Hole::updateStopMaskOffset() noexcept {
  tl::optional<Length> offset;
  if (!mData.getStopMaskConfig().isEnabled()) {
    offset = tl::nullopt;
  } else if (auto manualOffset = mData.getStopMaskConfig().getOffset()) {
    offset = *manualOffset;
  } else {
    offset = *mBoard.getDesignRules().getStopMaskClearance().calcValue(
        *mData.getDiameter());
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

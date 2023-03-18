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

#include "../../../graphics/graphicsscene.h"
#include "../../../graphics/holegraphicsitem.h"
#include "../../project.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "../boardlayerstack.h"

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
    mHole(new Hole(hole)),
    mGraphicsItem(new HoleGraphicsItem(*mHole, mBoard.getLayerStack(), true)),
    mOnEditedSlot(*this, &BI_Hole::holeEdited) {
  // Update automatic stop mask offset if design rules were modified.
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_Hole::updateAutoStopMaskOffset);
  updateAutoStopMaskOffset();

  // Register to the hole to get attribute updates.
  mHole->onEdited.attach(mOnEditedSlot);
}

BI_Hole::~BI_Hole() noexcept {
  mGraphicsItem.reset();
  mHole.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

tl::optional<Length> BI_Hole::getStopMaskOffset() const noexcept {
  if (!mHole->getStopMaskConfig().isEnabled()) {
    return tl::nullopt;
  } else if (const tl::optional<Length>& offset =
                 mHole->getStopMaskConfig().getOffset()) {
    return *offset;
  } else {
    return *getAutoStopMaskOffset();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Hole::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard(mGraphicsItem.data());
}

void BI_Hole::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard(mGraphicsItem.data());
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_Hole::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

const Uuid& BI_Hole::getUuid() const noexcept {
  return mHole->getUuid();
}

bool BI_Hole::isSelectable() const noexcept {
  const GraphicsLayer* layer =
      mBoard.getLayerStack().getLayer(GraphicsLayer::sBoardDrillsNpth);
  return layer && layer->isVisible();
}

void BI_Hole::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->setSelected(selected);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Hole::holeEdited(const Hole& hole, Hole::Event event) noexcept {
  Q_UNUSED(hole);

  switch (event) {
    case Hole::Event::DiameterChanged:
      updateAutoStopMaskOffset();
      break;
    default:
      break;
  }
}

void BI_Hole::updateAutoStopMaskOffset() noexcept {
  mGraphicsItem->setAutoStopMaskOffset(*getAutoStopMaskOffset());
}

UnsignedLength BI_Hole::getAutoStopMaskOffset() const noexcept {
  return mBoard.getDesignRules().getStopMaskClearance().calcValue(
      *mHole->getDiameter());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

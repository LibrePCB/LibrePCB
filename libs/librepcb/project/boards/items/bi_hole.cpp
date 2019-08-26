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

#include "../../project.h"
#include "../board.h"
#include "../boardlayerstack.h"

#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/holegraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Hole::BI_Hole(Board& board, const BI_Hole& other) : BI_Base(board) {
  mHole.reset(new Hole(Uuid::createRandom(), *other.mHole));
  init();
}

BI_Hole::BI_Hole(Board& board, const SExpression& node,
                 const Version& projectVersion)
  : BI_Base(board) {
  mHole.reset(new Hole(node, projectVersion));
  init();
}

BI_Hole::BI_Hole(Board& board, const Hole& hole) : BI_Base(board) {
  mHole.reset(new Hole(hole));
  init();
}

void BI_Hole::init() {
  mGraphicsItem.reset(new HoleGraphicsItem(*mHole, mBoard.getLayerStack()));
}

BI_Hole::~BI_Hole() noexcept {
  mGraphicsItem.reset();
  mHole.reset();
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

void BI_Hole::serialize(SExpression& root) const {
  mHole->serialize(root);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

const Point& BI_Hole::getPosition() const noexcept {
  return mHole->getPosition();
}

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
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

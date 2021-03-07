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
#include "si_netlabel.h"

#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_netsegment.h"

#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetLabel::SI_NetLabel(SI_NetSegment& segment, const SExpression& node,
                         const Version& fileFormat)
  : SI_Base(segment.getSchematic()),
    mNetSegment(segment),
    mNetLabel(node, fileFormat) {
  init();
}

SI_NetLabel::SI_NetLabel(SI_NetSegment& segment, const Point& position,
                         const Angle& rotation, const Alignment& alignment)
  : SI_Base(segment.getSchematic()),
    mNetSegment(segment),
    mNetLabel(Uuid::createRandom(), position, rotation, alignment) {
  init();
}

void SI_NetLabel::init() {
  // create the graphics item
  mGraphicsItem.reset(new SGI_NetLabel(*this));
  mGraphicsItem->setPos(mNetLabel.getPosition().toPxQPointF());
  mGraphicsItem->setRotation(-mNetLabel.getRotation().toDeg());
}

SI_NetLabel::~SI_NetLabel() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

NetSignal& SI_NetLabel::getNetSignalOfNetSegment() const noexcept {
  return mNetSegment.getNetSignal();
}

Length SI_NetLabel::getApproximateWidth() noexcept {
  return Length::fromPx(mGraphicsItem->boundingRect().right());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetLabel::setPosition(const Point& position) noexcept {
  if (mNetLabel.setPosition(position)) {
    mGraphicsItem->setPos(position.toPxQPointF());
    updateAnchor();
  }
}

void SI_NetLabel::setRotation(const Angle& rotation) noexcept {
  if (mNetLabel.setRotation(rotation)) {
    mGraphicsItem->setRotation(-rotation.toDeg());
    mGraphicsItem->updateCacheAndRepaint();
    updateAnchor();
  }
}

void SI_NetLabel::setAlignment(const Alignment& alignment) noexcept {
  if (mNetLabel.setAlignment(alignment)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetLabel::updateAnchor() noexcept {
  mGraphicsItem->setAnchor(
      mNetSegment.calcNearestPoint(mNetLabel.getPosition()));
}

void SI_NetLabel::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNameChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::nameChanged,
              [this]() { mGraphicsItem->updateCacheAndRepaint(); });
  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  SI_Base::addToSchematic(mGraphicsItem.data());
  mGraphicsItem->updateCacheAndRepaint();
  updateAnchor();
}

void SI_NetLabel::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mNameChangedConnection);
  disconnect(mHighlightChangedConnection);
  SI_Base::removeFromSchematic(mGraphicsItem.data());
}

void SI_NetLabel::serialize(SExpression& root) const {
  mNetLabel.serialize(root);
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_NetLabel::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_NetLabel::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

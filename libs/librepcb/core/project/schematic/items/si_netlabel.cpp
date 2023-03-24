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

#include "../../../utils/scopeguard.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetLabel::SI_NetLabel(SI_NetSegment& segment, const NetLabel& label)
  : SI_Base(segment.getSchematic()),
    onEdited(*this),
    mNetSegment(segment),
    mNetLabel(label),
    mAnchorPosition() {
  updateAnchor();
}

SI_NetLabel::~SI_NetLabel() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

NetSignal& SI_NetLabel::getNetSignalOfNetSegment() const noexcept {
  return mNetSegment.getNetSignal();
}

Length SI_NetLabel::getApproximateWidth() noexcept {
  return Length();  // TODO
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetLabel::setPosition(const Point& position) noexcept {
  if (mNetLabel.setPosition(position)) {
    onEdited.notify(Event::PositionChanged);
    updateAnchor();
  }
}

void SI_NetLabel::setRotation(const Angle& rotation) noexcept {
  if (mNetLabel.setRotation(rotation)) {
    onEdited.notify(Event::RotationChanged);
  }
}

void SI_NetLabel::setMirrored(const bool mirrored) noexcept {
  if (mNetLabel.setMirrored(mirrored)) {
    onEdited.notify(Event::MirroredChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetLabel::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNameChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::nameChanged,
              [this]() { onEdited.notify(Event::NetNameChanged); });
  SI_Base::addToSchematic();
}

void SI_NetLabel::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mNameChangedConnection);
  SI_Base::removeFromSchematic();
}

void SI_NetLabel::updateAnchor() noexcept {
  const Point p = mNetSegment.calcNearestPoint(mNetLabel.getPosition());
  if (p != mAnchorPosition) {
    mAnchorPosition = p;
    onEdited.notify(Event::AnchorPositionChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

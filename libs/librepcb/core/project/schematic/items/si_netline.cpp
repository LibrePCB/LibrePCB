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
#include "si_netline.h"

#include "../../../graphics/graphicsscene.h"
#include "../../../utils/scopeguard.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_netpoint.h"
#include "si_netsegment.h"
#include "si_symbol.h"
#include "si_symbolpin.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetLine::SI_NetLine(SI_NetSegment& segment, const Uuid& uuid,
                       SI_NetLineAnchor& startPoint, SI_NetLineAnchor& endPoint,
                       const UnsignedLength& width)
  : SI_Base(segment.getSchematic()),
    mNetSegment(segment),
    mNetLine(uuid, width, startPoint.toNetLineAnchor(),
             endPoint.toNetLineAnchor()),
    mStartPoint(&startPoint),
    mEndPoint(&endPoint) {
  // check if both netpoints are different
  if (mStartPoint == mEndPoint) {
    throw LogicError(__FILE__, __LINE__,
                     "SI_NetLine: both endpoints are the same.");
  }

  mGraphicsItem.reset(new SGI_NetLine(*this));
  updateLine();
}

SI_NetLine::~SI_NetLine() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

SI_NetLineAnchor* SI_NetLine::getOtherPoint(
    const SI_NetLineAnchor& firstPoint) const noexcept {
  if (&firstPoint == mStartPoint) {
    return mEndPoint;
  } else if (&firstPoint == mEndPoint) {
    return mStartPoint;
  } else {
    return nullptr;
  }
}

NetSignal& SI_NetLine::getNetSignalOfNetSegment() const noexcept {
  return getNetSegment().getNetSignal();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetLine::setWidth(const UnsignedLength& width) noexcept {
  if (mNetLine.setWidth(width)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetLine::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mStartPoint->registerNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mStartPoint->unregisterNetLine(*this); });
  mEndPoint->registerNetLine(*this);  // can throw

  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  SI_Base::addToSchematic(mGraphicsItem.data());
  sg.dismiss();
}

void SI_NetLine::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mEndPoint->unregisterNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mEndPoint->registerNetLine(*this); });
  mStartPoint->unregisterNetLine(*this);  // can throw

  disconnect(mHighlightChangedConnection);
  SI_Base::removeFromSchematic(mGraphicsItem.data());
  sg.dismiss();
}

void SI_NetLine::updateLine() noexcept {
  mGraphicsItem->updateCacheAndRepaint();
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_NetLine::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape();
}

void SI_NetLine::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

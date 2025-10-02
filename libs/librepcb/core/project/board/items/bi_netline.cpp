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
#include "bi_netline.h"

#include "../../../types/layer.h"
#include "../../../utils/scopeguard.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "bi_device.h"
#include "bi_netpoint.h"
#include "bi_netsegment.h"
#include "bi_pad.h"
#include "bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BI_NetLineAnchor
 ******************************************************************************/

std::vector<PositiveLength> BI_NetLineAnchor::getLineWidths() const noexcept {
  std::vector<PositiveLength> widths;
  foreach (const BI_NetLine* line, getNetLines()) {
    widths.emplace_back(line->getWidth());
  }
  return widths;
}

std::optional<PositiveLength> BI_NetLineAnchor::getMaxLineWidth()
    const noexcept {
  std::optional<PositiveLength> w;
  foreach (const BI_NetLine* line, getNetLines()) {
    if ((!w) || (line->getWidth() > *w)) {
      w = line->getWidth();
    }
  }
  return w;
}

std::optional<PositiveLength> BI_NetLineAnchor::getMedianLineWidth()
    const noexcept {
  std::vector<PositiveLength> widths = getLineWidths();
  if (!widths.empty()) {
    std::sort(widths.begin(), widths.end());
    return widths[widths.size() / 2];
  } else {
    return std::nullopt;
  }
}

BI_NetSegment* BI_NetLineAnchor::getNetSegmentOfLines() const noexcept {
  auto it = getNetLines().constBegin();
  if (it != getNetLines().constEnd()) {
    return &(*it)->getNetSegment();
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const Uuid& uuid,
                       BI_NetLineAnchor& a, BI_NetLineAnchor& b,
                       const Layer& layer, const PositiveLength& width)
  : BI_Base(segment.getBoard()),
    onEdited(*this),
    mNetSegment(segment),
    mTrace(uuid, layer, width, a.toTraceAnchor(), b.toTraceAnchor()),
    mP1(&a),
    mP2(&b) {
  // Sort anchors to get a canonical file format.
  if (mP2->toTraceAnchor() < mP1->toTraceAnchor()) {
    std::swap(mP1, mP2);
  }

  // check layer
  if (!mTrace.getLayer().isCopper()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The layer of netpoint \"%1\" is invalid (%2).")
                           .arg(mTrace.getUuid().toStr())
                           .arg(mTrace.getLayer().getNameTr()));
  }

  // check if both netpoints are different
  if (mP1 == mP2) {
    throw LogicError(__FILE__, __LINE__,
                     "BI_NetLine: both endpoints are the same.");
  }
}

BI_NetLine::~BI_NetLine() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

BI_NetLineAnchor* BI_NetLine::getOtherPoint(
    const BI_NetLineAnchor& firstPoint) const noexcept {
  if (&firstPoint == mP1) {
    return mP2;
  } else if (&firstPoint == mP2) {
    return mP1;
  } else {
    return nullptr;
  }
}

Path BI_NetLine::getSceneOutline(const Length& expansion) const noexcept {
  Length width = getWidth() + (expansion * 2);
  if (width > 0) {
    return Path::obround(mP1->getPosition(), mP2->getPosition(),
                         PositiveLength(width));
  } else {
    return Path();
  }
}

UnsignedLength BI_NetLine::getLength() const noexcept {
  return (mP2->getPosition() - mP1->getPosition()).getLength();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetLine::setLayer(const Layer& layer) {
  if (layer == mTrace.getLayer()) {
    return;
  }
  if (isAddedToBoard() || (!layer.isCopper())) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mTrace.setLayer(layer)) {
    onEdited.notify(Event::LayerChanged);
  }
}

void BI_NetLine::setWidth(const PositiveLength& width) noexcept {
  if (mTrace.setWidth(width)) {
    onEdited.notify(Event::WidthChanged);
    mBoard.invalidatePlanes(&mTrace.getLayer());
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_NetLine::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP1->registerNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP1->unregisterNetLine(*this); });
  mP2->registerNetLine(*this);  // can throw

  BI_Base::addToBoard();
  sg.dismiss();

  mBoard.invalidatePlanes(&mTrace.getLayer());

  if (const NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mNetSignalNameChangedConnection =
        connect(netsignal, &NetSignal::nameChanged, this,
                [this]() { onEdited.notify(Event::NetSignalNameChanged); });
  }
}

void BI_NetLine::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mP1->unregisterNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mP2->registerNetLine(*this); });
  mP2->unregisterNetLine(*this);  // can throw

  BI_Base::removeFromBoard();
  sg.dismiss();

  mBoard.invalidatePlanes(&mTrace.getLayer());

  if (mNetSignalNameChangedConnection) {
    disconnect(mNetSignalNameChangedConnection);
    mNetSignalNameChangedConnection = QMetaObject::Connection();
  }
}

void BI_NetLine::updatePositions() noexcept {
  onEdited.notify(Event::PositionsChanged);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

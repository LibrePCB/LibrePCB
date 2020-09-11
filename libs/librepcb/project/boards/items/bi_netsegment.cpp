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
#include "bi_netsegment.h"

#include "../../circuit/circuit.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "bi_device.h"
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include "bi_netline.h"
#include "bi_netpoint.h"
#include "bi_via.h"

#include <librepcb/common/scopeguardlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetSegment::BI_NetSegment(Board& board, const BI_NetSegment& other,
                             const QHash<const BI_Device*, BI_Device*>& devMap)
  : BI_Base(board),
    mUuid(Uuid::createRandom()),
    mNetSignal(&other.getNetSignal()) {
  // determine new pad anchors
  QHash<const BI_NetLineAnchor*, BI_NetLineAnchor*> anchorsMap;
  for (auto it = devMap.begin(); it != devMap.end(); ++it) {
    BI_Footprint& oldFp = it.key()->getFootprint();
    BI_Footprint& newFp = it.value()->getFootprint();
    foreach (const BI_FootprintPad* pad, oldFp.getPads()) {
      anchorsMap.insert(pad, newFp.getPad(pad->getLibPadUuid()));
    }
  }

  // copy vias
  foreach (const BI_Via* via, other.mVias) {
    BI_Via* copy = new BI_Via(*this, *via);
    Q_ASSERT(!getViaByUuid(copy->getUuid()));
    mVias.append(copy);
    anchorsMap.insert(via, copy);
  }
  // copy netpoints
  foreach (const BI_NetPoint* netpoint, other.mNetPoints) {
    BI_NetPoint* copy = new BI_NetPoint(*this, netpoint->getPosition());
    mNetPoints.append(copy);
    anchorsMap.insert(netpoint, copy);
  }
  // copy netlines
  foreach (const BI_NetLine* netline, other.mNetLines) {
    BI_NetLineAnchor* start = anchorsMap.value(&netline->getStartPoint());
    Q_ASSERT(start);
    BI_NetLineAnchor* end = anchorsMap.value(&netline->getEndPoint());
    Q_ASSERT(end);
    BI_NetLine* copy = new BI_NetLine(*this, *netline, *start, *end);
    mNetLines.append(copy);
  }
}

BI_NetSegment::BI_NetSegment(Board& board, const SExpression& node)
  : BI_Base(board),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mNetSignal(nullptr) {
  try {
    Uuid netSignalUuid = node.getValueByPath<Uuid>("net");
    mNetSignal =
        mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if (!mNetSignal) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid net signal UUID: \"%1\"")
                             .arg(netSignalUuid.toStr()));
    }

    // Load all vias
    foreach (const SExpression& node, node.getChildren("via")) {
      BI_Via* via = new BI_Via(*this, node);
      if (getViaByUuid(via->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a via with the UUID \"%1\"!")
                .arg(via->getUuid().toStr()));
      }
      mVias.append(via);
    }

    // Load all netpoints
    foreach (const SExpression& child, node.getChildren("junction")) {
      BI_NetPoint* netpoint = new BI_NetPoint(*this, child);
      if (getNetPointByUuid(netpoint->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a netpoint with the UUID \"%1\"!")
                .arg(netpoint->getUuid().toStr()));
      }
      mNetPoints.append(netpoint);
    }

    // Load all netlines
    foreach (const SExpression& node,
             node.getChildren("netline") + node.getChildren("trace")) {
      BI_NetLine* netline = new BI_NetLine(*this, node);
      if (getNetLineByUuid(netline->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a netline with the UUID \"%1\"!")
                .arg(netline->getUuid().toStr()));
      }
      mNetLines.append(netline);
    }

    if (!areAllNetPointsConnectedTogether()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The netsegment with the UUID \"%1\" is not cohesive!")
              .arg(mUuid.toStr()));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    qDeleteAll(mNetLines);
    mNetLines.clear();
    qDeleteAll(mNetPoints);
    mNetPoints.clear();
    qDeleteAll(mVias);
    mVias.clear();
    throw;  // ...and rethrow the exception
  }
}

BI_NetSegment::BI_NetSegment(Board& board, NetSignal& signal)
  : BI_Base(board), mUuid(Uuid::createRandom()), mNetSignal(&signal) {
}

BI_NetSegment::~BI_NetSegment() noexcept {
  // delete all items
  qDeleteAll(mNetLines);
  mNetLines.clear();
  qDeleteAll(mNetPoints);
  mNetPoints.clear();
  qDeleteAll(mVias);
  mVias.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BI_NetSegment::isUsed() const noexcept {
  return ((!mVias.isEmpty()) || (!mNetPoints.isEmpty()) ||
          (!mNetLines.isEmpty()));
}

int BI_NetSegment::getViasAtScenePos(const Point&    pos,
                                     QList<BI_Via*>& vias) const noexcept {
  int count = 0;
  foreach (BI_Via* via, mVias) {
    if (via->isSelectable() &&
        via->getGrabAreaScenePx().contains(pos.toPxQPointF())) {
      vias.append(via);
      ++count;
    }
  }
  return count;
}

int BI_NetSegment::getNetPointsAtScenePos(const Point&         pos,
                                          const GraphicsLayer* layer,
                                          QList<BI_NetPoint*>& points) const
    noexcept {
  int count = 0;
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    if (netpoint->isSelectable() &&
        netpoint->getGrabAreaScenePx().contains(pos.toPxQPointF()) &&
        ((!layer) || (netpoint->getLayerOfLines() == layer))) {
      points.append(netpoint);
      ++count;
    }
  }
  return count;
}

int BI_NetSegment::getNetLinesAtScenePos(const Point&         pos,
                                         const GraphicsLayer* layer,
                                         QList<BI_NetLine*>&  lines) const
    noexcept {
  int count = 0;
  foreach (BI_NetLine* netline, mNetLines) {
    if (netline->isSelectable() &&
        netline->getGrabAreaScenePx().contains(pos.toPxQPointF()) &&
        ((!layer) || (&netline->getLayer() == layer))) {
      lines.append(netline);
      ++count;
    }
  }
  return count;
}

BI_NetPoint* BI_NetSegment::getNetPointNextToScenePos(
    const Point& pos, const GraphicsLayer* layer,
    UnsignedLength& maxDistance) const noexcept {
  BI_NetPoint* result = nullptr;
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    if (netpoint->isSelectable() &&
        ((!layer) || (netpoint->getLayerOfLines() == layer))) {
      UnsignedLength distance = (netpoint->getPosition() - pos).getLength();
      if (distance < maxDistance) {
        maxDistance = distance;
        result      = netpoint;
      }
    }
  }
  return result;
}

BI_Via* BI_NetSegment::getViaNextToScenePos(const Point&    pos,
                                            UnsignedLength& maxDistance) const
    noexcept {
  BI_Via* result = nullptr;
  foreach (BI_Via* via, mVias) {
    if (via->isSelectable()) {
      // NOTE(5n8ke): maxDistance is depending on the center of the via and
      // not the actual distance between the position and the edge of the via
      UnsignedLength distance = (via->getPosition() - pos).getLength();
      if (distance < maxDistance) {
        maxDistance = distance;
        result      = via;
      }
    }
  }
  return result;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetSegment::setNetSignal(NetSignal& netsignal) {
  if (&netsignal != mNetSignal) {
    if ((isUsed() && isAddedToBoard()) ||
        (netsignal.getCircuit() != getCircuit())) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      mNetSignal->unregisterBoardNetSegment(*this);  // can throw
      auto sg =
          scopeGuard([&]() { mNetSignal->registerBoardNetSegment(*this); });
      netsignal.registerBoardNetSegment(*this);  // can throw
      sg.dismiss();
    }
    mNetSignal = &netsignal;
  }
}

/*******************************************************************************
 *  Via Methods
 ******************************************************************************/

BI_Via* BI_NetSegment::getViaByUuid(const Uuid& uuid) const noexcept {
  foreach (BI_Via* via, mVias) {
    if (via->getUuid() == uuid) return via;
  }
  return nullptr;
}

/*******************************************************************************
 *  NetPoint Methods
 ******************************************************************************/

BI_NetPoint* BI_NetSegment::getNetPointByUuid(const Uuid& uuid) const noexcept {
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    if (netpoint->getUuid() == uuid) return netpoint;
  }
  return nullptr;
}

/*******************************************************************************
 *  NetLine Methods
 ******************************************************************************/

BI_NetLine* BI_NetSegment::getNetLineByUuid(const Uuid& uuid) const noexcept {
  foreach (BI_NetLine* netline, mNetLines) {
    if (netline->getUuid() == uuid) return netline;
  }
  return nullptr;
}

/*******************************************************************************
 *  NetPoint+NetLine Methods
 ******************************************************************************/

void BI_NetSegment::addElements(const QList<BI_Via*>&      vias,
                                const QList<BI_NetPoint*>& netpoints,
                                const QList<BI_NetLine*>&  netlines) {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (BI_Via* via, vias) {
    if ((mVias.contains(via)) || (&via->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no via with the same uuid in the list
    if (getViaByUuid(via->getUuid())) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("There is already a via with the UUID \"%1\"!")
                             .arg(via->getUuid().toStr()));
    }
    // add to board
    via->addToBoard();  // can throw
    mVias.append(via);
    sgl.add([this, via]() {
      via->removeFromBoard();
      mVias.removeOne(via);
    });
  }
  foreach (BI_NetPoint* netpoint, netpoints) {
    if ((mNetPoints.contains(netpoint)) ||
        (&netpoint->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netpoint with the same uuid in the list
    if (getNetPointByUuid(netpoint->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a netpoint with the UUID \"%1\"!")
              .arg(netpoint->getUuid().toStr()));
    }
    // add to board
    netpoint->addToBoard();  // can throw
    mNetPoints.append(netpoint);
    sgl.add([this, netpoint]() {
      netpoint->removeFromBoard();
      mNetPoints.removeOne(netpoint);
    });
  }
  foreach (BI_NetLine* netline, netlines) {
    if ((mNetLines.contains(netline)) || (&netline->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netline with the same uuid in the list
    if (getNetLineByUuid(netline->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a netline with the UUID \"%1\"!")
              .arg(netline->getUuid().toStr()));
    }
    // add to board
    netline->addToBoard();  // can throw
    mNetLines.append(netline);
    sgl.add([this, netline]() {
      netline->removeFromBoard();
      mNetLines.removeOne(netline);
    });
  }

  if (!areAllNetPointsConnectedTogether()) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("The netsegment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  sgl.dismiss();
}

void BI_NetSegment::removeElements(const QList<BI_Via*>&      vias,
                                   const QList<BI_NetPoint*>& netpoints,
                                   const QList<BI_NetLine*>&  netlines) {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (BI_NetLine* netline, netlines) {
    if (!mNetLines.contains(netline)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    netline->removeFromBoard();  // can throw
    mNetLines.removeOne(netline);
    sgl.add([this, netline]() {
      netline->addToBoard();
      mNetLines.append(netline);
    });
  }
  foreach (BI_NetPoint* netpoint, netpoints) {
    if (!mNetPoints.contains(netpoint)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    netpoint->removeFromBoard();  // can throw
    mNetPoints.removeOne(netpoint);
    sgl.add([this, netpoint]() {
      netpoint->addToBoard();
      mNetPoints.append(netpoint);
    });
  }
  foreach (BI_Via* via, vias) {
    if (!mVias.contains(via)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // remove from board
    via->removeFromBoard();  // can throw
    mVias.removeOne(via);
    sgl.add([this, via]() {
      via->addToBoard();
      mVias.append(via);
    });
  }

  if (!areAllNetPointsConnectedTogether()) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("The netsegment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  sgl.dismiss();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_NetSegment::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mNetPoints.count() + mNetLines.count() + 1);
  mNetSignal->registerBoardNetSegment(*this);  // can throw
  sgl.add([&]() { mNetSignal->unregisterBoardNetSegment(*this); });
  foreach (BI_Via* via, mVias) {
    via->addToBoard();  // can throw
    sgl.add([via]() { via->removeFromBoard(); });
  }
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    netpoint->addToBoard();  // can throw
    sgl.add([netpoint]() { netpoint->removeFromBoard(); });
  }
  foreach (BI_NetLine* netline, mNetLines) {
    netline->addToBoard();  // can throw
    sgl.add([netline]() { netline->removeFromBoard(); });
  }

  BI_Base::addToBoard(nullptr);
  sgl.dismiss();
}

void BI_NetSegment::removeFromBoard() {
  if ((!isAddedToBoard())) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mNetPoints.count() + mNetLines.count() + 1);
  foreach (BI_NetLine* netline, mNetLines) {
    netline->removeFromBoard();  // can throw
    sgl.add([netline]() { netline->addToBoard(); });
  }
  foreach (BI_NetPoint* netpoint, mNetPoints) {
    netpoint->removeFromBoard();  // can throw
    sgl.add([netpoint]() { netpoint->addToBoard(); });
  }
  foreach (BI_Via* via, mVias) {
    via->removeFromBoard();  // can throw
    sgl.add([via]() { via->addToBoard(); });
  }
  mNetSignal->unregisterBoardNetSegment(*this);  // can throw
  sgl.add([&]() { mNetSignal->registerBoardNetSegment(*this); });

  BI_Base::removeFromBoard(nullptr);
  sgl.dismiss();
}

void BI_NetSegment::selectAll() noexcept {
  foreach (BI_Via* via, mVias)
    via->setSelected(via->isSelectable());
  foreach (BI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(netpoint->isSelectable());
  foreach (BI_NetLine* netline, mNetLines)
    netline->setSelected(netline->isSelectable());
}

void BI_NetSegment::setSelectionRect(const QRectF rectPx) noexcept {
  foreach (BI_Via* via, mVias)
    via->setSelected(via->isSelectable() &&
                     via->getGrabAreaScenePx().intersects(rectPx));
  foreach (BI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(netpoint->isSelectable() &&
                          netpoint->getGrabAreaScenePx().intersects(rectPx));
  foreach (BI_NetLine* netline, mNetLines)
    netline->setSelected(netline->isSelectable() &&
                         netline->getGrabAreaScenePx().intersects(rectPx));
}

void BI_NetSegment::clearSelection() const noexcept {
  foreach (BI_Via* via, mVias)
    via->setSelected(false);
  foreach (BI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(false);
  foreach (BI_NetLine* netline, mNetLines)
    netline->setSelected(false);
}

void BI_NetSegment::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.appendChild("net", mNetSignal->getUuid(), true);
  serializePointerContainerUuidSorted(root, mVias, "via");
  serializePointerContainerUuidSorted(root, mNetPoints, "junction");
  serializePointerContainerUuidSorted(root, mNetLines, "trace");
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath BI_NetSegment::getGrabAreaScenePx() const noexcept {
  return QPainterPath();
}

bool BI_NetSegment::isSelected() const noexcept {
  if (mNetLines.isEmpty()) return false;
  foreach (const BI_NetLine* netline, mNetLines) {
    if (!netline->isSelected()) return false;
  }
  return true;
}

void BI_NetSegment::setSelected(bool selected) noexcept {
  foreach (BI_Via* via, mVias)
    via->setSelected(selected);
  foreach (BI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(selected);
  foreach (BI_NetLine* netline, mNetLines)
    netline->setSelected(selected);
  BI_Base::setSelected(selected);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BI_NetSegment::checkAttributesValidity() const noexcept {
  if (mNetSignal == nullptr) return false;
  if (!areAllNetPointsConnectedTogether()) return false;
  return true;
}

bool BI_NetSegment::areAllNetPointsConnectedTogether() const noexcept {
  const BI_NetLineAnchor* p = nullptr;
  if (mVias.count() > 0) {
    p = mVias.first();
  } else if (mNetPoints.count() > 0) {
    p = mNetPoints.first();
  } else {
    return true;  // there are no vias or netpoints => must be "connected
                  // together" :)
  }
  Q_ASSERT(p);
  QSet<const BI_Via*>          vias;
  QSet<const BI_NetPoint*>     points;
  QSet<const BI_FootprintPad*> pads;
  findAllConnectedNetPoints(*p, vias, pads, points);
  return (vias.count() == mVias.count()) &&
         (points.count() == mNetPoints.count());
}

void BI_NetSegment::findAllConnectedNetPoints(
    const BI_NetLineAnchor& p, QSet<const BI_Via*>& vias,
    QSet<const BI_FootprintPad*>& pads, QSet<const BI_NetPoint*>& points) const
    noexcept {
  if (const BI_Via* via = dynamic_cast<const BI_Via*>(&p)) {
    if (vias.contains(via)) return;
    vias.insert(via);
    foreach (const BI_NetLine* netline, mNetLines) {
      if (&netline->getStartPoint() == via) {
        findAllConnectedNetPoints(netline->getEndPoint(), vias, pads, points);
      }
      if (&netline->getEndPoint() == via) {
        findAllConnectedNetPoints(netline->getStartPoint(), vias, pads, points);
      }
    }
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(&p)) {
    if (pads.contains(pad)) return;
    pads.insert(pad);
    foreach (const BI_NetLine* netline, mNetLines) {
      if (&netline->getStartPoint() == pad) {
        findAllConnectedNetPoints(netline->getEndPoint(), vias, pads, points);
      }
      if (&netline->getEndPoint() == pad) {
        findAllConnectedNetPoints(netline->getStartPoint(), vias, pads, points);
      }
    }
  } else if (const BI_NetPoint* np = dynamic_cast<const BI_NetPoint*>(&p)) {
    if (points.contains(np)) return;
    points.insert(np);
    foreach (const BI_NetLine* netline, mNetLines) {
      if (&netline->getStartPoint() == np) {
        findAllConnectedNetPoints(netline->getEndPoint(), vias, pads, points);
      }
      if (&netline->getEndPoint() == np) {
        findAllConnectedNetPoints(netline->getStartPoint(), vias, pads, points);
      }
    }
  } else {
    Q_ASSERT(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

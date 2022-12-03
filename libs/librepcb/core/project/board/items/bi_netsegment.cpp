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

#include "../../../utils/scopeguardlist.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "bi_device.h"
#include "bi_footprintpad.h"
#include "bi_netline.h"
#include "bi_netpoint.h"
#include "bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetSegment::BI_NetSegment(Board& board, const Uuid& uuid, NetSignal* signal)
  : BI_Base(board), mUuid(uuid), mNetSignal(signal) {
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

QString BI_NetSegment::getNetNameToDisplay(bool fallback) const noexcept {
  return mNetSignal ? *mNetSignal->getName()
                    : (fallback ? tr("(no net)") : QString());
}

bool BI_NetSegment::isUsed() const noexcept {
  return ((!mVias.isEmpty()) || (!mNetPoints.isEmpty()) ||
          (!mNetLines.isEmpty()));
}

int BI_NetSegment::getViasAtScenePos(const Point& pos,
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

int BI_NetSegment::getNetPointsAtScenePos(const Point& pos,
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

int BI_NetSegment::getNetLinesAtScenePos(const Point& pos,
                                         const GraphicsLayer* layer,
                                         QList<BI_NetLine*>& lines) const
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

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetSegment::setNetSignal(NetSignal* netsignal) {
  if (netsignal != mNetSignal) {
    if ((isUsed() && isAddedToBoard()) ||
        (netsignal && (netsignal->getCircuit() != getCircuit()))) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      ScopeGuardList sgl;
      if (mNetSignal) {
        mNetSignal->unregisterBoardNetSegment(*this);  // can throw
        sgl.add([&]() { mNetSignal->registerBoardNetSegment(*this); });
      }
      if (netsignal) {
        netsignal->registerBoardNetSegment(*this);  // can throw
        sgl.add([&]() { netsignal->unregisterBoardNetSegment(*this); });
      }
      sgl.dismiss();
    }
    mNetSignal = netsignal;
  }
}

/*******************************************************************************
 *  NetPoint+NetLine Methods
 ******************************************************************************/

void BI_NetSegment::addElements(const QList<BI_Via*>& vias,
                                const QList<BI_NetPoint*>& netpoints,
                                const QList<BI_NetLine*>& netlines) {
  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (BI_Via* via, vias) {
    if ((mVias.values().contains(via)) || (&via->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (mVias.contains(via->getUuid())) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("There is already a via with the UUID \"%1\"!")
                             .arg(via->getUuid().toStr()));
    }
    if (isAddedToBoard()) {
      via->addToBoard();  // can throw
    }
    mVias.insert(via->getUuid(), via);
    sgl.add([this, via]() {
      if (isAddedToBoard()) {
        via->removeFromBoard();
      }
      mVias.remove(via->getUuid());
    });
  }
  foreach (BI_NetPoint* netpoint, netpoints) {
    if ((mNetPoints.values().contains(netpoint)) ||
        (&netpoint->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (mNetPoints.contains(netpoint->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a netpoint with the UUID \"%1\"!")
              .arg(netpoint->getUuid().toStr()));
    }
    if (isAddedToBoard()) {
      netpoint->addToBoard();  // can throw
    }
    mNetPoints.insert(netpoint->getUuid(), netpoint);
    sgl.add([this, netpoint]() {
      if (isAddedToBoard()) {
        netpoint->removeFromBoard();
      }
      mNetPoints.remove(netpoint->getUuid());
    });
  }
  foreach (BI_NetLine* netline, netlines) {
    if ((mNetLines.values().contains(netline)) ||
        (&netline->getNetSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (mNetLines.contains(netline->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a netline with the UUID \"%1\"!")
              .arg(netline->getUuid().toStr()));
    }
    if (isAddedToBoard()) {
      netline->addToBoard();  // can throw
    }
    mNetLines.insert(netline->getUuid(), netline);
    sgl.add([this, netline]() {
      if (isAddedToBoard()) {
        netline->removeFromBoard();
      }
      mNetLines.remove(netline->getUuid());
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

void BI_NetSegment::removeElements(const QList<BI_Via*>& vias,
                                   const QList<BI_NetPoint*>& netpoints,
                                   const QList<BI_NetLine*>& netlines) {
  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (BI_NetLine* netline, netlines) {
    if (mNetLines.value(netline->getUuid()) != netline) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      netline->removeFromBoard();  // can throw
    }
    mNetLines.remove(netline->getUuid());
    sgl.add([this, netline]() {
      if (isAddedToBoard()) {
        netline->addToBoard();
      }
      mNetLines.insert(netline->getUuid(), netline);
    });
  }
  foreach (BI_NetPoint* netpoint, netpoints) {
    if (mNetPoints.value(netpoint->getUuid()) != netpoint) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      netpoint->removeFromBoard();  // can throw
    }
    mNetPoints.remove(netpoint->getUuid());
    sgl.add([this, netpoint]() {
      if (isAddedToBoard()) {
        netpoint->addToBoard();
      }
      mNetPoints.insert(netpoint->getUuid(), netpoint);
    });
  }
  foreach (BI_Via* via, vias) {
    if (mVias.value(via->getUuid()) != via) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      via->removeFromBoard();  // can throw
    }
    mVias.remove(via->getUuid());
    sgl.add([this, via]() {
      if (isAddedToBoard()) {
        via->addToBoard();
      }
      mVias.insert(via->getUuid(), via);
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
  if (mNetSignal) {
    mNetSignal->registerBoardNetSegment(*this);  // can throw
    sgl.add([&]() { mNetSignal->unregisterBoardNetSegment(*this); });
  }
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
  if (mNetSignal) {
    mNetSignal->unregisterBoardNetSegment(*this);  // can throw
    sgl.add([&]() { mNetSignal->registerBoardNetSegment(*this); });
  }

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
  root.ensureLineBreak();
  root.appendChild("net",
                   mNetSignal ? mNetSignal->getUuid() : tl::optional<Uuid>());
  root.ensureLineBreak();
  for (const BI_Via* obj : mVias) {
    root.ensureLineBreak();
    obj->getVia().serialize(root.appendList("via"));
  }
  root.ensureLineBreak();
  for (const BI_NetPoint* obj : mNetPoints) {
    root.ensureLineBreak();
    obj->getJunction().serialize(root.appendList("junction"));
  }
  root.ensureLineBreak();
  for (const BI_NetLine* obj : mNetLines) {
    root.ensureLineBreak();
    obj->getTrace().serialize(root.appendList("trace"));
  }
  root.ensureLineBreak();
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
  QSet<const BI_Via*> vias;
  QSet<const BI_NetPoint*> points;
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

}  // namespace librepcb

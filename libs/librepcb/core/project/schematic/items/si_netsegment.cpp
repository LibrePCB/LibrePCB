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
#include "si_netsegment.h"

#include "../../../utils/scopeguardlist.h"
#include "../../../utils/toolbox.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_netlabel.h"
#include "si_netline.h"
#include "si_netpoint.h"
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

SI_NetSegment::SI_NetSegment(Schematic& schematic, const Uuid& uuid,
                             NetSignal& signal)
  : SI_Base(schematic), mUuid(uuid), mNetSignal(&signal) {
}

SI_NetSegment::~SI_NetSegment() noexcept {
  // delete all items
  qDeleteAll(mNetLabels);
  mNetLabels.clear();
  qDeleteAll(mNetLines);
  mNetLines.clear();
  qDeleteAll(mNetPoints);
  mNetPoints.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SI_NetSegment::isUsed() const noexcept {
  return ((!mNetPoints.isEmpty()) || (!mNetLines.isEmpty()) ||
          (!mNetLabels.isEmpty()));
}

QSet<QString> SI_NetSegment::getForcedNetNames() const noexcept {
  QSet<QString> names;
  foreach (SI_NetLine* netline, mNetLines) {
    SI_SymbolPin* pin1 = dynamic_cast<SI_SymbolPin*>(&netline->getStartPoint());
    SI_SymbolPin* pin2 = dynamic_cast<SI_SymbolPin*>(&netline->getEndPoint());
    ComponentSignalInstance* sig1 =
        pin1 ? pin1->getComponentSignalInstance() : nullptr;
    ComponentSignalInstance* sig2 =
        pin2 ? pin2->getComponentSignalInstance() : nullptr;
    if (sig1 && sig1->isNetSignalNameForced())
      names.insert(sig1->getForcedNetSignalName());
    if (sig2 && sig2->isNetSignalNameForced())
      names.insert(sig2->getForcedNetSignalName());
  }
  return names;
}

QString SI_NetSegment::getForcedNetName() const noexcept {
  QSet<QString> names = getForcedNetNames();
  if (names.count() == 1) {
    return names.values().first();
  } else {
    return QString();
  }
}

Point SI_NetSegment::calcNearestPoint(const Point& p) const noexcept {
  Point pos = p;
  Length dist;
  for (auto it = mNetLines.begin(); it != mNetLines.end(); it++) {
    Point lp;
    UnsignedLength ld = Toolbox::shortestDistanceBetweenPointAndLine(
        p, it.value()->getStartPoint().getPosition(),
        it.value()->getEndPoint().getPosition(), &lp);
    if ((it == mNetLines.begin()) || (ld < dist)) {
      dist = *ld;
      pos = lp;
    }
  }
  return pos;
}

QSet<SI_SymbolPin*> SI_NetSegment::getAllConnectedPins() const noexcept {
  Q_ASSERT(isAddedToSchematic());
  QSet<SI_SymbolPin*> pins;
  foreach (const SI_NetLine* netline, mNetLines) {
    SI_SymbolPin* p1 = dynamic_cast<SI_SymbolPin*>(&netline->getStartPoint());
    SI_SymbolPin* p2 = dynamic_cast<SI_SymbolPin*>(&netline->getEndPoint());
    if (p1) {
      pins.insert(p1);
      Q_ASSERT(p1->getCompSigInstNetSignal() == mNetSignal);
    }
    if (p2) {
      pins.insert(p2);
      Q_ASSERT(p2->getCompSigInstNetSignal() == mNetSignal);
    }
  }

  return pins;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_NetSegment::setNetSignal(NetSignal& netsignal) {
  if (&netsignal != mNetSignal) {
    if ((isUsed() && isAddedToSchematic()) ||
        (netsignal.getCircuit() != getCircuit())) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToSchematic()) {
      mNetSignal->unregisterSchematicNetSegment(*this);  // can throw
      auto sg =
          scopeGuard([&]() { mNetSignal->registerSchematicNetSegment(*this); });
      netsignal.registerSchematicNetSegment(*this);  // can throw
      sg.dismiss();
    }
    mNetSignal = &netsignal;
  }
}

/*******************************************************************************
 *  NetPoint+NetLine Methods
 ******************************************************************************/

void SI_NetSegment::addNetPointsAndNetLines(
    const QList<SI_NetPoint*>& netpoints, const QList<SI_NetLine*>& netlines) {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (SI_NetPoint* netpoint, netpoints) {
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
    netpoint->addToSchematic();  // can throw
    mNetPoints.insert(netpoint->getUuid(), netpoint);
    sgl.add([this, netpoint]() {
      netpoint->removeFromSchematic();
      mNetPoints.remove(netpoint->getUuid());
    });
  }
  foreach (SI_NetLine* netline, netlines) {
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
    netline->addToSchematic();  // can throw
    mNetLines.insert(netline->getUuid(), netline);
    sgl.add([this, netline]() {
      netline->removeFromSchematic();
      mNetLines.remove(netline->getUuid());
    });
  }

  if (!areAllNetPointsConnectedTogether()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The netsegment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  updateAllNetLabelAnchors();

  sgl.dismiss();
}

void SI_NetSegment::removeNetPointsAndNetLines(
    const QList<SI_NetPoint*>& netpoints, const QList<SI_NetLine*>& netlines) {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (SI_NetLine* netline, netlines) {
    if (mNetLines.value(netline->getUuid()) != netline) {
      throw LogicError(__FILE__, __LINE__);
    }
    netline->removeFromSchematic();  // can throw
    mNetLines.remove(netline->getUuid());
    sgl.add([this, netline]() {
      netline->addToSchematic();
      mNetLines.insert(netline->getUuid(), netline);
    });
  }
  foreach (SI_NetPoint* netpoint, netpoints) {
    if (mNetPoints.value(netpoint->getUuid()) != netpoint) {
      throw LogicError(__FILE__, __LINE__);
    }
    netpoint->removeFromSchematic();  // can throw
    mNetPoints.remove(netpoint->getUuid());
    sgl.add([this, netpoint]() {
      netpoint->addToSchematic();
      mNetPoints.insert(netpoint->getUuid(), netpoint);
    });
  }

  if (!areAllNetPointsConnectedTogether()) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("The netsegment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  updateAllNetLabelAnchors();

  sgl.dismiss();
}

/*******************************************************************************
 *  NetLabel Methods
 ******************************************************************************/

void SI_NetSegment::addNetLabel(SI_NetLabel& netlabel) {
  if ((!isAddedToSchematic()) || (mNetLabels.values().contains(&netlabel)) ||
      (&netlabel.getNetSegment() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetLabels.contains(netlabel.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a netlabel with the UUID \"%1\"!")
            .arg(netlabel.getUuid().toStr()));
  }
  netlabel.addToSchematic();  // can throw
  mNetLabels.insert(netlabel.getUuid(), &netlabel);
}

void SI_NetSegment::removeNetLabel(SI_NetLabel& netlabel) {
  if ((!isAddedToSchematic()) ||
      (mNetLabels.value(netlabel.getUuid()) != &netlabel)) {
    throw LogicError(__FILE__, __LINE__);
  }
  netlabel.removeFromSchematic();  // can throw
  mNetLabels.remove(netlabel.getUuid());
}

void SI_NetSegment::updateAllNetLabelAnchors() noexcept {
  foreach (SI_NetLabel* netlabel, mNetLabels) { netlabel->updateAnchor(); }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_NetSegment::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mNetPoints.count() + mNetLines.count() +
                     mNetLabels.count() + 1);
  mNetSignal->registerSchematicNetSegment(*this);  // can throw
  sgl.add([&]() { mNetSignal->unregisterSchematicNetSegment(*this); });
  foreach (SI_NetPoint* netpoint, mNetPoints) {
    netpoint->addToSchematic();  // can throw
    sgl.add([netpoint]() { netpoint->removeFromSchematic(); });
  }
  foreach (SI_NetLine* netline, mNetLines) {
    netline->addToSchematic();  // can throw
    sgl.add([netline]() { netline->removeFromSchematic(); });
  }
  foreach (SI_NetLabel* netlabel, mNetLabels) {
    netlabel->addToSchematic();  // can throw
    sgl.add([netlabel]() { netlabel->removeFromSchematic(); });
  }

  SI_Base::addToSchematic(nullptr);
  sgl.dismiss();
}

void SI_NetSegment::removeFromSchematic() {
  if ((!isAddedToSchematic())) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mNetPoints.count() + mNetLines.count() +
                     mNetLabels.count() + 1);
  foreach (SI_NetLabel* netlabel, mNetLabels) {
    netlabel->removeFromSchematic();  // can throw
    sgl.add([netlabel]() { netlabel->addToSchematic(); });
  }
  foreach (SI_NetLine* netline, mNetLines) {
    netline->removeFromSchematic();  // can throw
    sgl.add([netline]() { netline->addToSchematic(); });
  }
  foreach (SI_NetPoint* netpoint, mNetPoints) {
    netpoint->removeFromSchematic();  // can throw
    sgl.add([netpoint]() { netpoint->addToSchematic(); });
  }
  mNetSignal->unregisterSchematicNetSegment(*this);  // can throw
  sgl.add([&]() { mNetSignal->registerSchematicNetSegment(*this); });

  SI_Base::removeFromSchematic(nullptr);
  sgl.dismiss();
}

void SI_NetSegment::selectAll() noexcept {
  foreach (SI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(true);
  foreach (SI_NetLine* netline, mNetLines)
    netline->setSelected(true);
  foreach (SI_NetLabel* netlabel, mNetLabels)
    netlabel->setSelected(true);
}

void SI_NetSegment::setSelectionRect(const QRectF rectPx) noexcept {
  foreach (SI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(netpoint->getGrabAreaScenePx().intersects(rectPx));
  foreach (SI_NetLine* netline, mNetLines)
    netline->setSelected(netline->getGrabAreaScenePx().intersects(rectPx));
  foreach (SI_NetLabel* netlabel, mNetLabels)
    netlabel->setSelected(netlabel->getGrabAreaScenePx().intersects(rectPx));
}

void SI_NetSegment::clearSelection() const noexcept {
  foreach (SI_NetPoint* netpoint, mNetPoints)
    netpoint->setSelected(false);
  foreach (SI_NetLine* netline, mNetLines)
    netline->setSelected(false);
  foreach (SI_NetLabel* netlabel, mNetLabels)
    netlabel->setSelected(false);
}

void SI_NetSegment::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("net", mNetSignal->getUuid());
  root.ensureLineBreak();
  for (const SI_NetPoint* obj : mNetPoints) {
    root.ensureLineBreak();
    obj->getJunction().serialize(root.appendList("junction"));
  }
  root.ensureLineBreak();
  for (const SI_NetLine* obj : mNetLines) {
    root.ensureLineBreak();
    obj->getNetLine().serialize(root.appendList("line"));
  }
  root.ensureLineBreak();
  for (const SI_NetLabel* obj : mNetLabels) {
    root.ensureLineBreak();
    obj->getNetLabel().serialize(root.appendList("label"));
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_NetSegment::getGrabAreaScenePx() const noexcept {
  return QPainterPath();
}

bool SI_NetSegment::isSelected() const noexcept {
  if (mNetLines.isEmpty()) return false;
  foreach (const SI_NetLine* netline, mNetLines) {
    if (!netline->isSelected()) return false;
  }
  return true;
}

void SI_NetSegment::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SI_NetSegment::checkAttributesValidity() const noexcept {
  if (mNetSignal == nullptr) return false;
  if (!areAllNetPointsConnectedTogether()) return false;
  return true;
}

bool SI_NetSegment::areAllNetPointsConnectedTogether() const noexcept {
  if (mNetPoints.count() > 1) {
    const SI_NetPoint* firstPoint = mNetPoints.first();
    QSet<const SI_SymbolPin*> pins;
    QSet<const SI_NetPoint*> points;
    findAllConnectedNetPoints(*firstPoint, pins, points);
    return (points.count() == mNetPoints.count());
  } else {
    return true;  // there is only 0 or 1 netpoint => must be "connected
                  // together" :)
  }
}

void SI_NetSegment::findAllConnectedNetPoints(
    const SI_NetLineAnchor& p, QSet<const SI_SymbolPin*>& pins,
    QSet<const SI_NetPoint*>& points) const noexcept {
  if (const SI_SymbolPin* pin = dynamic_cast<const SI_SymbolPin*>(&p)) {
    if (pins.contains(pin)) return;
    pins.insert(pin);
    foreach (const SI_NetLine* netline, mNetLines) {
      if (&netline->getStartPoint() == pin) {
        findAllConnectedNetPoints(netline->getEndPoint(), pins, points);
      }
      if (&netline->getEndPoint() == pin) {
        findAllConnectedNetPoints(netline->getStartPoint(), pins, points);
      }
    }
  } else if (const SI_NetPoint* np = dynamic_cast<const SI_NetPoint*>(&p)) {
    if (points.contains(np)) return;
    points.insert(np);
    foreach (const SI_NetLine* netline, mNetLines) {
      if (&netline->getStartPoint() == np) {
        findAllConnectedNetPoints(netline->getEndPoint(), pins, points);
      }
      if (&netline->getEndPoint() == np) {
        findAllConnectedNetPoints(netline->getStartPoint(), pins, points);
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

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

#include <librepcb/common/scopeguardlist.h>
#include <librepcb/common/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_NetSegment::SI_NetSegment(Schematic& schematic, const SExpression& node)
  : SI_Base(schematic),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mNetSignal(nullptr) {
  try {
    Uuid netSignalUuid = deserialize<Uuid>(node.getChild("net/@0"));
    mNetSignal =
        mSchematic.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if (!mNetSignal) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid net signal UUID: \"%1\"")
                             .arg(netSignalUuid.toStr()));
    }

    // Load all netpoints
    foreach (const SExpression& child, node.getChildren("junction")) {
      SI_NetPoint* netpoint = new SI_NetPoint(*this, child);
      if (getNetPointByUuid(netpoint->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a netpoint with the UUID \"%1\"!")
                .arg(netpoint->getUuid().toStr()));
      }
      mNetPoints.append(netpoint);
    }

    // Load all netlines
    foreach (const SExpression& child,
             node.getChildren("netline") + node.getChildren("line")) {
      SI_NetLine* netline = new SI_NetLine(*this, child);
      if (getNetLineByUuid(netline->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a netline with the UUID \"%1\"!")
                .arg(netline->getUuid().toStr()));
      }
      mNetLines.append(netline);
    }

    // Load all netlabels
    foreach (const SExpression& child,
             node.getChildren("netlabel") + node.getChildren("label")) {
      SI_NetLabel* netlabel = new SI_NetLabel(*this, child);
      if (getNetLabelByUuid(netlabel->getUuid())) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("There is already a netlabel with the UUID \"%1\"!")
                .arg(netlabel->getUuid().toStr()));
      }
      mNetLabels.append(netlabel);
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
    qDeleteAll(mNetLabels);
    mNetLabels.clear();
    qDeleteAll(mNetLines);
    mNetLines.clear();
    qDeleteAll(mNetPoints);
    mNetPoints.clear();
    throw;  // ...and rethrow the exception
  }
}

SI_NetSegment::SI_NetSegment(Schematic& schematic, NetSignal& signal)
  : SI_Base(schematic), mUuid(Uuid::createRandom()), mNetSignal(&signal) {
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

int SI_NetSegment::getNetPointsAtScenePos(const Point& pos,
                                          QList<SI_NetPoint*>& points) const
    noexcept {
  int count = 0;
  foreach (SI_NetPoint* netpoint, mNetPoints) {
    if (netpoint->getGrabAreaScenePx().contains(pos.toPxQPointF())) {
      points.append(netpoint);
      ++count;
    }
  }
  return count;
}

int SI_NetSegment::getNetLinesAtScenePos(const Point& pos,
                                         QList<SI_NetLine*>& lines) const
    noexcept {
  int count = 0;
  foreach (SI_NetLine* netline, mNetLines) {
    if (netline->getGrabAreaScenePx().contains(pos.toPxQPointF())) {
      lines.append(netline);
      ++count;
    }
  }
  return count;
}

int SI_NetSegment::getNetLabelsAtScenePos(const Point& pos,
                                          QList<SI_NetLabel*>& labels) const
    noexcept {
  int count = 0;
  foreach (SI_NetLabel* netlabel, mNetLabels) {
    if (netlabel->getGrabAreaScenePx().contains(pos.toPxQPointF())) {
      labels.append(netlabel);
      ++count;
    }
  }
  return count;
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
  for (int i = 0; i < mNetLines.count(); ++i) {
    Point lp;
    UnsignedLength ld = Toolbox::shortestDistanceBetweenPointAndLine(
        p, mNetLines.at(i)->getStartPoint().getPosition(),
        mNetLines.at(i)->getEndPoint().getPosition(), &lp);
    if ((i == 0) || (ld < dist)) {
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
 *  NetPoint Methods
 ******************************************************************************/

SI_NetPoint* SI_NetSegment::getNetPointByUuid(const Uuid& uuid) const noexcept {
  foreach (SI_NetPoint* netpoint, mNetPoints) {
    if (netpoint->getUuid() == uuid) return netpoint;
  }
  return nullptr;
}

/*******************************************************************************
 *  NetLine Methods
 ******************************************************************************/

SI_NetLine* SI_NetSegment::getNetLineByUuid(const Uuid& uuid) const noexcept {
  foreach (SI_NetLine* netline, mNetLines) {
    if (netline->getUuid() == uuid) return netline;
  }
  return nullptr;
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
    // add to schematic
    netpoint->addToSchematic();  // can throw
    mNetPoints.append(netpoint);
    sgl.add([this, netpoint]() {
      netpoint->removeFromSchematic();
      mNetPoints.removeOne(netpoint);
    });
  }
  foreach (SI_NetLine* netline, netlines) {
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
    // add to schematic
    netline->addToSchematic();  // can throw
    mNetLines.append(netline);
    sgl.add([this, netline]() {
      netline->removeFromSchematic();
      mNetLines.removeOne(netline);
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

void SI_NetSegment::removeNetPointsAndNetLines(
    const QList<SI_NetPoint*>& netpoints, const QList<SI_NetLine*>& netlines) {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(netpoints.count() + netlines.count());
  foreach (SI_NetLine* netline, netlines) {
    if (!mNetLines.contains(netline)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    netline->removeFromSchematic();  // can throw
    mNetLines.removeOne(netline);
    sgl.add([this, netline]() {
      netline->addToSchematic();
      mNetLines.append(netline);
    });
  }
  foreach (SI_NetPoint* netpoint, netpoints) {
    if (!mNetPoints.contains(netpoint)) {
      throw LogicError(__FILE__, __LINE__);
    }
    // remove from schematic
    netpoint->removeFromSchematic();  // can throw
    mNetPoints.removeOne(netpoint);
    sgl.add([this, netpoint]() {
      netpoint->addToSchematic();
      mNetPoints.append(netpoint);
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

SI_NetLabel* SI_NetSegment::getNetLabelByUuid(const Uuid& uuid) const noexcept {
  foreach (SI_NetLabel* netlabel, mNetLabels) {
    if (netlabel->getUuid() == uuid) return netlabel;
  }
  return nullptr;
}

void SI_NetSegment::addNetLabel(SI_NetLabel& netlabel) {
  if ((!isAddedToSchematic()) || (mNetLabels.contains(&netlabel)) ||
      (&netlabel.getNetSegment() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  // check if there is no netlabel with the same uuid in the list
  if (getNetLabelByUuid(netlabel.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a netlabel with the UUID \"%1\"!")
            .arg(netlabel.getUuid().toStr()));
  }
  // add to schematic
  netlabel.addToSchematic();  // can throw
  mNetLabels.append(&netlabel);
}

void SI_NetSegment::removeNetLabel(SI_NetLabel& netlabel) {
  if ((!isAddedToSchematic()) || (!mNetLabels.contains(&netlabel))) {
    throw LogicError(__FILE__, __LINE__);
  }
  // remove from schematic
  netlabel.removeFromSchematic();  // can throw
  mNetLabels.removeOne(&netlabel);
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
  root.appendChild("net", mNetSignal->getUuid(), true);
  serializePointerContainerUuidSorted(root, mNetPoints, "junction");
  serializePointerContainerUuidSorted(root, mNetLines, "line");
  serializePointerContainerUuidSorted(root, mNetLabels, "label");
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

}  // namespace project
}  // namespace librepcb

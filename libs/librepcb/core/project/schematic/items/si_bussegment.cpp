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
#include "si_bussegment.h"

#include "../../../utils/scopeguardlist.h"
#include "../../../utils/toolbox.h"
#include "../../circuit/bus.h"
#include "../../circuit/circuit.h"
#include "../../project.h"
#include "../schematic.h"
#include "si_busjunction.h"
#include "si_buslabel.h"
#include "si_busline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_BusSegment::SI_BusSegment(Schematic& schematic, const Uuid& uuid, Bus& bus)
  : SI_Base(schematic), mUuid(uuid), mBus(&bus) {
}

SI_BusSegment::~SI_BusSegment() noexcept {
  // delete all items
  qDeleteAll(mLabels);
  mLabels.clear();
  qDeleteAll(mLines);
  mLines.clear();
  qDeleteAll(mJunctions);
  mJunctions.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SI_BusSegment::isUsed() const noexcept {
  return ((!mJunctions.isEmpty()) || (!mLines.isEmpty()) ||
          (!mLabels.isEmpty()));
}

Point SI_BusSegment::calcNearestPoint(const Point& p) const noexcept {
  Point pos = p;
  Length dist;
  for (auto it = mLines.begin(); it != mLines.end(); it++) {
    Point lp;
    UnsignedLength ld = Toolbox::shortestDistanceBetweenPointAndLine(
        p, it.value()->getP1().getPosition(), it.value()->getP2().getPosition(),
        &lp);
    if ((it == mLines.begin()) || (ld < dist)) {
      dist = *ld;
      pos = lp;
    }
  }
  return pos;
}

QSet<SI_NetSegment*> SI_BusSegment::getAttachedNetSegments() const noexcept {
  QSet<SI_NetSegment*> set;
  for (SI_BusJunction* bj : mJunctions) {
    for (SI_NetLine* nl : bj->getNetLines()) {
      set.insert(&nl->getNetSegment());
    }
  }
  return set;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_BusSegment::setBus(Bus& bus) {
  if (&bus != mBus) {
    if ((isUsed() && isAddedToSchematic()) ||
        (bus.getCircuit() != getCircuit())) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToSchematic()) {
      mBus->unregisterSchematicBusSegment(*this);  // can throw
      auto sg = scopeGuard([&]() { mBus->registerSchematicBusSegment(*this); });
      bus.registerSchematicBusSegment(*this);  // can throw
      sg.dismiss();
    }
    mBus = &bus;
  }
}

/*******************************************************************************
 *  Junction/Line Methods
 ******************************************************************************/

void SI_BusSegment::addJunctionsAndLines(
    const QList<SI_BusJunction*>& junctions, const QList<SI_BusLine*>& lines) {
  ScopeGuardList sgl(junctions.count() + lines.count());
  foreach (SI_BusJunction* bj, junctions) {
    if ((mJunctions.values().contains(bj)) || (&bj->getBusSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (mJunctions.contains(bj->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a bus junction with the UUID \"%1\"!")
              .arg(bj->getUuid().toStr()));
    }
    if (isAddedToSchematic()) {
      bj->addToSchematic();  // can throw
    }
    mJunctions.insert(bj->getUuid(), bj);
    sgl.add([this, bj]() {
      if (isAddedToSchematic()) {
        bj->removeFromSchematic();
      }
      mJunctions.remove(bj->getUuid());
    });
  }
  foreach (SI_BusLine* bl, lines) {
    if ((mLines.values().contains(bl)) || (&bl->getBusSegment() != this)) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (mLines.contains(bl->getUuid())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There is already a bus line with the UUID \"%1\"!")
              .arg(bl->getUuid().toStr()));
    }
    if (isAddedToSchematic()) {
      bl->addToSchematic();  // can throw
    }
    mLines.insert(bl->getUuid(), bl);
    sgl.add([this, bl]() {
      if (isAddedToSchematic()) {
        bl->removeFromSchematic();
      }
      mLines.remove(bl->getUuid());
    });
  }

  if (!areAllJunctionsConnectedTogether()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The bus segment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  updateAllLabelAnchors();

  sgl.dismiss();

  emit junctionsAndLinesAdded(junctions, lines);
}

void SI_BusSegment::removeJunctionsAndLines(
    const QList<SI_BusJunction*>& junctions, const QList<SI_BusLine*>& lines) {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(junctions.count() + lines.count());
  foreach (SI_BusLine* bl, lines) {
    if (mLines.value(bl->getUuid()) != bl) {
      throw LogicError(__FILE__, __LINE__);
    }
    bl->removeFromSchematic();  // can throw
    mLines.remove(bl->getUuid());
    sgl.add([this, bl]() {
      bl->addToSchematic();
      mLines.insert(bl->getUuid(), bl);
    });
  }
  foreach (SI_BusJunction* bj, junctions) {
    if (mJunctions.value(bj->getUuid()) != bj) {
      throw LogicError(__FILE__, __LINE__);
    }
    bj->removeFromSchematic();  // can throw
    mJunctions.remove(bj->getUuid());
    sgl.add([this, bj]() {
      bj->addToSchematic();
      mJunctions.insert(bj->getUuid(), bj);
    });
  }

  if (!areAllJunctionsConnectedTogether()) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("The bus segment with the UUID \"%1\" is not cohesive!")
            .arg(mUuid.toStr()));
  }

  updateAllLabelAnchors();

  sgl.dismiss();

  emit junctionsAndLinesRemoved(junctions, lines);
}

/*******************************************************************************
 *  Label Methods
 ******************************************************************************/

void SI_BusSegment::addLabel(SI_BusLabel& label) {
  if ((mLabels.values().contains(&label)) || (&label.getBusSegment() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mLabels.contains(label.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a bus label with the UUID \"%1\"!")
            .arg(label.getUuid().toStr()));
  }
  if (isAddedToSchematic()) {
    label.addToSchematic();  // can throw
  }
  mLabels.insert(label.getUuid(), &label);
  emit labelAdded(label);
}

void SI_BusSegment::removeLabel(SI_BusLabel& label) {
  if ((mLabels.value(label.getUuid()) != &label)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isAddedToSchematic()) {
    label.removeFromSchematic();  // can throw
  }
  mLabels.remove(label.getUuid());
  emit labelRemoved(label);
}

void SI_BusSegment::updateAllLabelAnchors() noexcept {
  foreach (SI_BusLabel* label, mLabels) {
    label->updateAnchor();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_BusSegment::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mJunctions.count() + mLines.count() + mLabels.count() + 1);
  mBus->registerSchematicBusSegment(*this);  // can throw
  sgl.add([&]() { mBus->unregisterSchematicBusSegment(*this); });
  foreach (SI_BusJunction* bj, mJunctions) {
    bj->addToSchematic();  // can throw
    sgl.add([bj]() { bj->removeFromSchematic(); });
  }
  foreach (SI_BusLine* bl, mLines) {
    bl->addToSchematic();  // can throw
    sgl.add([bl]() { bl->removeFromSchematic(); });
  }
  foreach (SI_BusLabel* bl, mLabels) {
    bl->addToSchematic();  // can throw
    sgl.add([bl]() { bl->removeFromSchematic(); });
  }

  SI_Base::addToSchematic();
  sgl.dismiss();
}

void SI_BusSegment::removeFromSchematic() {
  if ((!isAddedToSchematic())) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mJunctions.count() + mLines.count() + mLabels.count() + 1);
  foreach (SI_BusLabel* bl, mLabels) {
    bl->removeFromSchematic();  // can throw
    sgl.add([bl]() { bl->addToSchematic(); });
  }
  foreach (SI_BusLine* bl, mLines) {
    bl->removeFromSchematic();  // can throw
    sgl.add([bl]() { bl->addToSchematic(); });
  }
  foreach (SI_BusJunction* bj, mJunctions) {
    bj->removeFromSchematic();  // can throw
    sgl.add([bj]() { bj->addToSchematic(); });
  }
  mBus->unregisterSchematicBusSegment(*this);  // can throw
  sgl.add([&]() { mBus->registerSchematicBusSegment(*this); });

  SI_Base::removeFromSchematic();
  sgl.dismiss();
}

void SI_BusSegment::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("bus", mBus->getUuid());
  root.ensureLineBreak();
  for (const SI_BusJunction* obj : mJunctions) {
    root.ensureLineBreak();
    obj->getJunction().serialize(root.appendList("junction"));
  }
  root.ensureLineBreak();
  for (const SI_BusLine* obj : mLines) {
    root.ensureLineBreak();
    obj->getNetLine().serialize(root.appendList("line"));
  }
  root.ensureLineBreak();
  for (const SI_BusLabel* obj : mLabels) {
    root.ensureLineBreak();
    obj->getNetLabel().serialize(root.appendList("label"));
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SI_BusSegment::checkAttributesValidity() const noexcept {
  if (!mBus) return false;
  if (!areAllJunctionsConnectedTogether()) return false;
  return true;
}

bool SI_BusSegment::areAllJunctionsConnectedTogether() const noexcept {
  const SI_BusJunction* p = nullptr;
  if (mJunctions.count() > 0) {
    p = mJunctions.first();
  } else if (mLines.count() > 0) {
    p = &mLines.first()->getP1();
  } else {
    return true;  // Empty segment is considered as valid.
  }
  Q_ASSERT(p);
  QSet<const SI_BusJunction*> points;
  QSet<const SI_BusLine*> lines;
  findAllConnectedJunctions(*p, points, lines);
  return (points.count() == mJunctions.count()) &&
      (lines.count() == mLines.count());
}

void SI_BusSegment::findAllConnectedJunctions(
    const SI_BusJunction& np, QSet<const SI_BusJunction*>& points,
    QSet<const SI_BusLine*>& lines) const noexcept {
  if (points.contains(&np)) return;
  points.insert(&np);
  foreach (const SI_BusLine* line, mLines) {
    if (&line->getP1() == &np) {
      findAllConnectedJunctions(line->getP2(), points, lines);
    }
    if (&line->getP2() == &np) {
      findAllConnectedJunctions(line->getP1(), points, lines);
    }
    if ((&line->getP1() == &np) || (&line->getP2() == &np)) {
      lines.insert(line);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

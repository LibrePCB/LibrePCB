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
#include "bus.h"

#include "../../utils/toolbox.h"
#include "../schematic/items/si_bussegment.h"
#include "../schematic/items/si_netsegment.h"
#include "circuit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Bus::Bus(Circuit& circuit, const Uuid& uuid, const BusName& name, bool autoName,
         bool prefixNetNames,
         const std::optional<UnsignedLength>& maxTraceLengthDifference)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mHasAutoName(autoName),
    mPrefixNetNames(prefixNetNames),
    mMaxTraceLengthDifference(maxTraceLengthDifference) {
}

Bus::~Bus() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

QSet<NetSignal*> Bus::getConnectedNetSignals() const noexcept {
  QSet<NetSignal*> nets;
  for (SI_BusSegment* busSeg : mRegisteredSchematicBusSegments) {
    for (SI_NetSegment* netSeg : busSeg->getAttachedNetSegments()) {
      nets.insert(&netSeg->getNetSignal());
    }
  }
  return nets;
}

bool Bus::isUsed() const noexcept {
  return !mRegisteredSchematicBusSegments.isEmpty();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Bus::setName(const BusName& name, bool isAutoName) noexcept {
  if ((name == mName) && (isAutoName == mHasAutoName)) {
    return;
  }
  mName = name;
  mHasAutoName = isAutoName;
  emit nameChanged(mName);
}

void Bus::setPrefixNetNames(bool prefix) noexcept {
  mPrefixNetNames = prefix;
}

void Bus::setMaxTraceLengthDifference(
    const std::optional<UnsignedLength>& diff) noexcept {
  mMaxTraceLengthDifference = diff;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Bus::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mIsAddedToCircuit = true;
}

void Bus::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The bus \"%1\" cannot be removed because it is still in use!")
            .arg(*mName));
  }
  mIsAddedToCircuit = false;
}

void Bus::registerSchematicBusSegment(SI_BusSegment& s) {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__, "Bus is not added to circuit.");
  }
  if (mRegisteredSchematicBusSegments.contains(&s)) {
    throw LogicError(__FILE__, __LINE__, "Bus segment already in bus.");
  }
  if (s.getCircuit() != mCircuit) {
    throw LogicError(__FILE__, __LINE__, "Bus segment is from other circuit.");
  }
  mRegisteredSchematicBusSegments.append(&s);
}

void Bus::unregisterSchematicBusSegment(SI_BusSegment& s) {
  if ((!mIsAddedToCircuit) || (!mRegisteredSchematicBusSegments.contains(&s))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSchematicBusSegments.removeOne(&s);
}

void Bus::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("auto", mHasAutoName);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild("prefix_nets", mPrefixNetNames);
  root.appendChild("max_trace_length_difference", mMaxTraceLengthDifference);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

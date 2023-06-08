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
#include "boardzonedata.h"

#include "../../serialization/sexpression.h"
#include "../../types/layer.h"
#include "../../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardZoneData::BoardZoneData(const BoardZoneData& other) noexcept
  : mUuid(other.mUuid),
    mLayers(other.mLayers),
    mRules(other.mRules),
    mOutline(other.mOutline),
    mLocked(other.mLocked) {
}

BoardZoneData::BoardZoneData(const Uuid& uuid,
                             const BoardZoneData& other) noexcept
  : mUuid(uuid),
    mLayers(other.mLayers),
    mRules(other.mRules),
    mOutline(other.mOutline),
    mLocked(other.mLocked) {
}

BoardZoneData::BoardZoneData(const Uuid& uuid, const QSet<const Layer*>& layers,
                             Zone::Rules rules, const Path& outline,
                             bool locked) noexcept
  : mUuid(uuid),
    mLayers(layers),
    mRules(rules),
    mOutline(outline),
    mLocked(locked) {
}

BoardZoneData::BoardZoneData(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayers(),
    mRules(0),
    mOutline(Path(node)),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))) {
  foreach (const SExpression* child, node.getChildren("layer")) {
    mLayers.insert(&deserialize<const Layer&>(child->getChild("@0")));
  }
  checkLayers(mLayers);

  mRules.setFlag(Zone::Rule::NoCopper,
                 deserialize<bool>(node.getChild("no_copper/@0")));
  mRules.setFlag(Zone::Rule::NoPlanes,
                 deserialize<bool>(node.getChild("no_planes/@0")));
  mRules.setFlag(Zone::Rule::NoExposure,
                 deserialize<bool>(node.getChild("no_exposure/@0")));
  mRules.setFlag(Zone::Rule::NoDevices,
                 deserialize<bool>(node.getChild("no_devices/@0")));
}

BoardZoneData::~BoardZoneData() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardZoneData::setLayers(const QSet<const Layer*>& layers) {
  if (layers == mLayers) {
    return false;
  }

  checkLayers(layers);  // can throw
  mLayers = layers;
  return true;
}

bool BoardZoneData::setRules(Zone::Rules rules) noexcept {
  if (rules == mRules) {
    return false;
  }

  mRules = rules;
  return true;
}

bool BoardZoneData::setOutline(const Path& outline) noexcept {
  if (outline == mOutline) {
    return false;
  }

  mOutline = outline;
  return true;
}

bool BoardZoneData::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardZoneData::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("no_copper", mRules.testFlag(Zone::Rule::NoCopper));
  root.appendChild("no_planes", mRules.testFlag(Zone::Rule::NoPlanes));
  root.appendChild("no_exposure", mRules.testFlag(Zone::Rule::NoExposure));
  root.appendChild("no_devices", mRules.testFlag(Zone::Rule::NoDevices));
  root.ensureLineBreak();
  foreach (const Layer* layer, Toolbox::sortedQSet(mLayers, &Layer::lessThan)) {
    root.appendChild("layer", *layer);
    root.ensureLineBreak();
  }
  root.appendChild("lock", mLocked);
  root.ensureLineBreak();
  mOutline.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BoardZoneData::operator==(const BoardZoneData& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayers != rhs.mLayers) return false;
  if (mRules != rhs.mRules) return false;
  if (mOutline != rhs.mOutline) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardZoneData::checkLayers(const QSet<const Layer*>& layers) {
  foreach (const Layer* layer, layers) {
    if (!layer->isCopper()) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid zone layer: %1").arg(layer->getId()));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

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
#include "zone.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Zone::Zone(const Zone& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mLayers(other.mLayers),
    mRules(other.mRules),
    mOutline(other.mOutline) {
}

Zone::Zone(const Uuid& uuid, const Zone& other) noexcept : Zone(other) {
  mUuid = uuid;
}

Zone::Zone(const Uuid& uuid, Layers layers, Rules rules,
           const Path& outline) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayers(layers),
    mRules(rules),
    mOutline(outline) {
}

Zone::Zone(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayers(0),
    mRules(0),
    mOutline(Path(node)) {
  mLayers.setFlag(Layer::Top, deserialize<bool>(node.getChild("top/@0")));
  mLayers.setFlag(Layer::Inner, deserialize<bool>(node.getChild("inner/@0")));
  mLayers.setFlag(Layer::Bottom, deserialize<bool>(node.getChild("bottom/@0")));
  mRules.setFlag(Rule::NoCopper,
                 deserialize<bool>(node.getChild("no_copper/@0")));
  mRules.setFlag(Rule::NoPlanes,
                 deserialize<bool>(node.getChild("no_planes/@0")));
  mRules.setFlag(Rule::NoExposure,
                 deserialize<bool>(node.getChild("no_exposure/@0")));
  mRules.setFlag(Rule::NoDevices,
                 deserialize<bool>(node.getChild("no_devices/@0")));
}

Zone::~Zone() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Zone::setLayers(Layers layers) noexcept {
  if (layers == mLayers) {
    return false;
  }

  mLayers = layers;
  onEdited.notify(Event::LayersChanged);
  return true;
}

bool Zone::setRules(Rules rules) noexcept {
  if (rules == mRules) {
    return false;
  }

  mRules = rules;
  onEdited.notify(Event::RulesChanged);
  return true;
}

bool Zone::setOutline(const Path& outline) noexcept {
  if (outline == mOutline) {
    return false;
  }

  mOutline = outline;
  onEdited.notify(Event::OutlineChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Zone::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("no_copper", mRules.testFlag(Rule::NoCopper));
  root.appendChild("no_planes", mRules.testFlag(Rule::NoPlanes));
  root.appendChild("no_exposure", mRules.testFlag(Rule::NoExposure));
  root.appendChild("no_devices", mRules.testFlag(Rule::NoDevices));
  root.ensureLineBreak();
  root.appendChild("top", mLayers.testFlag(Layer::Top));
  root.appendChild("inner", mLayers.testFlag(Layer::Inner));
  root.appendChild("bottom", mLayers.testFlag(Layer::Bottom));
  root.ensureLineBreak();
  mOutline.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Zone::operator==(const Zone& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayers != rhs.mLayers) return false;
  if (mRules != rhs.mRules) return false;
  if (mOutline != rhs.mOutline) return false;
  return true;
}

Zone& Zone::operator=(const Zone& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setLayers(rhs.mLayers);
  setRules(rhs.mRules);
  setOutline(rhs.mOutline);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

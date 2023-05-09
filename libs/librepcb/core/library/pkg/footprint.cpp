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
#include "footprint.h"

#include "package.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Footprint::Footprint(const Footprint& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mNames(other.mNames),
    mDescriptions(other.mDescriptions),
    mModelPosition(other.mModelPosition),
    mModelRotation(other.mModelRotation),
    mModels(other.mModels),
    mPads(other.mPads),
    mPolygons(other.mPolygons),
    mCircles(other.mCircles),
    mStrokeTexts(other.mStrokeTexts),
    mHoles(other.mHoles),
    mNamesEditedSlot(*this, &Footprint::namesEdited),
    mDescriptionsEditedSlot(*this, &Footprint::descriptionsEdited),
    mPadsEditedSlot(*this, &Footprint::padsEdited),
    mPolygonsEditedSlot(*this, &Footprint::polygonsEdited),
    mCirclesEditedSlot(*this, &Footprint::circlesEdited),
    mStrokeTextsEditedSlot(*this, &Footprint::strokeTextsEdited),
    mHolesEditedSlot(*this, &Footprint::holesEdited) {
  mNames.onEdited.attach(mNamesEditedSlot);
  mDescriptions.onEdited.attach(mDescriptionsEditedSlot);
  mPads.onEdited.attach(mPadsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mStrokeTexts.onEdited.attach(mStrokeTextsEditedSlot);
  mHoles.onEdited.attach(mHolesEditedSlot);
}

Footprint::Footprint(const Uuid& uuid, const ElementName& name_en_US,
                     const QString& description_en_US)
  : onEdited(*this),
    mUuid(uuid),
    mNames(name_en_US),
    mDescriptions(description_en_US),
    mModelPosition(),
    mModelRotation(),
    mModels(),
    mPads(),
    mPolygons(),
    mCircles(),
    mStrokeTexts(),
    mHoles(),
    mNamesEditedSlot(*this, &Footprint::namesEdited),
    mDescriptionsEditedSlot(*this, &Footprint::descriptionsEdited),
    mPadsEditedSlot(*this, &Footprint::padsEdited),
    mPolygonsEditedSlot(*this, &Footprint::polygonsEdited),
    mCirclesEditedSlot(*this, &Footprint::circlesEdited),
    mStrokeTextsEditedSlot(*this, &Footprint::strokeTextsEdited),
    mHolesEditedSlot(*this, &Footprint::holesEdited) {
  mNames.onEdited.attach(mNamesEditedSlot);
  mDescriptions.onEdited.attach(mDescriptionsEditedSlot);
  mPads.onEdited.attach(mPadsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mStrokeTexts.onEdited.attach(mStrokeTextsEditedSlot);
  mHoles.onEdited.attach(mHolesEditedSlot);
}

Footprint::Footprint(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mNames(node),
    mDescriptions(node),
    mModelPosition(deserialize<Point3D>(node.getChild("3d_position"))),
    mModelRotation(deserialize<Angle3D>(node.getChild("3d_rotation"))),
    mModels(),
    mPads(node),
    mPolygons(node),
    mCircles(node),
    mStrokeTexts(node),
    mHoles(node),
    mNamesEditedSlot(*this, &Footprint::namesEdited),
    mDescriptionsEditedSlot(*this, &Footprint::descriptionsEdited),
    mPadsEditedSlot(*this, &Footprint::padsEdited),
    mPolygonsEditedSlot(*this, &Footprint::polygonsEdited),
    mCirclesEditedSlot(*this, &Footprint::circlesEdited),
    mStrokeTextsEditedSlot(*this, &Footprint::strokeTextsEdited),
    mHolesEditedSlot(*this, &Footprint::holesEdited) {
  foreach (const SExpression* child, node.getChildren("3d_model")) {
    mModels.insert(deserialize<Uuid>(child->getChild("@0")));
  }
  mNames.onEdited.attach(mNamesEditedSlot);
  mDescriptions.onEdited.attach(mDescriptionsEditedSlot);
  mPads.onEdited.attach(mPadsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mStrokeTexts.onEdited.attach(mStrokeTextsEditedSlot);
  mHoles.onEdited.attach(mHolesEditedSlot);
}

Footprint::~Footprint() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Footprint::setModelPosition(const Point3D& position) noexcept {
  if (position == mModelPosition) {
    return false;
  }

  mModelPosition = position;
  onEdited.notify(Event::ModelPositionChanged);
  return true;
}

bool Footprint::setModelRotation(const Angle3D& rotation) noexcept {
  if (rotation == mModelRotation) {
    return false;
  }

  mModelRotation = rotation;
  onEdited.notify(Event::ModelRotationChanged);
  return true;
}

bool Footprint::setModels(const QSet<Uuid>& models) noexcept {
  if (models == mModels) {
    return false;
  }

  mModels = models;
  onEdited.notify(Event::ModelsChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Footprint::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  mNames.serialize(root);
  root.ensureLineBreak();
  mDescriptions.serialize(root);
  root.ensureLineBreak();
  {
    SExpression& child = root.appendList("3d_position");
    child.appendChild(std::get<0>(mModelPosition));
    child.appendChild(std::get<1>(mModelPosition));
    child.appendChild(std::get<2>(mModelPosition));
  }
  {
    SExpression& child = root.appendList("3d_rotation");
    child.appendChild(std::get<0>(mModelRotation));
    child.appendChild(std::get<1>(mModelRotation));
    child.appendChild(std::get<2>(mModelRotation));
  }
  root.ensureLineBreak();
  foreach (const Uuid& uuid, Toolbox::sortedQSet(mModels)) {
    root.appendChild("3d_model", uuid);
    root.ensureLineBreak();
  }
  mPads.serialize(root);
  root.ensureLineBreak();
  mPolygons.serialize(root);
  root.ensureLineBreak();
  mCircles.serialize(root);
  root.ensureLineBreak();
  mStrokeTexts.serialize(root);
  root.ensureLineBreak();
  mHoles.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Footprint::operator==(const Footprint& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mNames != rhs.mNames) return false;
  if (mDescriptions != rhs.mDescriptions) return false;
  if (mModelPosition != rhs.mModelPosition) return false;
  if (mModelRotation != rhs.mModelRotation) return false;
  if (mModels != rhs.mModels) return false;
  if (mPads != rhs.mPads) return false;
  if (mPolygons != rhs.mPolygons) return false;
  if (mCircles != rhs.mCircles) return false;
  if (mStrokeTexts != rhs.mStrokeTexts) return false;
  if (mHoles != rhs.mHoles) return false;
  return true;
}

Footprint& Footprint::operator=(const Footprint& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  mNames = rhs.mNames;
  mDescriptions = rhs.mDescriptions;
  setModelPosition(rhs.mModelPosition);
  setModelRotation(rhs.mModelRotation);
  setModels(rhs.mModels);
  mPads = rhs.mPads;
  mPolygons = rhs.mPolygons;
  mCircles = rhs.mCircles;
  mStrokeTexts = rhs.mStrokeTexts;
  mHoles = rhs.mHoles;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Footprint::namesEdited(const LocalizedNameMap& names, const QString& key,
                            LocalizedNameMap::Event event) noexcept {
  Q_UNUSED(names);
  Q_UNUSED(key);
  Q_UNUSED(event);
  onEdited.notify(Event::NamesEdited);
}

void Footprint::descriptionsEdited(
    const LocalizedDescriptionMap& names, const QString& key,
    LocalizedDescriptionMap::Event event) noexcept {
  Q_UNUSED(names);
  Q_UNUSED(key);
  Q_UNUSED(event);
  onEdited.notify(Event::DescriptionsEdited);
}

void Footprint::padsEdited(const FootprintPadList& list, int index,
                           const std::shared_ptr<const FootprintPad>& pad,
                           FootprintPadList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(pad);
  Q_UNUSED(event);
  onEdited.notify(Event::PadsEdited);
}

void Footprint::polygonsEdited(const PolygonList& list, int index,
                               const std::shared_ptr<const Polygon>& polygon,
                               PolygonList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(polygon);
  Q_UNUSED(event);
  onEdited.notify(Event::PolygonsEdited);
}

void Footprint::circlesEdited(const CircleList& list, int index,
                              const std::shared_ptr<const Circle>& circle,
                              CircleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(circle);
  Q_UNUSED(event);
  onEdited.notify(Event::CirclesEdited);
}

void Footprint::strokeTextsEdited(const StrokeTextList& list, int index,
                                  const std::shared_ptr<const StrokeText>& text,
                                  StrokeTextList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(text);
  Q_UNUSED(event);
  onEdited.notify(Event::StrokeTextsEdited);
}

void Footprint::holesEdited(const HoleList& list, int index,
                            const std::shared_ptr<const Hole>& hole,
                            HoleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(hole);
  Q_UNUSED(event);
  onEdited.notify(Event::HolesEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

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

#include "footprintgraphicsitem.h"
#include "package.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Footprint::Footprint(const Footprint& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mNames(other.mNames),
    mDescriptions(other.mDescriptions),
    mPads(other.mPads),
    mPolygons(other.mPolygons),
    mCircles(other.mCircles),
    mStrokeTexts(other.mStrokeTexts),
    mHoles(other.mHoles),
    mStrokeFont(nullptr),
    mRegisteredGraphicsItem(nullptr),
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
    mPads(),
    mPolygons(),
    mCircles(),
    mStrokeTexts(),
    mHoles(),
    mStrokeFont(nullptr),
    mRegisteredGraphicsItem(nullptr),
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
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mNames(node),
    mDescriptions(node),
    mPads(node),
    mPolygons(node),
    mCircles(node),
    mStrokeTexts(node),
    mHoles(node),
    mStrokeFont(nullptr),
    mRegisteredGraphicsItem(nullptr),
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

Footprint::~Footprint() noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Footprint::setStrokeFontForAllTexts(const StrokeFont* font) noexcept {
  mStrokeFont = font;
  for (StrokeText& text : mStrokeTexts) {
    text.setFont(mStrokeFont);
  }
}

void Footprint::registerGraphicsItem(FootprintGraphicsItem& item) noexcept {
  Q_ASSERT(!mRegisteredGraphicsItem);
  mRegisteredGraphicsItem = &item;
}

void Footprint::unregisterGraphicsItem(FootprintGraphicsItem& item) noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == &item);
  mRegisteredGraphicsItem = nullptr;
}

void Footprint::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  mNames.serialize(root);
  mDescriptions.serialize(root);
  mPads.serialize(root);
  mPolygons.serialize(root);
  mCircles.serialize(root);
  mStrokeTexts.serialize(root);
  mHoles.serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Footprint::operator==(const Footprint& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mNames != rhs.mNames) return false;
  if (mDescriptions != rhs.mDescriptions) return false;
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
  mNames        = rhs.mNames;
  mDescriptions = rhs.mDescriptions;
  mPads         = rhs.mPads;
  mPolygons     = rhs.mPolygons;
  mCircles      = rhs.mCircles;
  mStrokeTexts  = rhs.mStrokeTexts;
  mHoles        = rhs.mHoles;
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
  switch (event) {
    case FootprintPadList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addPad(const_cast<FootprintPad&>(*pad));
      }
      break;
    case FootprintPadList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removePad(const_cast<FootprintPad&>(*pad));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::PadsEdited);
}

void Footprint::polygonsEdited(const PolygonList& list, int index,
                               const std::shared_ptr<const Polygon>& polygon,
                               PolygonList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case PolygonList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addPolygon(const_cast<Polygon&>(*polygon));
      }
      break;
    case PolygonList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removePolygon(const_cast<Polygon&>(*polygon));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::PolygonsEdited);
}

void Footprint::circlesEdited(const CircleList& list, int index,
                              const std::shared_ptr<const Circle>& circle,
                              CircleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case CircleList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addCircle(const_cast<Circle&>(*circle));
      }
      break;
    case CircleList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removeCircle(const_cast<Circle&>(*circle));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::CirclesEdited);
}

void Footprint::strokeTextsEdited(const StrokeTextList& list, int index,
                                  const std::shared_ptr<const StrokeText>& text,
                                  StrokeTextList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case StrokeTextList::Event::ElementAdded:
      const_cast<StrokeText&>(*text).setFont(mStrokeFont);
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addStrokeText(const_cast<StrokeText&>(*text));
      }
      break;
    case StrokeTextList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removeStrokeText(
            const_cast<StrokeText&>(*text));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::StrokeTextsEdited);
}

void Footprint::holesEdited(const HoleList& list, int index,
                            const std::shared_ptr<const Hole>& hole,
                            HoleList::Event                    event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case HoleList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addHole(const_cast<Hole&>(*hole));
      }
      break;
    case HoleList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removeHole(const_cast<Hole&>(*hole));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::HolesEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

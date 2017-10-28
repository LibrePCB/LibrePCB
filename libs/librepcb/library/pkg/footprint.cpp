/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "footprint.h"
#include "package.h"
#include "footprintgraphicsitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Footprint::Footprint(const Footprint& other) noexcept :
    mPads(this), mPolygons(this), mEllipses(this), mTexts(this), mHoles(this),
    mRegisteredGraphicsItem(nullptr)
{
    *this = other;
}

Footprint::Footprint(const Uuid& uuid, const QString& name_en_US,
                     const QString& description_en_US) :
    mUuid(uuid), mPads(this), mPolygons(this), mEllipses(this), mTexts(this), mHoles(this),
    mRegisteredGraphicsItem(nullptr)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.setDefaultValue(name_en_US);
    mDescriptions.setDefaultValue(description_en_US);
}

Footprint::Footprint(const SExpression& node) :
    mPads(this), mPolygons(this), mEllipses(this), mTexts(this), mHoles(this),
    mRegisteredGraphicsItem(nullptr)
{
    // read attributes
    mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
    mNames.loadFromDomElement(node);
    mDescriptions.loadFromDomElement(node);

    // load all elements
    mPads.loadFromDomElement(node);
    mPolygons.loadFromDomElement(node);
    mEllipses.loadFromDomElement(node);
    mTexts.loadFromDomElement(node);
    mHoles.loadFromDomElement(node);
}

Footprint::~Footprint() noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Footprint::registerGraphicsItem(FootprintGraphicsItem& item) noexcept
{
    Q_ASSERT(!mRegisteredGraphicsItem);
    mRegisteredGraphicsItem = &item;
}

void Footprint::unregisterGraphicsItem(FootprintGraphicsItem& item) noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == &item);
    mRegisteredGraphicsItem = nullptr;
}

void Footprint::serialize(SExpression& root) const
{
    root.appendToken(mUuid);
    mNames.serialize(root);
    mDescriptions.serialize(root);
    mPads.serialize(root);
    mPolygons.serialize(root);
    mEllipses.serialize(root);
    mTexts.serialize(root);
    mHoles.serialize(root);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Footprint::operator==(const Footprint& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                     return false;
    if (mNames != rhs.mNames)                   return false;
    if (mDescriptions != rhs.mDescriptions)     return false;
    if (mPads != rhs.mPads)                     return false;
    if (mPolygons != rhs.mPolygons)             return false;
    if (mEllipses != rhs.mEllipses)             return false;
    if (mTexts != rhs.mTexts)                   return false;
    if (mHoles != rhs.mHoles)                   return false;
    return true;
}

Footprint& Footprint::operator=(const Footprint& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mNames = rhs.mNames;
    mDescriptions = rhs.mDescriptions;
    mPads = rhs.mPads;
    mPolygons = rhs.mPolygons;
    mEllipses = rhs.mEllipses;
    mTexts = rhs.mTexts;
    mHoles = rhs.mHoles;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Footprint::listObjectAdded(const FootprintPadList& list, int newIndex,
                                const std::shared_ptr<FootprintPad>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mPads);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPad(*ptr);
}

void Footprint::listObjectAdded(const PolygonList& list, int newIndex,
                                const std::shared_ptr<Polygon>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mPolygons);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPolygon(*ptr);
}

void Footprint::listObjectAdded(const EllipseList& list, int newIndex,
                                const std::shared_ptr<Ellipse>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mEllipses);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addEllipse(*ptr);
}

void Footprint::listObjectAdded(const TextList& list, int newIndex,
                                const std::shared_ptr<Text>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mTexts);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addText(*ptr);
}

void Footprint::listObjectAdded(const HoleList& list, int newIndex,
                                const std::shared_ptr<Hole>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mHoles);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addHole(*ptr);
}

void Footprint::listObjectRemoved(const FootprintPadList& list, int oldIndex,
                                  const std::shared_ptr<FootprintPad>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mPads);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePad(*ptr);
}

void Footprint::listObjectRemoved(const PolygonList& list, int oldIndex,
                                  const std::shared_ptr<Polygon>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mPolygons);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePolygon(*ptr);
}

void Footprint::listObjectRemoved(const EllipseList& list, int oldIndex,
                                  const std::shared_ptr<Ellipse>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mEllipses);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeEllipse(*ptr);
}

void Footprint::listObjectRemoved(const TextList& list, int oldIndex,
                                  const std::shared_ptr<Text>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mTexts);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeText(*ptr);
}

void Footprint::listObjectRemoved(const HoleList& list, int oldIndex,
                                  const std::shared_ptr<Hole>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mHoles);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeHole(*ptr);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

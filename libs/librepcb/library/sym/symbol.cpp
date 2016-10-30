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
#include "symbol.h"
#include "symbolgraphicsitem.h"
#include <librepcb/common/fileio/domdocument.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Symbol::Symbol(const Uuid& uuid, const Version& version, const QString& author,
               const QString& name_en_US, const QString& description_en_US,
               const QString& keywords_en_US) :
    LibraryElement(getShortElementName(), getLongElementName(), uuid, version, author,
                   name_en_US, description_en_US, keywords_en_US),
    mPins(this), mPolygons(this), mEllipses(this), mTexts(this),
    mRegisteredGraphicsItem(nullptr)
{
}

Symbol::Symbol(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly),
    mPins(this), mPolygons(this), mEllipses(this), mTexts(this),
    mRegisteredGraphicsItem(nullptr)
{
    const DomElement& root = mLoadingXmlFileDocument->getRoot();
    mPins.loadFromDomElement(root); // can throw
    mPolygons.loadFromDomElement(root); // can throw
    mEllipses.loadFromDomElement(root); // can throw
    mTexts.loadFromDomElement(root); // can throw

    cleanupAfterLoadingElementFromFile();
}

Symbol::~Symbol() noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Symbol::registerGraphicsItem(SymbolGraphicsItem& item) noexcept
{
    Q_ASSERT(!mRegisteredGraphicsItem);
    mRegisteredGraphicsItem = &item;
}

void Symbol::unregisterGraphicsItem(SymbolGraphicsItem& item) noexcept
{
    Q_ASSERT(mRegisteredGraphicsItem == &item);
    mRegisteredGraphicsItem = nullptr;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Symbol::listObjectAdded(const SymbolPinList& list, int newIndex,
                             const std::shared_ptr<SymbolPin>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mPins);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPin(*ptr);
}

void Symbol::listObjectAdded(const PolygonList& list, int newIndex,
                             const std::shared_ptr<Polygon>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mPolygons);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPolygon(*ptr);
}

void Symbol::listObjectAdded(const EllipseList& list, int newIndex,
                             const std::shared_ptr<Ellipse>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mEllipses);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addEllipse(*ptr);
}

void Symbol::listObjectAdded(const TextList& list, int newIndex,
                             const std::shared_ptr<Text>& ptr) noexcept
{
    Q_UNUSED(newIndex);
    Q_ASSERT(&list == &mTexts);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addText(*ptr);
}

void Symbol::listObjectRemoved(const SymbolPinList& list, int oldIndex,
                               const std::shared_ptr<SymbolPin>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mPins);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePin(*ptr);
}

void Symbol::listObjectRemoved(const PolygonList& list, int oldIndex,
                               const std::shared_ptr<Polygon>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mPolygons);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePolygon(*ptr);
}

void Symbol::listObjectRemoved(const EllipseList& list, int oldIndex,
                               const std::shared_ptr<Ellipse>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mEllipses);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeEllipse(*ptr);
}

void Symbol::listObjectRemoved(const TextList& list, int oldIndex,
                               const std::shared_ptr<Text>& ptr) noexcept
{
    Q_UNUSED(oldIndex);
    Q_ASSERT(&list == &mTexts);
    if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeText(*ptr);
}

void Symbol::serialize(DomElement& root) const
{
    LibraryElement::serialize(root);
    mPins.serialize(root);
    mPolygons.serialize(root);
    mEllipses.serialize(root);
    mTexts.serialize(root);
}

bool Symbol::checkAttributesValidity() const noexcept
{
    // symbol pin uuids and names must be unique
    QList<Uuid> uuids;
    QList<QString> names;
    for (const SymbolPin& pin : mPins) {
        Uuid uuid = pin.getUuid();
        QString name = pin.getName();
        if (uuid.isNull() || uuids.contains(uuid) || names.contains(name)) {
            return false;
        } else {
            uuids.append(uuid);
            names.append(name);
        }
    }

    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

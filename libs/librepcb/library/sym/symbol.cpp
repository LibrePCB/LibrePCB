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
                   name_en_US, description_en_US, keywords_en_US)
{
}

Symbol::Symbol(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly)
{
    try
    {
        const DomElement& root = mLoadingXmlFileDocument->getRoot();

        // Load all pins
        foreach (const DomElement* node, root.getChilds("pin")) {
            SymbolPin* pin = new SymbolPin(*node);
            if (mPins.contains(pin->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The pin \"%1\" exists multiple times in \"%2\"."))
                    .arg(pin->getUuid().toStr(), root.getDocFilePath().toNative()));
            }
            mPins.insert(pin->getUuid(), pin);
        }

        // Load all polygons
        foreach (const DomElement* node, root.getChilds("polygon")) {
            mPolygons.append(new Polygon(*node));
        }

        // Load all ellipses
        foreach (const DomElement* node, root.getChilds("ellipse")) {
            mEllipses.append(new Ellipse(*node));
        }

        // Load all texts
        foreach (const DomElement* node, root.getChilds("text")) {
            mTexts.append(new Text(*node));
        }

        cleanupAfterLoadingElementFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mTexts);         mTexts.clear();
        qDeleteAll(mEllipses);      mEllipses.clear();
        qDeleteAll(mPolygons);      mPolygons.clear();
        qDeleteAll(mPins);          mPins.clear();
        throw;
    }
}

Symbol::~Symbol() noexcept
{
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mEllipses);      mEllipses.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPins);          mPins.clear();
}

/*****************************************************************************************
 *  FootprintPad Methods
 ****************************************************************************************/

void Symbol::addPin(SymbolPin& pin) noexcept
{
    Q_ASSERT(!mPins.contains(pin.getUuid()));
    mPins.insert(pin.getUuid(), &pin);
}

void Symbol::removePin(SymbolPin& pin) noexcept
{
    Q_ASSERT(mPins.contains(pin.getUuid()));
    Q_ASSERT(mPins.value(pin.getUuid()) == &pin);
    mPins.remove(pin.getUuid());
}

/*****************************************************************************************
 *  Polygon Methods
 ****************************************************************************************/

void Symbol::addPolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(!mPolygons.contains(&polygon));
    mPolygons.append(&polygon);
}

void Symbol::removePolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(mPolygons.contains(&polygon));
    mPolygons.removeAll(&polygon);
}

/*****************************************************************************************
 *  Ellipse Methods
 ****************************************************************************************/

void Symbol::addEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(!mEllipses.contains(&ellipse));
    mEllipses.append(&ellipse);
}

void Symbol::removeEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(mEllipses.contains(&ellipse));
    mEllipses.removeAll(&ellipse);
}

/*****************************************************************************************
 *  Text Methods
 ****************************************************************************************/

void Symbol::addText(Text& text) noexcept
{
    Q_ASSERT(!mTexts.contains(&text));
    mTexts.append(&text);
}

void Symbol::removeText(Text& text) noexcept
{
    Q_ASSERT(mTexts.contains(&text));
    mTexts.removeAll(&text);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Symbol::serialize(DomElement& root) const
{
    LibraryElement::serialize(root);
    serializePointerContainer(root, mPins,     "pin");
    serializePointerContainer(root, mPolygons, "polygon");
    serializePointerContainer(root, mEllipses, "ellipse");
    serializePointerContainer(root, mTexts,    "text");
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

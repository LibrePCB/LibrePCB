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
#include <librepcb/common/fileio/domelement.h>
#include "footprint.h"
#include "package.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Footprint::Footprint(const Uuid& uuid, const QString& name_en_US,
                     const QString& description_en_US) throw (Exception) :
    mUuid(uuid)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.setDefaultValue(name_en_US);
    mDescriptions.setDefaultValue(description_en_US);
}

Footprint::Footprint(const DomElement& domElement) throw (Exception)
{
    try
    {
        // read attributes
        mUuid = domElement.getAttribute<Uuid>("uuid", true);
        mNames.loadFromDomElement(domElement);
        mDescriptions.loadFromDomElement(domElement);

        // Load all pads
        foreach (const DomElement* node, domElement.getChilds("pad")) {
            FootprintPad* pad = FootprintPad::fromDomElement(*node);
            if (mPads.contains(pad->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                    .arg(pad->getUuid().toStr(), domElement.getDocFilePath().toNative()));
            }
            mPads.insert(pad->getUuid(), pad);
        }

        // Load all polygons
        foreach (const DomElement* node, domElement.getChilds("polygon")) {
            mPolygons.append(new Polygon(*node));
        }

        // Load all ellipses
        foreach (const DomElement* node, domElement.getChilds("ellipse")) {
            mEllipses.append(new Ellipse(*node));
        }

        // Load all texts
        foreach (const DomElement* node, domElement.getChilds("text")) {
            mTexts.append(new Text(*node));
        }

        // Load all holes
        foreach (const DomElement* node, domElement.getChilds("hole")) {
            mHoles.append(new Hole(*node));
        }

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (Exception& e) {
        qDeleteAll(mHoles);         mHoles.clear();
        qDeleteAll(mTexts);         mTexts.clear();
        qDeleteAll(mEllipses);      mEllipses.clear();
        qDeleteAll(mPolygons);      mPolygons.clear();
        qDeleteAll(mPads);          mPads.clear();
        throw;
    }
}

Footprint::~Footprint() noexcept
{
    qDeleteAll(mHoles);         mHoles.clear();
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mEllipses);      mEllipses.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPads);          mPads.clear();
}

/*****************************************************************************************
 *  FootprintPad Methods
 ****************************************************************************************/

void Footprint::addPad(FootprintPad& pad) noexcept
{
    Q_ASSERT(!mPads.contains(pad.getUuid()));
    mPads.insert(pad.getUuid(), &pad);
}

void Footprint::removePad(FootprintPad& pad) noexcept
{
    Q_ASSERT(mPads.contains(pad.getUuid()));
    Q_ASSERT(mPads.value(pad.getUuid()) == &pad);
    mPads.remove(pad.getUuid());
}

/*****************************************************************************************
 *  Polygon Methods
 ****************************************************************************************/

void Footprint::addPolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(!mPolygons.contains(&polygon));
    mPolygons.append(&polygon);
}

void Footprint::removePolygon(Polygon& polygon) noexcept
{
    Q_ASSERT(mPolygons.contains(&polygon));
    mPolygons.removeAll(&polygon);
}

/*****************************************************************************************
 *  Ellipse Methods
 ****************************************************************************************/

void Footprint::addEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(!mEllipses.contains(&ellipse));
    mEllipses.append(&ellipse);
}

void Footprint::removeEllipse(Ellipse& ellipse) noexcept
{
    Q_ASSERT(mEllipses.contains(&ellipse));
    mEllipses.removeAll(&ellipse);
}

/*****************************************************************************************
 *  Text Methods
 ****************************************************************************************/

void Footprint::addText(Text& text) noexcept
{
    Q_ASSERT(!mTexts.contains(&text));
    mTexts.append(&text);
}

void Footprint::removeText(Text& text) noexcept
{
    Q_ASSERT(mTexts.contains(&text));
    mTexts.removeAll(&text);
}

/*****************************************************************************************
 *  Hole Methods
 ****************************************************************************************/

void Footprint::addHole(Hole& hole) noexcept
{
    Q_ASSERT(!mHoles.contains(&hole));
    mHoles.append(&hole);
}

void Footprint::removeHole(Hole& hole) noexcept
{
    Q_ASSERT(mHoles.contains(&hole));
    mHoles.removeAll(&hole);
}

/*****************************************************************************************
 *  Public Methods
 ****************************************************************************************/

void Footprint::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    mNames.serialize(root);
    mDescriptions.serialize(root);
    serializePointerContainer(root, mPads,     "pad");
    serializePointerContainer(root, mPolygons, "polygon");
    serializePointerContainer(root, mEllipses, "ellipse");
    serializePointerContainer(root, mTexts,    "text");
    serializePointerContainer(root, mHoles,    "hole");
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Footprint::checkAttributesValidity() const noexcept
{
    if (mPads.isEmpty() && mTexts.isEmpty() &&
        mPolygons.isEmpty() && mEllipses.isEmpty())                 return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

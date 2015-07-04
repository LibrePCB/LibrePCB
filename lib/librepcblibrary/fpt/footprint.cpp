/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <librepcbcommon/fileio/xmldomelement.h>
#include "footprint.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Footprint::Footprint(const QUuid& uuid, const Version& version, const QString& author,
                     const QString& name_en_US, const QString& description_en_US,
                     const QString& keywords_en_US) throw (Exception) :
    LibraryElement("footprint", uuid, version, author, name_en_US, description_en_US, keywords_en_US)
{
}

Footprint::Footprint(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "footprint")
{
    try
    {
        readFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mHoles);         mHoles.clear();
        qDeleteAll(mEllipses);      mEllipses.clear();
        qDeleteAll(mTexts);         mTexts.clear();
        qDeleteAll(mPolygons);      mPolygons.clear();
        qDeleteAll(mPads);          mPads.clear();
        throw;
    }
}

Footprint::~Footprint() noexcept
{
    qDeleteAll(mHoles);         mHoles.clear();
    qDeleteAll(mEllipses);      mEllipses.clear();
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPads);          mPads.clear();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Footprint::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    // Load all pads
    for (XmlDomElement* node = root.getFirstChild("pads/pad", true, false);
         node; node = node->getNextSibling("pad"))
    {
        FootprintPad* pad = new FootprintPad(*node);
        if (mPads.contains(pad->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pad->getUuid().toString(),
                QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                .arg(pad->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mPads.insert(pad->getUuid(), pad);
    }

    // Load all geometry elements
    for (XmlDomElement* node = root.getFirstChild("geometry/*", true, false);
         node; node = node->getNextSibling())
    {
        if (node->getName() == "polygon")
        {
            mPolygons.append(new FootprintPolygon(*node));
        }
        else if (node->getName() == "text")
        {
            mTexts.append(new FootprintText(*node));
        }
        else if (node->getName() == "ellipse")
        {
            mEllipses.append(new FootprintEllipse(*node));
        }
        else if (node->getName() == "hole")
        {
            FootprintHole_t* hole = new FootprintHole_t();
            hole->pos.setX(node->getAttribute<Length>("x", true));
            hole->pos.setY(node->getAttribute<Length>("y", true));
            hole->diameter = node->getAttribute<Length>("diameter", true);
            mHoles.append(hole);
        }
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getName(),
                QString(tr("Unknown geometry element \"%1\" in \"%2\"."))
                .arg(node->getName(), mXmlFilepath.toNative()));
        }
    }
}

XmlDomElement* Footprint::serializeToXmlDomElement(uint version) const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement(version));
    XmlDomElement* geometry = root->appendChild("geometry");
    foreach (const FootprintPolygon* polygon, mPolygons)
        geometry->appendChild(polygon->serializeToXmlDomElement(version));
    foreach (const FootprintText* text, mTexts)
        geometry->appendChild(text->serializeToXmlDomElement(version));
    foreach (const FootprintEllipse* ellipse, mEllipses)
        geometry->appendChild(ellipse->serializeToXmlDomElement(version));
    foreach (const FootprintHole_t* hole, mHoles)
    {
        XmlDomElement* child = geometry->appendChild("hole");
        child->setAttribute("x", hole->pos.getX());
        child->setAttribute("y", hole->pos.getY());
        child->setAttribute("diameter", hole->diameter);
    }
    XmlDomElement* pads = root->appendChild("pads");
    foreach (const FootprintPad* pad, mPads)
        pads->appendChild(pad->serializeToXmlDomElement(version));
    return root.take();
}

bool Footprint::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())                 return false;
    if (mPads.isEmpty() && mTexts.isEmpty() &&
        mPolygons.isEmpty() && mEllipses.isEmpty())                 return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

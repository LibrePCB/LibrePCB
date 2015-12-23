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
#include "package.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Footprint::Footprint(const Uuid& uuid, const QString& name_en_US,
                     const QString& description_en_US, bool isDefault) throw (Exception) :
    mUuid(uuid), mIsDefault(isDefault)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
}

Footprint::Footprint(const XmlDomElement& domElement) throw (Exception)
{
    try
    {
        // read attributes
        mUuid = domElement.getAttribute<Uuid>("uuid", true);
        mIsDefault = domElement.getAttribute<bool>("default", true);

        // read names and descriptions in all available languages
        LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
        LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);

        // Load all geometry elements
        for (XmlDomElement* node = domElement.getFirstChild("geometry/*", true, false);
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
            else if (node->getName() == "pad")
            {
                FootprintPad* pad = new FootprintPad(*node);
                if (mPads.contains(pad->getPadUuid()))
                {
                    throw RuntimeError(__FILE__, __LINE__, pad->getPadUuid().toStr(),
                        QString(tr("The pad \"%1\" exists multiple times in \"%2\"."))
                        .arg(pad->getPadUuid().toStr(), domElement.getDocFilePath().toNative()));
                }
                mPads.insert(pad->getPadUuid(), pad);
            }
            else
            {
                throw RuntimeError(__FILE__, __LINE__, node->getName(),
                    QString(tr("Unknown geometry element \"%1\" in \"%2\"."))
                    .arg(node->getName(), domElement.getDocFilePath().toNative()));
            }
        }

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
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
 *  Getters
 ****************************************************************************************/

QString Footprint::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString Footprint::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

/*****************************************************************************************
 *  Public Methods
 ****************************************************************************************/

XmlDomElement* Footprint::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("footprint"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("default", mIsDefault);
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    XmlDomElement* geometry = root->appendChild("geometry");
    foreach (const FootprintPolygon* polygon, mPolygons)
        geometry->appendChild(polygon->serializeToXmlDomElement());
    foreach (const FootprintText* text, mTexts)
        geometry->appendChild(text->serializeToXmlDomElement());
    foreach (const FootprintEllipse* ellipse, mEllipses)
        geometry->appendChild(ellipse->serializeToXmlDomElement());
    foreach (const FootprintHole_t* hole, mHoles)
    {
        XmlDomElement* child = geometry->appendChild("hole");
        child->setAttribute("x", hole->pos.getX());
        child->setAttribute("y", hole->pos.getY());
        child->setAttribute("diameter", hole->diameter);
    }
    foreach (const FootprintPad* pad, mPads)
        geometry->appendChild(pad->serializeToXmlDomElement());
    return root.take();
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

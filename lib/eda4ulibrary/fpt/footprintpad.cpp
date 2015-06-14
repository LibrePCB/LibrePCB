/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <eda4ucommon/fileio/xmldomelement.h>
#include "footprintpad.h"
#include "../librarybaseelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPad::FootprintPad(const QUuid& uuid, const QString& name_en_US,
                           const QString& description_en_US) noexcept :
    mUuid(uuid), mType(Type_t::ThtRect), mPosition(0, 0), mRotation(0), mWidth(0),
    mHeight(0), mDrillDiameter(0), mLayerId(0)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
}

FootprintPad::FootprintPad(const XmlDomElement& domElement) throw (Exception) :
    mUuid(), mPosition()
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mType = stringToType(domElement.getAttribute("type"));
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mRotation = domElement.getAttribute<Angle>("rotation");
    mWidth = domElement.getAttribute<Length>("width");
    mHeight = domElement.getAttribute<Length>("height");
    mDrillDiameter = domElement.getAttribute<Length>("drill");
    mLayerId = domElement.getAttribute<uint>("layer");

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintPad::~FootprintPad() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString FootprintPad::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString FootprintPad::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

QRectF FootprintPad::toPxQRectF() const noexcept
{
    QRectF rect;
    rect.setWidth(mWidth.toPx());
    rect.setHeight(mHeight.toPx());
    rect.translate(mPosition.toPxQPointF());
    return rect;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void FootprintPad::setName(const QString& locale, const QString& name) noexcept
{
    mNames.insert(locale, name);
}

void FootprintPad::setDescription(const QString& locale, const QString& description) noexcept
{
    mDescriptions.insert(locale, description);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* FootprintPad::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("pad"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("type", typeToString(mType));
    root->setAttribute("x", mPosition.getX().toMmString());
    root->setAttribute("y", mPosition.getY().toMmString());
    root->setAttribute("rotation", mRotation);
    root->setAttribute("width", mWidth);
    root->setAttribute("height", mHeight);
    root->setAttribute("drill", mDrillDiameter);
    root->setAttribute("layer", mLayerId);
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintPad::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (typeToString(mType).isEmpty())      return false;
    if (mWidth <= 0)                        return false;
    if (mHeight <= 0)                       return false;
    if (mDrillDiameter < 0)                 return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

FootprintPad::Type_t FootprintPad::stringToType(const QString& type) throw (Exception)
{
    if      (type == QLatin1String("tht_rect"))     return Type_t::ThtRect;
    else if (type == QLatin1String("tht_octagon"))  return Type_t::ThtOctagon;
    else if (type == QLatin1String("tht_round"))    return Type_t::ThtRound;
    else if (type == QLatin1String("smd_rect"))     return Type_t::SmdRect;
    else throw RuntimeError(__FILE__, __LINE__, type, type);
}

QString FootprintPad::typeToString(Type_t type) noexcept
{
    switch (type)
    {
        case Type_t::ThtRect:       return QString("tht_rect");
        case Type_t::ThtOctagon:    return QString("tht_octagon");
        case Type_t::ThtRound:      return QString("tht_round");
        case Type_t::SmdRect:       return QString("smd_rect");
        default: Q_ASSERT(false); return QString();
    }
}


/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

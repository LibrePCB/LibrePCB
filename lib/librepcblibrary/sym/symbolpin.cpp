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
#include "symbolpin.h"
#include "../librarybaseelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPin::SymbolPin(const QUuid& uuid, const QString& name_en_US,
                     const QString& description_en_US) noexcept :
    mUuid(uuid), mPosition(0, 0), mLength(0), mAngle(0)
{
    Q_ASSERT(mUuid.isNull() == false);
    mNames.insert("en_US", name_en_US);
    mDescriptions.insert("en_US", description_en_US);
}

SymbolPin::SymbolPin(const XmlDomElement& domElement) throw (Exception) :
    mUuid(), mPosition(), mLength(), mAngle()
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mLength = domElement.getAttribute<Length>("length");
    mAngle = domElement.getAttribute<Angle>("angle");

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolPin::~SymbolPin() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString SymbolPin::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString SymbolPin::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SymbolPin::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
}

void SymbolPin::setLength(const Length& length) noexcept
{
    mLength = length;
}

void SymbolPin::setAngle(const Angle& angle) noexcept
{
    mAngle = angle;
}

void SymbolPin::setName(const QString& locale, const QString& name) noexcept
{
    mNames.insert(locale, name);
}

void SymbolPin::setDescription(const QString& locale, const QString& description) noexcept
{
    mDescriptions.insert(locale, description);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* SymbolPin::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("pin"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("x", mPosition.getX().toMmString());
    root->setAttribute("y", mPosition.getY().toMmString());
    root->setAttribute("length", mLength.toMmString());
    root->setAttribute("angle", mAngle.toDegString());
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPin::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mLength < 0)                        return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

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
#include "attribute.h"
#include "genericcomponent.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Attribute::Attribute(GenericComponent& genComp,
                     const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mGenericComponent(genComp)
{
    // read attributes
    mKey = domElement.getAttribute("key", true);
    mType = stringToType(domElement.getAttribute("type", true));

    // read names, descriptions and default values in all available languages
    LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), domElement, "description", mDescriptions);
    LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), domElement, "default_value", mDefaultValues);
}

Attribute::~Attribute() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString Attribute::getName(const QString& locale ) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString Attribute::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

QString Attribute::getDefaultValue(const QString& locale ) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDefaultValues, locale);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Attribute::Type_t Attribute::stringToType(const QString& type) throw (Exception)
{
         if (type == "string")      return Type_t::String;
    else if (type == "length")      return Type_t::Length;
    else if (type == "resistance")  return Type_t::Resistance;
    else if (type == "capacitance") return Type_t::Capacitance;
    else if (type == "inductance")  return Type_t::Inductance;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, type,
            QString(tr("Invalid attribute type: \"%1\"")).arg(type));
    }
}

QString Attribute::typeToString(Type_t type) noexcept
{
    switch (type)
    {
        case Type_t::String:        return QStringLiteral("string");
        case Type_t::Length:        return QStringLiteral("length");
        case Type_t::Resistance:    return QStringLiteral("resistance");
        case Type_t::Capacitance:   return QStringLiteral("capacitance");
        case Type_t::Inductance:    return QStringLiteral("inductance");
        default:
            Q_ASSERT(false);
            qCritical() << "unknown attribute type:" << static_cast<int>(type);
            return QString();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

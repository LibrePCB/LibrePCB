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
#include "symbolpin.h"
#include "symbol.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPin::SymbolPin(Symbol& symbol, const XmlDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mLength = domElement.getAttribute<Length>("length");
    mAngle = domElement.getAttribute<Angle>("angle");

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(mSymbol.getXmlFilepath(), domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(mSymbol.getXmlFilepath(), domElement, "description", mDescriptions);
}

SymbolPin::~SymbolPin() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString SymbolPin::getName(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString SymbolPin::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

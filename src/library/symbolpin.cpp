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

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPin::SymbolPin(Symbol& symbol, const QDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol), mDomElement(domElement)
{
    mUuid = mDomElement.attribute("uuid");
    if (mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mSymbol.getXmlFilepath().toStr(),
            QString(tr("Invalid symbol pin UUID in file \"%1\"."))
            .arg(mSymbol.getXmlFilepath().toNative()));
    }

    // read geometry attributes
    mPosition.setXmm(mDomElement.attribute("x"));
    mPosition.setYmm(mDomElement.attribute("y"));
    mLength.setLengthMm(mDomElement.attribute("length"));
    mAngle.setAngleDeg(mDomElement.attribute("angle"));

    // read names and descriptions in all available languages
    LibraryBaseElement::readLocaleDomNodes(mSymbol.getXmlFilepath(), mDomElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(mSymbol.getXmlFilepath(), mDomElement, "description", mDescriptions);
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

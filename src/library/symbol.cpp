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
#include "symbol.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Symbol::Symbol(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "symbol")
{
    QDomElement tmpNode;

    // Load all pins
    if (mDomRoot.firstChildElement("pins").isNull())
        mDomRoot.appendChild(mXmlFile->getDocument().createElement("pins"));
    tmpNode = mDomRoot.firstChildElement("pins").firstChildElement("pin");
    while (!tmpNode.isNull())
    {
        SymbolPin* pin = new SymbolPin(*this, tmpNode);
        if (mPins.contains(pin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pin->getUuid().toString(),
                QString(tr("The pin \"%1\" exists multiple times in \"%2\"."))
                .arg(pin->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mPins.insert(pin->getUuid(), pin);
        tmpNode = tmpNode.nextSiblingElement("pin");
    }

    // Load all geometry elements
    if (mDomRoot.firstChildElement("geometry").isNull())
        mDomRoot.appendChild(mXmlFile->getDocument().createElement("geometry"));
    tmpNode = mDomRoot.firstChildElement("geometry").firstChildElement();
    while (!tmpNode.isNull())
    {
        if (tmpNode.nodeName() == "polygon")
        {
            mPolygons.append(new SymbolPolygon(*this, tmpNode));
        }
        else if (tmpNode.nodeName() == "text")
        {
            mTexts.append(new SymbolText(*this, tmpNode));
        }
        else
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.nodeName(),
                QString(tr("Unknown geometry element \"%1\" in \"%2\"."))
                .arg(tmpNode.nodeName(), mXmlFilepath.toNative()));
        }
        tmpNode = tmpNode.nextSiblingElement();
    }
}

Symbol::~Symbol() noexcept
{
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPins);          mPins.clear();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

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
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Symbol::Symbol(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "symbol")
{
    try
    {
        readFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mTexts);         mTexts.clear();
        qDeleteAll(mPolygons);      mPolygons.clear();
        qDeleteAll(mPins);          mPins.clear();
        throw;
    }
}

Symbol::~Symbol() noexcept
{
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPins);          mPins.clear();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Symbol::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    // Load all pins
    for (XmlDomElement* node = root.getFirstChild("pins/pin", true, false);
         node; node = node->getNextSibling("pin"))
    {
        SymbolPin* pin = new SymbolPin(*this, *node);
        if (mPins.contains(pin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pin->getUuid().toString(),
                QString(tr("The pin \"%1\" exists multiple times in \"%2\"."))
                .arg(pin->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mPins.insert(pin->getUuid(), pin);
    }

    // Load all geometry elements
    for (XmlDomElement* node = root.getFirstChild("geometry/*", true, false);
         node; node = node->getNextSibling())
    {
        if (node->getName() == "polygon")
        {
            mPolygons.append(new SymbolPolygon(*this, *node));
        }
        else if (node->getName() == "text")
        {
            mTexts.append(new SymbolText(*this, *node));
        }
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getName(),
                QString(tr("Unknown geometry element \"%1\" in \"%2\"."))
                .arg(node->getName(), mXmlFilepath.toNative()));
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

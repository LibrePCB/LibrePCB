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
#include "componentcategory.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentCategory::ComponentCategory(const FilePath& elementDirectory) :
    LibraryCategory(elementDirectory, "cmpcat", "component_category")
{
    readFromFile();
}

ComponentCategory::~ComponentCategory()
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ComponentCategory::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryCategory::parseDomTree(root);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

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
#include <QDomDocument>
#include <QDomElement>
#include "librarybaseelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryBaseElement::LibraryBaseElement(const FilePath& xmlFilePath,
                                       const QString& xmlRootNodeName) :
    QObject(0), mXmlFile(0)
{
    mXmlFile = new XmlFile(xmlFilePath, 0);
    Q_UNUSED(xmlRootNodeName);
}

LibraryBaseElement::~LibraryBaseElement()
{
    delete mXmlFile;        mXmlFile = 0;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

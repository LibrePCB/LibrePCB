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
#include "package.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Package::Package(const QUuid& uuid, const Version& version, const QString& author,
                 const QString& name_en_US, const QString& description_en_US,
                 const QString& keywords_en_US) throw (Exception) :
    LibraryElement("pkg", "package", uuid, version, author, name_en_US, description_en_US, keywords_en_US)
{
}

Package::Package(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "pkg", "package")
{
    readFromFile();
}

Package::~Package() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Package::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    mFootprintUuid = root.getFirstChild("meta/footprint", true, true)->getText<QUuid>(true);
}

XmlDomElement* Package::serializeToXmlDomElement(int version) const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement(version));
    root->getFirstChild("meta", true)->appendTextChild("footprint", mFootprintUuid);
    return root.take();
}

bool Package::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (mFootprintUuid.isNull())                                return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library

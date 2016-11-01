/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "librarycategory.h"
#include <librepcb/common/fileio/xmldomdocument.h>
#include <librepcb/common/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryCategory::LibraryCategory(const QString& shortElementName,
                                 const QString& longElementName, const Uuid& uuid,
                                 const Version& version, const QString& author,
                                 const QString& name_en_US, const QString& description_en_US,
                                 const QString& keywords_en_US) throw (Exception) :
    LibraryBaseElement(true, shortElementName, longElementName, uuid, version, author,
                       name_en_US, description_en_US, keywords_en_US)
{
}

LibraryCategory::LibraryCategory(const FilePath& elementDirectory,
                                 const QString& shortElementName,
                                 const QString& longElementName, bool readOnly) throw (Exception) :
    LibraryBaseElement(elementDirectory, true, shortElementName, longElementName, readOnly)
{
    XmlDomElement& root = mLoadingXmlFileDocument->getRoot();

    // read parent uuid
    mParentUuid = root.getFirstChild("meta/parent", true, true)->getText<Uuid>(false);
}

LibraryCategory::~LibraryCategory() noexcept
{
}

/*****************************************************************************************
 *  Protected Methods
 ****************************************************************************************/

XmlDomElement* LibraryCategory::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryBaseElement::serializeToXmlDomElement());
    root->getFirstChild("meta", true)->appendTextChild("parent", mParentUuid);
    return root.take();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

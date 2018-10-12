/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "libraryelement.h"

#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElement::LibraryElement(const QString& shortElementName,
                               const QString& longElementName, const Uuid& uuid,
                               const Version& version, const QString& author,
                               const ElementName& name_en_US,
                               const QString&     description_en_US,
                               const QString&     keywords_en_US)
  : LibraryBaseElement(true, shortElementName, longElementName, uuid, version,
                       author, name_en_US, description_en_US, keywords_en_US) {
}

LibraryElement::LibraryElement(const FilePath& elementDirectory,
                               const QString&  shortElementName,
                               const QString& longElementName, bool readOnly)
  : LibraryBaseElement(elementDirectory, true, shortElementName,
                       longElementName, readOnly) {
  // read category UUIDs
  foreach (const SExpression& node,
           mLoadingFileDocument.getChildren("category")) {
    mCategories.insert(node.getValueOfFirstChild<Uuid>());
  }
}

LibraryElement::~LibraryElement() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LibraryElement::serialize(SExpression& root) const {
  LibraryBaseElement::serialize(root);
  foreach (const Uuid& uuid, Toolbox::sortedQSet(mCategories)) {
    root.appendChild("category", uuid, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

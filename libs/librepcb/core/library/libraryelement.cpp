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

#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"
#include "libraryelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElement::LibraryElement(const QString& shortElementName,
                               const QString& longElementName, const Uuid& uuid,
                               const Version& version, const QString& author,
                               const ElementName& name_en_US,
                               const QString& description_en_US,
                               const QString& keywords_en_US)
  : LibraryBaseElement(true, shortElementName, longElementName, uuid, version,
                       author, name_en_US, description_en_US, keywords_en_US) {
}

LibraryElement::LibraryElement(
    std::unique_ptr<TransactionalDirectory> directory,
    const QString& shortElementName, const QString& longElementName)
  : LibraryBaseElement(std::move(directory), true, shortElementName,
                       longElementName) {
  // read category UUIDs
  foreach (const SExpression& node,
           mLoadingFileDocument.getChildren("category")) {
    mCategories.insert(
        deserialize<Uuid>(node.getChild("@0"), mLoadingFileFormat));
  }
}

LibraryElement::~LibraryElement() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList LibraryElement::runChecks() const {
  LibraryElementCheck check(*this);
  return check.runChecks();  // can throw
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

}  // namespace librepcb

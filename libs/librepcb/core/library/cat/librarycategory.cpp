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
#include "librarycategory.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryCategory::LibraryCategory(const QString& shortElementName,
                                 const QString& longElementName,
                                 const Uuid& uuid, const Version& version,
                                 const QString& author,
                                 const ElementName& name_en_US,
                                 const QString& description_en_US,
                                 const QString& keywords_en_US)
  : LibraryBaseElement(shortElementName, longElementName, uuid, version, author,
                       name_en_US, description_en_US, keywords_en_US) {
}

LibraryCategory::LibraryCategory(
    const QString& shortElementName, const QString& longElementName,
    std::unique_ptr<TransactionalDirectory> directory, const SExpression& root)
  : LibraryBaseElement(shortElementName, longElementName, true,
                       std::move(directory), root),
    mParentUuid(deserialize<std::optional<Uuid>>(root.getChild("parent/@0"))) {
}

LibraryCategory::~LibraryCategory() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LibraryCategory::serialize(SExpression& root) const {
  LibraryBaseElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("parent", mParentUuid);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

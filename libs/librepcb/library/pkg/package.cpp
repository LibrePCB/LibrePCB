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
#include "package.h"

#include "packagecheck.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Package::Package(const Uuid& uuid, const Version& version,
                 const QString& author, const ElementName& name_en_US,
                 const QString& description_en_US,
                 const QString& keywords_en_US)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, name_en_US, description_en_US, keywords_en_US) {
}

Package::Package(std::unique_ptr<TransactionalDirectory> directory)
  : LibraryElement(std::move(directory), getShortElementName(),
                   getLongElementName()) {
  mPads.loadFromDomElement(mLoadingFileDocument);
  mFootprints.loadFromDomElement(mLoadingFileDocument);

  cleanupAfterLoadingElementFromFile();
}

Package::~Package() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList Package::runChecks() const {
  PackageCheck check(*this);
  return check.runChecks();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Package::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  mPads.serialize(root);
  mFootprints.serialize(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

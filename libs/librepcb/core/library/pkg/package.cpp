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

#include "../../serialization/fileformatmigration.h"
#include "packagecheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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

Package::Package(std::unique_ptr<TransactionalDirectory> directory,
                 const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    mPads(root),
    mFootprints(root) {
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

std::unique_ptr<Package> Package::open(
    std::unique_ptr<TransactionalDirectory> directory) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  for (auto migration : FileFormatMigration::getMigrations(fileFormat)) {
    migration->upgradePackage(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  return std::unique_ptr<Package>(new Package(std::move(directory), root));
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Package::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.ensureLineBreak();
  mPads.serialize(root);
  root.ensureLineBreak();
  mFootprints.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

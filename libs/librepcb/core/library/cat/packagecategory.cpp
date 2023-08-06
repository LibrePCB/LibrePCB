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
#include "packagecategory.h"

#include "../../serialization/fileformatmigration.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageCategory::PackageCategory(const Uuid& uuid, const Version& version,
                                 const QString& author,
                                 const ElementName& name_en_US,
                                 const QString& description_en_US,
                                 const QString& keywords_en_US)
  : LibraryCategory(getShortElementName(), getLongElementName(), uuid, version,
                    author, name_en_US, description_en_US, keywords_en_US) {
}

PackageCategory::PackageCategory(
    std::unique_ptr<TransactionalDirectory> directory, const SExpression& root)
  : LibraryCategory(getShortElementName(), getLongElementName(),
                    std::move(directory), root) {
}

PackageCategory::~PackageCategory() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<PackageCategory> PackageCategory::open(
    std::unique_ptr<TransactionalDirectory> directory,
    bool abortBeforeMigration) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  const auto migrations = FileFormatMigration::getMigrations(fileFormat);
  if (abortBeforeMigration && (!migrations.isEmpty())) {
    return nullptr;
  }
  for (auto migration : migrations) {
    migration->upgradePackageCategory(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  std::unique_ptr<PackageCategory> obj(
      new PackageCategory(std::move(directory), root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
  }
  return obj;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

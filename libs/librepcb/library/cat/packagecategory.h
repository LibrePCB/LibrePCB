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

#ifndef LIBREPCB_LIBRARY_PACKAGECATEGORY_H
#define LIBREPCB_LIBRARY_PACKAGECATEGORY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "librarycategory.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class PackageCategory
 ******************************************************************************/

/**
 * @brief The PackageCategory class
 */
class PackageCategory final : public LibraryCategory {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageCategory()                             = delete;
  PackageCategory(const PackageCategory& other) = delete;
  PackageCategory(const Uuid& uuid, const Version& version,
                  const QString& author, const ElementName& name_en_US,
                  const QString& description_en_US,
                  const QString& keywords_en_US);
  explicit PackageCategory(std::unique_ptr<TransactionalDirectory> directory);
  ~PackageCategory() noexcept;

  // Operator Overloadings
  PackageCategory& operator=(const PackageCategory& rhs) = delete;

  // Static Methods
  static QString getShortElementName() noexcept {
    return QStringLiteral("pkgcat");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("package_category");
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_PACKAGECATEGORY_H

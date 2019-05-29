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

#ifndef LIBREPCB_LIBRARY_PACKAGE_H
#define LIBREPCB_LIBRARY_PACKAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryelement.h"
#include "footprint.h"
#include "packagepad.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class Package
 ******************************************************************************/

/**
 * @brief The Package class represents a package of a component (including
 * footprint and 3D model)
 *
 * Following information is considered as the "interface" of a package and must
 * therefore never be changed:
 *  - UUID
 *  - Package pads (neither adding nor removing pads is allowed)
 *    - UUID
 *  - Footprints (adding new footprints is allowed, but removing not)
 *    - UUID
 *    - Footprint pads (neither adding nor removing pads is allowed)
 *      - UUID
 */
class Package final : public LibraryElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Package()                     = delete;
  Package(const Package& other) = delete;
  Package(const Uuid& uuid, const Version& version, const QString& author,
          const ElementName& name_en_US, const QString& description_en_US,
          const QString& keywords_en_US);
  explicit Package(std::unique_ptr<TransactionalDirectory> directory);
  ~Package() noexcept;

  // Getters
  PackagePadList&       getPads() noexcept { return mPads; }
  const PackagePadList& getPads() const noexcept { return mPads; }
  FootprintList&        getFootprints() noexcept { return mFootprints; }
  const FootprintList&  getFootprints() const noexcept { return mFootprints; }

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;

  // Operator Overloadings
  Package& operator=(const Package& rhs) = delete;

  // Static Methods
  static QString getShortElementName() noexcept {
    return QStringLiteral("pkg");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("package");
  }

private:  // Methods
  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

private:                       // Data
  PackagePadList mPads;        ///< empty list if the package has no pads
  FootprintList  mFootprints;  ///< minimum one footprint
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_PACKAGE_H

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

#ifndef LIBREPCB_LIBRARY_PACKAGECHECK_H
#define LIBREPCB_LIBRARY_PACKAGECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "libraryelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class Package;

/*******************************************************************************
 *  Class PackageCheck
 ******************************************************************************/

/**
 * @brief The PackageCheck class
 */
class PackageCheck : public LibraryElementCheck {
public:
  // Constructors / Destructor
  PackageCheck() = delete;
  PackageCheck(const PackageCheck& other) = delete;
  explicit PackageCheck(const Package& package) noexcept;
  virtual ~PackageCheck() noexcept;

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;

  // Operator Overloadings
  PackageCheck& operator=(const PackageCheck& rhs) = delete;

protected:  // Methods
  void checkDuplicatePadNames(MsgList& msgs) const;
  void checkMissingFootprint(MsgList& msgs) const;
  void checkMissingTexts(MsgList& msgs) const;
  void checkWrongTextLayers(MsgList& msgs) const;
  void checkPadsOverlapWithPlacement(MsgList& msgs) const;

private:  // Data
  const Package& mPackage;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_PACKAGECHECK_H

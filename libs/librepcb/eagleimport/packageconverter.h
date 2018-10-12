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

#ifndef LIBREPCB_EAGLEIMPORT_PACKAGECONVERTER_H
#define LIBREPCB_EAGLEIMPORT_PACKAGECONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/graphics/graphicslayername.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace parseagle {
class Package;
}

namespace librepcb {

namespace library {
class Package;
}

namespace eagleimport {

class ConverterDb;

/*******************************************************************************
 *  Class PackageConverter
 ******************************************************************************/

/**
 * @brief The PackageConverter class
 */
class PackageConverter final {
public:
  // Constructors / Destructor
  PackageConverter()                              = delete;
  PackageConverter(const PackageConverter& other) = delete;
  PackageConverter(const parseagle::Package& package, ConverterDb& db) noexcept;
  ~PackageConverter() noexcept;

  // General Methods
  std::unique_ptr<library::Package> generate() const;

  // Operator Overloadings
  PackageConverter& operator=(const PackageConverter& rhs) = delete;

private:
  QString                  createDescription() const noexcept;
  static GraphicsLayerName convertBoardLayer(int eagleLayerId);

  const parseagle::Package& mPackage;
  ConverterDb&              mDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif  // LIBREPCB_EAGLEIMPORT_PACKAGECONVERTER_H

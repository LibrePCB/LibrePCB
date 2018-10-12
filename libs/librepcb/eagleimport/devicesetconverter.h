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

#ifndef LIBREPCB_EAGLEIMPORT_DEVICESETCONVERTER_H
#define LIBREPCB_EAGLEIMPORT_DEVICESETCONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace parseagle {
class DeviceSet;
}

namespace librepcb {

namespace library {
class Component;
}

namespace eagleimport {

class ConverterDb;

/*******************************************************************************
 *  Class DeviceSetConverter
 ******************************************************************************/

/**
 * @brief The DeviceSetConverter class
 */
class DeviceSetConverter final {
public:
  // Constructors / Destructor
  DeviceSetConverter()                                = delete;
  DeviceSetConverter(const DeviceSetConverter& other) = delete;
  DeviceSetConverter(const parseagle::DeviceSet& deviceSet,
                     ConverterDb&                db) noexcept;
  ~DeviceSetConverter() noexcept;

  // General Methods
  std::unique_ptr<library::Component> generate() const;

  // Operator Overloadings
  DeviceSetConverter& operator=(const DeviceSetConverter& rhs) = delete;

private:
  QString createDescription() const noexcept;

  const parseagle::DeviceSet& mDeviceSet;
  ConverterDb&                mDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif  // LIBREPCB_EAGLEIMPORT_DEVICESETCONVERTER_H

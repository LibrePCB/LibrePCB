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

#ifndef LIBREPCB_EAGLEIMPORT_DEVICECONVERTER_H
#define LIBREPCB_EAGLEIMPORT_DEVICECONVERTER_H

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
class Device;
}  // namespace parseagle

namespace librepcb {

namespace library {
class Device;
}

namespace eagleimport {

class ConverterDb;

/*******************************************************************************
 *  Class DeviceConverter
 ******************************************************************************/

/**
 * @brief The DeviceConverter class
 */
class DeviceConverter final {
public:
  // Constructors / Destructor
  DeviceConverter()                             = delete;
  DeviceConverter(const DeviceConverter& other) = delete;
  DeviceConverter(const parseagle::DeviceSet& deviceSet,
                  const parseagle::Device& device, ConverterDb& db) noexcept;
  ~DeviceConverter() noexcept;

  // General Methods
  std::unique_ptr<library::Device> generate() const;

  // Operator Overloadings
  DeviceConverter& operator=(const DeviceConverter& rhs) = delete;

private:
  QString createDescription() const noexcept;

  const parseagle::DeviceSet& mDeviceSet;
  const parseagle::Device&    mDevice;
  ConverterDb&                mDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif  // LIBREPCB_EAGLEIMPORT_DEVICECONVERTER_H

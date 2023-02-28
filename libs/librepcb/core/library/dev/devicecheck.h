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

#ifndef LIBREPCB_CORE_DEVICECHECK_H
#define LIBREPCB_CORE_DEVICECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Device;

/*******************************************************************************
 *  Class DeviceCheck
 ******************************************************************************/

/**
 * @brief The DeviceCheck class
 */
class DeviceCheck : public LibraryElementCheck {
public:
  // Constructors / Destructor
  DeviceCheck() = delete;
  DeviceCheck(const DeviceCheck& other) = delete;
  explicit DeviceCheck(const Device& device) noexcept;
  virtual ~DeviceCheck() noexcept;

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  DeviceCheck& operator=(const DeviceCheck& rhs) = delete;

protected:  // Methods
  void checkNoPadsConnected(MsgList& msgs) const;

private:  // Data
  const Device& mDevice;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_CORE_MSGNOPADSINDEVICECONNECTED_H
#define LIBREPCB_CORE_MSGNOPADSINDEVICECONNECTED_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../rulecheck/rulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MsgNoPadsInDeviceConnected
 ******************************************************************************/

/**
 * @brief The MsgNoPadsInDeviceConnected class
 */
class MsgNoPadsInDeviceConnected final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgNoPadsInDeviceConnected)

public:
  // Constructors / Destructor
  MsgNoPadsInDeviceConnected() noexcept;
  MsgNoPadsInDeviceConnected(const MsgNoPadsInDeviceConnected& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgNoPadsInDeviceConnected() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

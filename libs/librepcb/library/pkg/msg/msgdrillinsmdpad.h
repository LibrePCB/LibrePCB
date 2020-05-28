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

#ifndef LIBREPCB_LIBRARY_MSGDRILLINSMDPAD_H
#define LIBREPCB_LIBRARY_MSGDRILLINSMDPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../msg/libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class FootprintPad;

/*******************************************************************************
 *  Class MsgDrillInSmdPad
 ******************************************************************************/

/**
 * @brief The MsgDrillInSmdPad class
 */
class MsgDrillInSmdPad final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgDrillInSmdPad)

public:
  // Constructors / Destructor
  MsgDrillInSmdPad() = delete;
  explicit MsgDrillInSmdPad(const FootprintPad& pad) noexcept;
  MsgDrillInSmdPad(const MsgDrillInSmdPad& other) noexcept
    : LibraryElementCheckMessage(other) {}
  virtual ~MsgDrillInSmdPad() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_MSGDRILLINSMDPAD_H

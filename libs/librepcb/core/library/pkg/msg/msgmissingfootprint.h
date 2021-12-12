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

#ifndef LIBREPCB_CORE_MSGMISSINGFOOTPRINT_H
#define LIBREPCB_CORE_MSGMISSINGFOOTPRINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../msg/libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MsgMissingFootprint
 ******************************************************************************/

/**
 * @brief The MsgMissingFootprint class
 */
class MsgMissingFootprint final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingFootprint)

public:
  // Constructors / Destructor
  MsgMissingFootprint() noexcept;
  MsgMissingFootprint(const MsgMissingFootprint& other) noexcept
    : LibraryElementCheckMessage(other) {}
  virtual ~MsgMissingFootprint() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

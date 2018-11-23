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

#ifndef LIBREPCB_LIBRARY_MSGMISSINGAUTHOR_H
#define LIBREPCB_LIBRARY_MSGMISSINGAUTHOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class MsgMissingAuthor
 ******************************************************************************/

/**
 * @brief The MsgMissingAuthor class
 */
class MsgMissingAuthor final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingAuthor)

public:
  // Constructors / Destructor
  MsgMissingAuthor() noexcept;
  MsgMissingAuthor(const MsgMissingAuthor& other) noexcept
    : LibraryElementCheckMessage(other) {}
  virtual ~MsgMissingAuthor() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_MSGMISSINGAUTHOR_H

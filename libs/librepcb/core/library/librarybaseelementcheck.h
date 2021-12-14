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

#ifndef LIBREPCB_LIBRARY_LIBRARYBASEELEMENTCHECK_H
#define LIBREPCB_LIBRARY_LIBRARYBASEELEMENTCHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "./msg/libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class LibraryBaseElement;

/*******************************************************************************
 *  Class LibraryBaseElementCheck
 ******************************************************************************/

/**
 * @brief The LibraryBaseElementCheck class
 */
class LibraryBaseElementCheck {
public:
  // Constructors / Destructor
  LibraryBaseElementCheck() = delete;
  LibraryBaseElementCheck(const LibraryBaseElementCheck& other) = delete;
  explicit LibraryBaseElementCheck(const LibraryBaseElement& element) noexcept;
  virtual ~LibraryBaseElementCheck() noexcept;

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const;

  // Operator Overloadings
  LibraryBaseElementCheck& operator=(const LibraryBaseElementCheck& rhs) =
      delete;

protected:
  typedef LibraryElementCheckMessageList MsgList;
  void checkDefaultNameTitleCase(MsgList& msgs) const;
  void checkMissingAuthor(MsgList& msgs) const;

private:  // Data
  const LibraryBaseElement& mElement;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif

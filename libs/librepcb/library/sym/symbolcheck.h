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

#ifndef LIBREPCB_LIBRARY_SYMBOLCHECK_H
#define LIBREPCB_LIBRARY_SYMBOLCHECK_H

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

class Symbol;

/*******************************************************************************
 *  Class SymbolCheck
 ******************************************************************************/

/**
 * @brief The SymbolCheck class
 */
class SymbolCheck : public LibraryElementCheck {
public:
  // Constructors / Destructor
  SymbolCheck() = delete;
  SymbolCheck(const SymbolCheck& other) = delete;
  explicit SymbolCheck(const Symbol& symbol) noexcept;
  virtual ~SymbolCheck() noexcept;

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;

  // Operator Overloadings
  SymbolCheck& operator=(const SymbolCheck& rhs) = delete;

protected:  // Methods
  void checkDuplicatePinNames(MsgList& msgs) const;
  void checkOffTheGridPins(MsgList& msgs) const;
  void checkOverlappingPins(MsgList& msgs) const;
  void checkMissingTexts(MsgList& msgs) const;
  void checkWrongTextLayers(MsgList& msgs) const;

private:  // Data
  const Symbol& mSymbol;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_SYMBOLCHECK_H

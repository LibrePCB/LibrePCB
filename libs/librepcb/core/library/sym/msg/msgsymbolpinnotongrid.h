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

#ifndef LIBREPCB_CORE_MSGSYMBOLPINNOTONGRID_H
#define LIBREPCB_CORE_MSGSYMBOLPINNOTONGRID_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../rulecheck/rulecheckmessage.h"
#include "../../../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SymbolPin;

/*******************************************************************************
 *  Class MsgSymbolPinNotOnGrid
 ******************************************************************************/

/**
 * @brief The MsgSymbolPinNotOnGrid class
 */
class MsgSymbolPinNotOnGrid final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgSymbolPinNotOnGrid)

public:
  // Constructors / Destructor
  MsgSymbolPinNotOnGrid() = delete;
  MsgSymbolPinNotOnGrid(std::shared_ptr<const SymbolPin> pin,
                        const PositiveLength& gridInterval) noexcept;
  MsgSymbolPinNotOnGrid(const MsgSymbolPinNotOnGrid& other) noexcept
    : RuleCheckMessage(other),
      mPin(other.mPin),
      mGridInterval(other.mGridInterval) {}
  virtual ~MsgSymbolPinNotOnGrid() noexcept;

  // Getters
  const std::shared_ptr<const SymbolPin>& getPin() const noexcept {
    return mPin;
  }
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }

private:
  std::shared_ptr<const SymbolPin> mPin;
  PositiveLength mGridInterval;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

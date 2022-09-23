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

#ifndef LIBREPCB_CORE_MSGOVERLAPPINGSYMBOLPINS_H
#define LIBREPCB_CORE_MSGOVERLAPPINGSYMBOLPINS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/length.h"
#include "../../msg/libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SymbolPin;

/*******************************************************************************
 *  Class MsgOverlappingSymbolPins
 ******************************************************************************/

/**
 * @brief The MsgOverlappingSymbolPins class
 */
class MsgOverlappingSymbolPins final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgOverlappingSymbolPins)

public:
  // Constructors / Destructor
  MsgOverlappingSymbolPins() = delete;
  MsgOverlappingSymbolPins(
      QVector<std::shared_ptr<const SymbolPin>> pins) noexcept;
  MsgOverlappingSymbolPins(const MsgOverlappingSymbolPins& other) noexcept
    : LibraryElementCheckMessage(other), mPins(other.mPins) {}
  virtual ~MsgOverlappingSymbolPins() noexcept;

  // Getters
  const QVector<std::shared_ptr<const SymbolPin>>& getPins() const noexcept {
    return mPins;
  }

private:
  static QString buildMessage(
      const QVector<std::shared_ptr<const SymbolPin>>& pins) noexcept;

private:  // Data
  QVector<std::shared_ptr<const SymbolPin>> mPins;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

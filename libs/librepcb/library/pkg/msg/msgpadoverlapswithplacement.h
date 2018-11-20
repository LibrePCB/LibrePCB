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

#ifndef LIBREPCB_LIBRARY_MSGPADOVERLAPSWITHPLACEMENT_H
#define LIBREPCB_LIBRARY_MSGPADOVERLAPSWITHPLACEMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../msg/libraryelementcheckmessage.h"

#include <librepcb/common/units/length.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class Footprint;
class FootprintPad;

/*******************************************************************************
 *  Class MsgPadOverlapsWithPlacement
 ******************************************************************************/

/**
 * @brief The MsgPadOverlapsWithPlacement class
 */
class MsgPadOverlapsWithPlacement final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadOverlapsWithPlacement)

public:
  // Constructors / Destructor
  MsgPadOverlapsWithPlacement() = delete;
  MsgPadOverlapsWithPlacement(std::shared_ptr<const Footprint>    footprint,
                              std::shared_ptr<const FootprintPad> pad,
                              const QString&                      pkgPadName,
                              const Length& clearance) noexcept;
  MsgPadOverlapsWithPlacement(const MsgPadOverlapsWithPlacement& other) noexcept
    : LibraryElementCheckMessage(other),
      mFootprint(other.mFootprint),
      mPad(other.mPad) {}
  virtual ~MsgPadOverlapsWithPlacement() noexcept;

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint>    mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_MSGPADOVERLAPSWITHPLACEMENT_H

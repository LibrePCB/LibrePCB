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

#ifndef LIBREPCB_CORE_MSGPADCLEARANCEVIOLATION_H
#define LIBREPCB_CORE_MSGPADCLEARANCEVIOLATION_H

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

class Footprint;
class FootprintPad;

/*******************************************************************************
 *  Class MsgPadClearanceViolation
 ******************************************************************************/

/**
 * @brief The MsgPadClearanceViolation class
 */
class MsgPadClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadClearanceViolation)

public:
  // Constructors / Destructor
  MsgPadClearanceViolation() = delete;
  MsgPadClearanceViolation(std::shared_ptr<const Footprint> footprint,
                           std::shared_ptr<const FootprintPad> pad1,
                           const QString& pkgPad1Name,
                           std::shared_ptr<const FootprintPad> pad2,
                           const QString& pkgPad2Name,
                           const Length& clearance) noexcept;
  MsgPadClearanceViolation(const MsgPadClearanceViolation& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mPad1(other.mPad1),
      mPad2(other.mPad2) {}
  virtual ~MsgPadClearanceViolation() noexcept;

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad1() const noexcept { return mPad1; }
  std::shared_ptr<const FootprintPad> getPad2() const noexcept { return mPad2; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad1;
  std::shared_ptr<const FootprintPad> mPad2;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

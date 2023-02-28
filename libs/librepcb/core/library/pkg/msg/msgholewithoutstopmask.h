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

#ifndef LIBREPCB_CORE_MSGHOLEWITHOUTSTOPMASK_H
#define LIBREPCB_CORE_MSGHOLEWITHOUTSTOPMASK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../rulecheck/rulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Footprint;
class Hole;

/*******************************************************************************
 *  Class MsgHoleWithoutStopMask
 ******************************************************************************/

/**
 * @brief The MsgHoleWithoutStopMask class
 */
class MsgHoleWithoutStopMask final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgHoleWithoutStopMask)

public:
  // Constructors / Destructor
  MsgHoleWithoutStopMask() = delete;
  MsgHoleWithoutStopMask(std::shared_ptr<const Footprint> footprint,
                         std::shared_ptr<const Hole> hole) noexcept;
  MsgHoleWithoutStopMask(const MsgHoleWithoutStopMask& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mHole(other.mHole) {}
  virtual ~MsgHoleWithoutStopMask() noexcept;

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const Hole> getHole() const noexcept { return mHole; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const Hole> mHole;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

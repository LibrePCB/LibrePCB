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

#ifndef LIBREPCB_CORE_BI_HOLE_H
#define LIBREPCB_CORE_BI_HOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../utils/signalslot.h"
#include "../boardholedata.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

/*******************************************************************************
 *  Class BI_Hole
 ******************************************************************************/

/**
 * @brief The BI_Hole class
 */
class BI_Hole final : public BI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    DiameterChanged,
    PathChanged,
    StopMaskOffsetChanged,
  };
  Signal<BI_Hole, Event> onEdited;
  typedef Slot<BI_Hole, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_Hole() = delete;
  BI_Hole(const BI_Hole& other) = delete;
  BI_Hole(Board& board, const BoardHoleData& data);
  ~BI_Hole() noexcept;

  // Getters
  const BoardHoleData& getData() const noexcept { return mData; }
  const tl::optional<Length>& getStopMaskOffset() const noexcept {
    return mStopMaskOffset;
  }

  // Setters
  bool setDiameter(const PositiveLength& diameter) noexcept;
  bool setPath(const NonEmptyPath& path) noexcept;
  bool setStopMaskConfig(const MaskConfig& config) noexcept;
  bool setLocked(bool locked) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_Hole& operator=(const BI_Hole& rhs) = delete;

private:  // Methods
  void updateStopMaskOffset() noexcept;

private:  // Data
  BoardHoleData mData;

  // Cached Attributes
  tl::optional<Length> mStopMaskOffset;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

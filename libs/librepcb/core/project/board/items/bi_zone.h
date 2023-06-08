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

#ifndef LIBREPCB_CORE_BI_ZONE_H
#define LIBREPCB_CORE_BI_ZONE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../utils/signalslot.h"
#include "../boardzonedata.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

/*******************************************************************************
 *  Class BI_Zone
 ******************************************************************************/

/**
 * @brief The BI_Zone class
 */
class BI_Zone final : public BI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    BoardLayersChanged,
    LayersChanged,
    RulesChanged,
    OutlineChanged,
    IsLockedChanged,
  };
  Signal<BI_Zone, Event> onEdited;
  typedef Slot<BI_Zone, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_Zone() = delete;
  BI_Zone(const BI_Zone& other) = delete;
  BI_Zone(Board& board, const BoardZoneData& data);
  ~BI_Zone() noexcept;

  // Getters
  const BoardZoneData& getData() const noexcept { return mData; }

  // Setters
  bool setLayers(const QSet<const Layer*>& layers);
  bool setRules(Zone::Rules rules) noexcept;
  bool setOutline(const Path& outline) noexcept;
  bool setLocked(bool locked) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_Zone& operator=(const BI_Zone& rhs) = delete;

private:  // Data
  BoardZoneData mData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

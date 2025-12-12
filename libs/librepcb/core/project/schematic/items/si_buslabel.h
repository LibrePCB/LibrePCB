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

#ifndef LIBREPCB_CORE_SI_BUSLABEL_H
#define LIBREPCB_CORE_SI_BUSLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/netlabel.h"
#include "../../../utils/signalslot.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class SI_BusSegment;
class Schematic;

/*******************************************************************************
 *  Class SI_BusLabel
 ******************************************************************************/

/**
 * @brief The SI_BusLabel class
 */
class SI_BusLabel final : public SI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    BusNameChanged,
    AnchorPositionChanged,
  };
  Signal<SI_BusLabel, Event> onEdited;
  typedef Slot<SI_BusLabel, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_BusLabel() = delete;
  SI_BusLabel(const SI_BusLabel& other) = delete;
  explicit SI_BusLabel(SI_BusSegment& segment, const NetLabel& label);
  ~SI_BusLabel() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mNetLabel.getUuid(); }
  const Point& getPosition() const noexcept { return mNetLabel.getPosition(); }
  const Angle& getRotation() const noexcept { return mNetLabel.getRotation(); }
  bool getMirrored() const noexcept { return mNetLabel.getMirrored(); }
  const Point& getAnchorPosition() const noexcept { return mAnchorPosition; }
  const NetLabel& getNetLabel() const noexcept { return mNetLabel; }
  SI_BusSegment& getBusSegment() const noexcept { return mSegment; }

  // Setters
  void setPosition(const Point& position) noexcept;
  void setRotation(const Angle& rotation) noexcept;
  void setMirrored(const bool mirrored) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void updateAnchor() noexcept;

  // Operator Overloadings
  SI_BusLabel& operator=(const SI_BusLabel& rhs) = delete;

private:
  // General
  QMetaObject::Connection mNameChangedConnection;

  // Attributes
  SI_BusSegment& mSegment;
  NetLabel mNetLabel;

  // Cached Attributes
  Point mAnchorPosition;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

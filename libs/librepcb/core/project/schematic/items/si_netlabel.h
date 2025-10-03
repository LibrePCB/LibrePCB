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

#ifndef LIBREPCB_CORE_SI_NETLABEL_H
#define LIBREPCB_CORE_SI_NETLABEL_H

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
class NetSignal;
class SI_NetSegment;
class Schematic;

/*******************************************************************************
 *  Class SI_NetLabel
 ******************************************************************************/

/**
 * @brief The SI_NetLabel class
 */
class SI_NetLabel final : public SI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    NetNameChanged,
    AnchorPositionChanged,
  };
  Signal<SI_NetLabel, Event> onEdited;
  typedef Slot<SI_NetLabel, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_NetLabel() = delete;
  SI_NetLabel(const SI_NetLabel& other) = delete;
  explicit SI_NetLabel(SI_NetSegment& segment, const NetLabel& label);
  ~SI_NetLabel() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mNetLabel.getUuid(); }
  const Point& getPosition() const noexcept { return mNetLabel.getPosition(); }
  const Angle& getRotation() const noexcept { return mNetLabel.getRotation(); }
  bool getMirrored() const noexcept { return mNetLabel.getMirrored(); }
  const Point& getAnchorPosition() const noexcept { return mAnchorPosition; }
  const NetLabel& getNetLabel() const noexcept { return mNetLabel; }
  SI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  NetSignal& getNetSignalOfNetSegment() const noexcept;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setRotation(const Angle& rotation) noexcept;
  void setMirrored(const bool mirrored) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void updateAnchor() noexcept;

  // Operator Overloadings
  SI_NetLabel& operator=(const SI_NetLabel& rhs) = delete;

private:
  // General
  QMetaObject::Connection mNameChangedConnection;

  // Attributes
  SI_NetSegment& mNetSegment;
  NetLabel mNetLabel;

  // Cached Attributes
  Point mAnchorPosition;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

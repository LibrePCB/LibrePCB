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

#ifndef LIBREPCB_CORE_SI_BUSLINE_H
#define LIBREPCB_CORE_SI_BUSLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/netline.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_BusJunction;
class SI_BusLine;
class SI_BusSegment;

/*******************************************************************************
 *  Class SI_BusLine
 ******************************************************************************/

/**
 * @brief The SI_BusLine class
 */
class SI_BusLine final : public SI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionsChanged,
    BusNameChanged,
  };
  Signal<SI_BusLine, Event> onEdited;
  typedef Slot<SI_BusLine, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_BusLine() = delete;
  SI_BusLine(const SI_BusLine& other) = delete;
  SI_BusLine(SI_BusSegment& segment, const Uuid& uuid, SI_BusJunction& a,
             SI_BusJunction& b, const UnsignedLength& width);
  ~SI_BusLine() noexcept;

  // Getters
  SI_BusSegment& getBusSegment() const noexcept { return mSegment; }
  const NetLine& getNetLine() const noexcept { return mNetLine; }
  const Uuid& getUuid() const noexcept { return mNetLine.getUuid(); }
  const UnsignedLength& getWidth() const noexcept {
    return mNetLine.getWidth();
  }
  SI_BusJunction& getP1() const noexcept { return *mP1; }
  SI_BusJunction& getP2() const noexcept { return *mP2; }
  SI_BusJunction* getOtherPoint(
      const SI_BusJunction& firstPoint) const noexcept;

  // Setters
  void setWidth(const UnsignedLength& width) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void updatePositions() noexcept;

  // Operator Overloadings
  SI_BusLine& operator=(const SI_BusLine& rhs) = delete;

private:  // Data
  SI_BusSegment& mSegment;
  NetLine mNetLine;

  // References
  SI_BusJunction* mP1;
  SI_BusJunction* mP2;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

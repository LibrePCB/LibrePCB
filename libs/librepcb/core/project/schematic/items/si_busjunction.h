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

#ifndef LIBREPCB_CORE_SI_BUSJUNCTION_H
#define LIBREPCB_CORE_SI_BUSJUNCTION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/junction.h"
#include "./si_busline.h"
#include "./si_netline.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SI_BusJunction
 ******************************************************************************/

/**
 * @brief The SI_BusJunction class
 */
class SI_BusJunction final : public SI_Base, public SI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    JunctionChanged,
    BusNameChanged,
  };
  Signal<SI_BusJunction, Event> onEdited;
  typedef Slot<SI_BusJunction, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_BusJunction() = delete;
  SI_BusJunction(const SI_BusJunction& other) = delete;
  SI_BusJunction(SI_BusSegment& segment, const Uuid& uuid,
                 const Point& position);
  ~SI_BusJunction() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mJunction.getUuid(); }
  const Point& getPosition() const noexcept override {
    return mJunction.getPosition();
  }
  const Junction& getJunction() const noexcept { return mJunction; }
  bool isVisibleJunction() const noexcept;
  SI_BusSegment& getBusSegment() const noexcept { return mSegment; }
  bool isUsed() const noexcept;
  bool isOpen() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void registerBusLine(SI_BusLine& l);
  void unregisterBusLine(SI_BusLine& l);
  const QSet<SI_BusLine*>& getBusLines() const noexcept {
    return mRegisteredBusLines;
  }

  // Inherited from SI_NetLineAnchor
  void registerNetLine(SI_NetLine& netline) override;
  void unregisterNetLine(SI_NetLine& netline) override;
  const QSet<SI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }
  NetLineAnchor toNetLineAnchor() const noexcept override;

  // Operator Overloadings
  SI_BusJunction& operator=(const SI_BusJunction& rhs) = delete;
  bool operator==(const SI_BusJunction& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const SI_BusJunction& rhs) noexcept { return (this != &rhs); }

private:
  // Attributes
  SI_BusSegment& mSegment;
  Junction mJunction;

  // Registered Elements
  QSet<SI_BusLine*> mRegisteredBusLines;  ///< all registered netlines
  QSet<SI_NetLine*> mRegisteredNetLines;  ///< all registered netlines
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

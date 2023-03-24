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

#ifndef LIBREPCB_CORE_SI_NETPOINT_H
#define LIBREPCB_CORE_SI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/junction.h"
#include "./si_netline.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SI_NetPoint
 ******************************************************************************/

/**
 * @brief The SI_NetPoint class
 */
class SI_NetPoint final : public SI_Base, public SI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    JunctionChanged,
    NetSignalNameChanged,
  };
  Signal<SI_NetPoint, Event> onEdited;
  typedef Slot<SI_NetPoint, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_NetPoint() = delete;
  SI_NetPoint(const SI_NetPoint& other) = delete;
  SI_NetPoint(SI_NetSegment& segment, const Uuid& uuid, const Point& position);
  ~SI_NetPoint() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mJunction.getUuid(); }
  const Point& getPosition() const noexcept override {
    return mJunction.getPosition();
  }
  const Junction& getJunction() const noexcept { return mJunction; }
  bool isVisibleJunction() const noexcept;
  bool isOpenLineEnd() const noexcept;
  SI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  NetSignal& getNetSignalOfNetSegment() const noexcept;
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  NetLineAnchor toNetLineAnchor() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Inherited from SI_NetLineAnchor
  void registerNetLine(SI_NetLine& netline) override;
  void unregisterNetLine(SI_NetLine& netline) override;
  const QSet<SI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }

  // Operator Overloadings
  SI_NetPoint& operator=(const SI_NetPoint& rhs) = delete;
  bool operator==(const SI_NetPoint& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const SI_NetPoint& rhs) noexcept { return (this != &rhs); }

private:
  // Attributes
  SI_NetSegment& mNetSegment;
  Junction mJunction;

  // Registered Elements
  QSet<SI_NetLine*> mRegisteredNetLines;  ///< all registered netlines
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

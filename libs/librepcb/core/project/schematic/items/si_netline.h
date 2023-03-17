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

#ifndef LIBREPCB_CORE_SI_NETLINE_H
#define LIBREPCB_CORE_SI_NETLINE_H

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

class NetSignal;
class SI_NetLine;
class SI_NetSegment;

/*******************************************************************************
 *  Class SI_NetLineAnchor
 ******************************************************************************/

class SI_NetLineAnchor {
public:
  SI_NetLineAnchor() noexcept = default;
  virtual ~SI_NetLineAnchor() noexcept = default;

  virtual void registerNetLine(SI_NetLine& netline) = 0;
  virtual void unregisterNetLine(SI_NetLine& netline) = 0;
  virtual const QSet<SI_NetLine*>& getNetLines() const noexcept = 0;
  virtual const Point& getPosition() const noexcept = 0;

  virtual NetLineAnchor toNetLineAnchor() const noexcept = 0;
};

/*******************************************************************************
 *  Class SI_NetLine
 ******************************************************************************/

/**
 * @brief The SI_NetLine class
 */
class SI_NetLine final : public SI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionsChanged,
    NetSignalNameChanged,
  };
  Signal<SI_NetLine, Event> onEdited;
  typedef Slot<SI_NetLine, Event> OnEditedSlot;

  // Constructors / Destructor
  SI_NetLine() = delete;
  SI_NetLine(const SI_NetLine& other) = delete;
  SI_NetLine(SI_NetSegment& segment, const Uuid& uuid,
             SI_NetLineAnchor& startPoint, SI_NetLineAnchor& endPoint,
             const UnsignedLength& width);
  ~SI_NetLine() noexcept;

  // Getters
  SI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  const NetLine& getNetLine() const noexcept { return mNetLine; }
  const Uuid& getUuid() const noexcept { return mNetLine.getUuid(); }
  const UnsignedLength& getWidth() const noexcept {
    return mNetLine.getWidth();
  }
  SI_NetLineAnchor& getStartPoint() const noexcept { return *mStartPoint; }
  SI_NetLineAnchor& getEndPoint() const noexcept { return *mEndPoint; }
  SI_NetLineAnchor* getOtherPoint(const SI_NetLineAnchor& firstPoint) const
      noexcept;
  NetSignal& getNetSignalOfNetSegment() const noexcept;

  // Setters
  void setWidth(const UnsignedLength& width) noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void updatePositions() noexcept;

  // Operator Overloadings
  SI_NetLine& operator=(const SI_NetLine& rhs) = delete;

private:  // Data
  SI_NetSegment& mNetSegment;
  NetLine mNetLine;

  // References
  SI_NetLineAnchor* mStartPoint;
  SI_NetLineAnchor* mEndPoint;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

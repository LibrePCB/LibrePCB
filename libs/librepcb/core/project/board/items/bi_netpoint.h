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

#ifndef LIBREPCB_CORE_BI_NETPOINT_H
#define LIBREPCB_CORE_BI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/junction.h"
#include "bi_base.h"
#include "bi_netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class BI_NetPoint
 ******************************************************************************/

/**
 * @brief The BI_NetPoint class
 */
class BI_NetPoint final : public BI_Base, public BI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    LayerOfTracesChanged,
    MaxTraceWidthChanged,
    NetSignalNameChanged,
  };
  Signal<BI_NetPoint, Event> onEdited;
  typedef Slot<BI_NetPoint, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_NetPoint() = delete;
  BI_NetPoint(const BI_NetPoint& other) = delete;
  BI_NetPoint(BI_NetSegment& segment, const Uuid& uuid, const Point& position);
  ~BI_NetPoint() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mJunction.getUuid(); }
  const Point& getPosition() const noexcept override {
    return mJunction.getPosition();
  }
  const Junction& getJunction() const noexcept { return mJunction; }
  BI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  const Layer* getLayerOfTraces() const noexcept { return mLayerOfTraces; }
  const UnsignedLength& getMaxTraceWidth() const noexcept {
    return mMaxTraceWidth;
  }
  TraceAnchor toTraceAnchor() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  // Inherited from BI_NetLineAnchor
  void registerNetLine(BI_NetLine& netline) override;
  void unregisterNetLine(BI_NetLine& netline) override;
  const QSet<BI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }

  // Operator Overloadings
  BI_NetPoint& operator=(const BI_NetPoint& rhs) = delete;
  bool operator==(const BI_NetPoint& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_NetPoint& rhs) noexcept { return (this != &rhs); }

private:  // Methods
  void netLineEdited(const BI_NetLine& obj, BI_NetLine::Event event) noexcept;
  void updateLayerOfTraces() noexcept;
  void updateMaxTraceWidth() noexcept;

private:  // Data
  BI_NetSegment& mNetSegment;
  Junction mJunction;
  QMetaObject::Connection mNetSignalNameChangedConnection;

  // Cached Attributes
  const Layer* mLayerOfTraces;
  UnsignedLength mMaxTraceWidth;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;  ///< all registered netlines

  // Slots
  BI_NetLine::OnEditedSlot mOnNetLineEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

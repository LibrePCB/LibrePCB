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

#ifndef LIBREPCB_CORE_BI_VIA_H
#define LIBREPCB_CORE_BI_VIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/via.h"
#include "./bi_netline.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BI_Via
 ******************************************************************************/

/**
 * @brief The BI_Via class
 */
class BI_Via final : public BI_Base, public BI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    LayersChanged,
    PositionChanged,
    SizeChanged,
    DrillDiameterChanged,
    NetSignalNameChanged,
    StopMaskDiametersChanged,
  };
  Signal<BI_Via, Event> onEdited;
  typedef Slot<BI_Via, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_Via() = delete;
  BI_Via(const BI_Via& other) = delete;
  BI_Via(BI_NetSegment& netsegment, const Via& via);
  ~BI_Via() noexcept;

  // Getters
  BI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  const Point& getPosition() const noexcept override {
    return mVia.getPosition();
  }
  const Via& getVia() const noexcept { return mVia; }
  const Uuid& getUuid() const noexcept { return mVia.getUuid(); }
  const PositiveLength& getDrillDiameter() const noexcept {
    return mVia.getDrillDiameter();
  }
  const PositiveLength& getSize() const noexcept { return mVia.getSize(); }
  const tl::optional<PositiveLength>& getStopMaskDiameterTop() const noexcept {
    return mStopMaskDiameterTop;
  }
  const tl::optional<PositiveLength>& getStopMaskDiameterBottom() const
      noexcept {
    return mStopMaskDiameterBottom;
  }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  QSet<const Layer*> getCopperLayers() const noexcept;
  tl::optional<std::pair<const Layer*, const Layer*> > getDrillLayerSpan() const
      noexcept;
  TraceAnchor toTraceAnchor() const noexcept override;

  // Setters
  void setLayers(const Layer& from, const Layer& to);
  void setPosition(const Point& position) noexcept;
  void setSize(const PositiveLength& size) noexcept;
  void setDrillDiameter(const PositiveLength& diameter) noexcept;
  void setExposureConfig(const MaskConfig& config) noexcept;

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
  BI_Via& operator=(const BI_Via& rhs) = delete;
  bool operator==(const BI_Via& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_Via& rhs) noexcept { return (this != &rhs); }

private:  // Methods
  void updateStopMaskDiameters() noexcept;

private:  // Data
  Via mVia;
  BI_NetSegment& mNetSegment;
  QMetaObject::Connection mNetSignalNameChangedConnection;

  // Cached Attributes
  tl::optional<PositiveLength> mStopMaskDiameterTop;
  tl::optional<PositiveLength> mStopMaskDiameterBottom;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

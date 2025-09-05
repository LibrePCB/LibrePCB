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

#ifndef LIBREPCB_CORE_BI_PAD_H
#define LIBREPCB_CORE_BI_PAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/path.h"
#include "../boardpaddata.h"
#include "bi_device.h"
#include "bi_netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSignalInstance;
class FootprintPad;
class PackagePad;

/*******************************************************************************
 *  Class BI_Pad
 ******************************************************************************/

/**
 * @brief A pad in a board (either standalone or from a footprint)
 */
class BI_Pad final : public BI_Base, public BI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    // Common pad events
    UuidChanged,
    PositionChanged,
    RotationChanged,
    ShapeChanged,
    WidthChanged,
    HeightChanged,
    RadiusChanged,
    CustomShapeOutlineChanged,
    StopMaskConfigChanged,
    SolderPasteConfigChanged,
    CopperClearanceChanged,
    ComponentSideChanged,
    FunctionChanged,
    HolesEdited,
    // Specific events
    MirroredChanged,
    LockedChanged,
    TextChanged,
    GeometriesChanged,
  };
  Signal<BI_Pad, Event> onEdited;
  typedef Slot<BI_Pad, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_Pad() = delete;
  BI_Pad(const BI_Pad& other) = delete;
  /**
   * @brief Construct a board pad (not related to a footprint)
   *
   * @attention This kind of pad needs to be added to the board with
   *            ::librepcb::BI_NetSegment::addElements().
   *
   * @param netsegment  Net segment of the pad.
   * @param properties  Pad properties.
   */
  BI_Pad(BI_NetSegment& netsegment, const BoardPadData& properties);
  /**
   * @brief Construct a footprint pad
   *
   * @attention This kind of pad is added to the board exclusively from
   *            the constructor ::librepcb::BI_Device::BI_Device().
   *
   * @param device    The device of the pad's footprint.
   * @param padUuid   Footprint pad UUID.
   */
  BI_Pad(BI_Device& device, const Uuid& padUuid);
  ~BI_Pad() noexcept;

  // Getters
  BI_NetSegment* getNetSegment() const noexcept { return mNetSegment; }
  BI_Device* getDevice() const noexcept { return mDevice; }
  const PackagePad* getLibPackagePad() const noexcept { return mPackagePad; }
  ComponentSignalInstance* getComponentSignalInstance() const noexcept {
    return mComponentSignalInstance;
  }
  NetSignal* getNetSignal() const noexcept;
  const BoardPadData& getProperties() const noexcept { return mProperties; }
  const Uuid& getUuid() const noexcept { return mProperties.getUuid(); }

  /**
   * @brief Get the absolute position of the pad (global scene coordinates)
   *
   * @return Global pad position
   */
  const Point& getPosition() const noexcept override { return mPosition; }

  /**
   * @brief Get the absolute rotation of the pad (global scene coordinates)
   *
   * @return Global pad rotation
   */
  const Angle& getRotation() const noexcept { return mRotation; }

  /**
   * @brief Get the absolute mirror state of the pad (global scene coordinates)
   *
   * @return Global pad mirror state
   */
  bool getMirrored() const noexcept { return mMirrored; }

  Pad::ComponentSide getComponentSide() const noexcept;
  const Layer& getSolderLayer() const noexcept;
  bool isOnLayer(const Layer& layer) const noexcept;
  const QString& getText() const noexcept { return mText; }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  const QHash<const Layer*, QList<PadGeometry>>& getGeometries()
      const noexcept {
    return mGeometries;
  }
  TraceAnchor toTraceAnchor() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setRotation(const Angle& rotation) noexcept;
  void setShape(Pad::Shape shape) noexcept;
  void setWidth(const PositiveLength& width) noexcept;
  void setHeight(const PositiveLength& height) noexcept;
  void setRadius(const UnsignedLimitedRatio& radius) noexcept;
  void setCustomShapeOutline(const Path& outline) noexcept;
  void setStopMaskConfig(const MaskConfig& config) noexcept;
  void setSolderPasteConfig(const MaskConfig& config) noexcept;
  void setCopperClearance(const UnsignedLength& clearance) noexcept;
  void setComponentSideAndHoles(Pad::ComponentSide side,
                                const PadHoleList& holes);
  void setFunction(Pad::Function function) noexcept;
  void setLocked(bool locked) noexcept;

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
  BI_Pad& operator=(const BI_Pad& rhs) = delete;

private:  // Methods
  void deviceEdited(const BI_Device& obj, BI_Device::Event event) noexcept;
  void netSignalChanged(NetSignal* from, NetSignal* to);
  void updateTransform() noexcept;
  void updateText() noexcept;
  void updateGeometries() noexcept;
  void invalidatePlanes() noexcept;
  QString getLibraryDeviceName() const noexcept;
  QString getComponentInstanceName() const noexcept;
  QString getPadNameOrUuid() const noexcept;
  QString getNetSignalName() const noexcept;
  UnsignedLength getSizeForMaskOffsetCalculaton() const noexcept;
  QList<PadGeometry> getGeometryOnLayer(const Layer& layer) const noexcept;
  QList<PadGeometry> getGeometryOnCopperLayer(
      const Layer& layer) const noexcept;
  bool isConnectedOnLayer(const Layer& layer) const noexcept;

private:  // Data
  /// The net segment this pad is part of
  ///
  /// @attention  This is `nullptr if this is a footprint pad.
  BI_NetSegment* mNetSegment;

  /// The device this pad is part of
  ///
  /// @attention  This is `nullptr if this is a board pad.
  BI_Device* mDevice;

  /// The footprint pad of the device
  ///
  /// @attention  This is `nullptr` if this is a board pad!
  const FootprintPad* mFootprintPad;

  /// The package pad where this footprint pad is connected to
  ///
  /// @attention  This is `nullptr` if this is a board pad or the footprint pad
  ///             is not connected!
  const PackagePad* mPackagePad;

  /// The net signal where this footprint pad is connected to
  ///
  /// @attention  This is `nullptr` if this is a board pad or the footprint pad
  ///             is not connected!
  ComponentSignalInstance* mComponentSignalInstance;

  /// The pad's properties
  ///
  /// If this is a footprint pad, the properties will be copied from the
  /// corresponding pad from the footprint in the project library and are
  /// considered read-only.
  ///
  /// If this is a board pad, the properties are the single source of truth,
  /// and can be modified with the setters on this class.
  BoardPadData mProperties;

  // Cached Properties
  Point mPosition;
  Angle mRotation;
  bool mMirrored;
  QString mText;
  QHash<const Layer*, QList<PadGeometry>> mGeometries;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;

  // Slots
  BI_Device::OnEditedSlot mOnDeviceEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

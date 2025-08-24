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

#ifndef LIBREPCB_CORE_BI_FOOTPRINTPAD_H
#define LIBREPCB_CORE_BI_FOOTPRINTPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/path.h"
#include "../../../library/pkg/footprintpad.h"
#include "bi_device.h"
#include "bi_netline.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSignalInstance;
class PackagePad;

/*******************************************************************************
 *  Class BI_FootprintPad
 ******************************************************************************/

/**
 * @brief The BI_FootprintPad class
 */
class BI_FootprintPad final : public BI_Base, public BI_NetLineAnchor {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    TextChanged,
    GeometriesChanged,
  };
  Signal<BI_FootprintPad, Event> onEdited;
  typedef Slot<BI_FootprintPad, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_FootprintPad() = delete;
  BI_FootprintPad(const BI_FootprintPad& other) = delete;
  BI_FootprintPad(BI_Device& device, const Uuid& padUuid);
  ~BI_FootprintPad();

  // Getters

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

  const QString& getText() const noexcept { return mText; }

  const Uuid& getLibPadUuid() const noexcept;
  BI_Device& getDevice() const noexcept { return mDevice; }
  Pad::ComponentSide getComponentSide() const noexcept;
  const Layer& getSolderLayer() const noexcept;
  bool isOnLayer(const Layer& layer) const noexcept;
  const FootprintPad& getLibPad() const noexcept { return *mFootprintPad; }
  const PackagePad* getLibPackagePad() const noexcept { return mPackagePad; }
  ComponentSignalInstance* getComponentSignalInstance() const noexcept {
    return mComponentSignalInstance;
  }
  NetSignal* getCompSigInstNetSignal() const noexcept;
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  const QHash<const Layer*, QList<PadGeometry>>& getGeometries()
      const noexcept {
    return mGeometries;
  }
  TraceAnchor toTraceAnchor() const noexcept override;

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
  BI_FootprintPad& operator=(const BI_FootprintPad& rhs) = delete;

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
  BI_Device& mDevice;
  const FootprintPad* mFootprintPad;

  /// The package pad where this footprint pad is connected to
  ///
  /// @attention This is `nullptr` if the footprint pad is not connected!
  const PackagePad* mPackagePad;

  /// The net signal where this footprint pad is connected to
  ///
  /// @attention This is `nullptr` if the footprint pad is not connected!
  ComponentSignalInstance* mComponentSignalInstance;

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

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

#ifndef LIBREPCB_CORE_FOOTPRINTPAD_H
#define LIBREPCB_CORE_FOOTPRINTPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../exceptions.h"
#include "../../geometry/padgeometry.h"
#include "../../geometry/padhole.h"
#include "../../geometry/path.h"
#include "../../serialization/serializableobjectlist.h"
#include "../../types/angle.h"
#include "../../types/length.h"
#include "../../types/maskconfig.h"
#include "../../types/point.h"
#include "../../types/ratio.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class FootprintPad
 ******************************************************************************/

/**
 * @brief The FootprintPad class represents a pad of a footprint
 */
class FootprintPad final {
  Q_DECLARE_TR_FUNCTIONS(FootprintPad)

public:
  // Types
  enum class Shape {
    RoundedRect,
    RoundedOctagon,
    Custom,
  };

  enum class ComponentSide {
    Top,
    Bottom,
  };

  enum class Function {
    Unspecified = 0,
    StandardPad,
    PressFitPad,
    ThermalPad,
    BgaPad,
    EdgeConnectorPad,
    TestPad,
    LocalFiducial,
    GlobalFiducial,
    _COUNT,
  };

  // Signals
  enum class Event {
    UuidChanged,
    PackagePadUuidChanged,
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
  };
  Signal<FootprintPad, Event> onEdited;
  typedef Slot<FootprintPad, Event> OnEditedSlot;

  // Constructors / Destructor
  FootprintPad() = delete;
  FootprintPad(const FootprintPad& other) noexcept;
  FootprintPad(const Uuid& uuid, const tl::optional<Uuid>& pkgPadUuid,
               const Point& pos, const Angle& rot, Shape shape,
               const PositiveLength& width, const PositiveLength& height,
               const UnsignedLimitedRatio& radius,
               const Path& customShapeOutline, const MaskConfig& autoStopMask,
               const MaskConfig& autoSolderPaste,
               const UnsignedLength& copperClearance, ComponentSide side,
               Function function, const PadHoleList& holes) noexcept;
  explicit FootprintPad(const SExpression& node);
  ~FootprintPad() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const tl::optional<Uuid>& getPackagePadUuid() const noexcept {
    return mPackagePadUuid;
  }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  Shape getShape() const noexcept { return mShape; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  const PositiveLength& getHeight() const noexcept { return mHeight; }
  const UnsignedLimitedRatio& getRadius() const noexcept { return mRadius; }
  const Path& getCustomShapeOutline() const noexcept {
    return mCustomShapeOutline;
  }
  const MaskConfig& getStopMaskConfig() const noexcept {
    return mStopMaskConfig;
  }
  const MaskConfig& getSolderPasteConfig() const noexcept {
    return mSolderPasteConfig;
  }
  const UnsignedLength& getCopperClearance() const noexcept {
    return mCopperClearance;
  }
  ComponentSide getComponentSide() const noexcept { return mComponentSide; }
  Function getFunction() const noexcept { return mFunction; }
  bool getFunctionIsFiducial() const noexcept;
  bool getFunctionNeedsSoldering() const noexcept;
  const PadHoleList& getHoles() const noexcept { return mHoles; }
  PadHoleList& getHoles() noexcept { return mHoles; }
  bool isTht() const noexcept;
  bool isOnLayer(const Layer& layer) const noexcept;
  const Layer& getSmtLayer() const noexcept;
  bool hasTopCopper() const noexcept;
  bool hasBottomCopper() const noexcept;
  bool hasAutoTopStopMask() const noexcept;
  bool hasAutoBottomStopMask() const noexcept;
  bool hasAutoTopSolderPaste() const noexcept;
  bool hasAutoBottomSolderPaste() const noexcept;
  PadGeometry getGeometry() const noexcept;
  QHash<const Layer*, QList<PadGeometry>> buildPreviewGeometries() const
      noexcept;

  // Setters
  bool setPackagePadUuid(const tl::optional<Uuid>& pad) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rot) noexcept;
  bool setShape(Shape shape) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setRadius(const UnsignedLimitedRatio& radius) noexcept;
  bool setCustomShapeOutline(const Path& outline) noexcept;
  bool setStopMaskConfig(const MaskConfig& config) noexcept;
  bool setSolderPasteConfig(const MaskConfig& config) noexcept;
  bool setCopperClearance(const UnsignedLength& clearance) noexcept;
  bool setComponentSide(ComponentSide side) noexcept;
  bool setFunction(Function function) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const FootprintPad& rhs) const noexcept;
  bool operator!=(const FootprintPad& rhs) const noexcept {
    return !(*this == rhs);
  }
  FootprintPad& operator=(const FootprintPad& rhs) noexcept;

  // Static Methods
  static UnsignedLimitedRatio getRecommendedRadius(
      const PositiveLength& width, const PositiveLength& height) noexcept;
  static QString getFunctionDescriptionTr(Function function) noexcept;

private:  // Methods
  void holesEdited(const PadHoleList& list, int index,
                   const std::shared_ptr<const PadHole>& hole,
                   PadHoleList::Event event) noexcept;

private:  // Data
  Uuid mUuid;

  /// The connected package pad
  ///
  /// This is the UUID of the package pad where this footprint pad is
  /// connected to. It can be tl::nullopt, which means that the footprint pad
  /// is electrically not connected (e.g. for mechanical-only pads).
  tl::optional<Uuid> mPackagePadUuid;
  Point mPosition;
  Angle mRotation;
  Shape mShape;
  PositiveLength mWidth;
  PositiveLength mHeight;
  UnsignedLimitedRatio mRadius;
  Path mCustomShapeOutline;  ///< Empty if not needed; Implicitly closed
  MaskConfig mStopMaskConfig;
  MaskConfig mSolderPasteConfig;
  UnsignedLength mCopperClearance;
  ComponentSide mComponentSide;
  Function mFunction;
  PadHoleList mHoles;  ///< If not empty, it's a THT pad.

  // Slots
  PadHoleList::OnEditedSlot mHolesEditedSlot;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const FootprintPad::Function& key, uint seed = 0) noexcept {
  return ::qHash(static_cast<int>(key), seed);
}

/*******************************************************************************
 *  Class FootprintPadList
 ******************************************************************************/

struct FootprintPadListNameProvider {
  static constexpr const char* tagname = "pad";
};
using FootprintPadList =
    SerializableObjectList<FootprintPad, FootprintPadListNameProvider,
                           FootprintPad::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::FootprintPad::Function)

#endif

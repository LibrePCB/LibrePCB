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

#ifndef LIBREPCB_CORE_PAD_H
#define LIBREPCB_CORE_PAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/maskconfig.h"
#include "../types/point.h"
#include "../types/ratio.h"
#include "../types/uuid.h"
#include "padgeometry.h"
#include "padhole.h"
#include "path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class Pad
 ******************************************************************************/

/**
 * @brief Base class for pads, extended in derived classes
 */
class Pad {
  Q_DECLARE_TR_FUNCTIONS(Pad)

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

  // Constructors / Destructor
  Pad() = delete;
  Pad(const Pad& other) noexcept;
  Pad(const Uuid& uuid, const Pad& other) noexcept;
  Pad(const Uuid& uuid, const Point& pos, const Angle& rot, Shape shape,
      const PositiveLength& width, const PositiveLength& height,
      const UnsignedLimitedRatio& radius, const Path& customShapeOutline,
      const MaskConfig& autoStopMask, const MaskConfig& autoSolderPaste,
      const UnsignedLength& copperClearance, ComponentSide side,
      Function function, const PadHoleList& holes) noexcept;
  explicit Pad(const SExpression& node);
  virtual ~Pad() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
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
  QHash<const Layer*, QList<PadGeometry>> buildPreviewGeometries()
      const noexcept;

  // Operator Overloadings
  bool operator==(const Pad& rhs) const noexcept;
  bool operator!=(const Pad& rhs) const noexcept { return !(*this == rhs); }

  // Static Methods
  static UnsignedLimitedRatio getRecommendedRadius(
      const PositiveLength& width, const PositiveLength& height) noexcept;
  static QString getFunctionDescriptionTr(Function function) noexcept;

protected:  // Data
  Uuid mUuid;
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
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline std::size_t qHash(const Pad::Function& key,
                         std::size_t seed = 0) noexcept {
  return ::qHash(static_cast<int>(key), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Pad::Function)

#endif

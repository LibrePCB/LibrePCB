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
#include "../../geometry/path.h"
#include "../../serialization/serializableobjectlist.h"
#include "../../types/angle.h"
#include "../../types/length.h"
#include "../../types/point.h"
#include "../../types/uuid.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

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
  enum class Shape { ROUND, RECT, OCTAGON };
  enum class BoardSide { TOP, BOTTOM, THT };

  // Signals
  enum class Event {
    UuidChanged,
    PackagePadUuidChanged,
    PositionChanged,
    RotationChanged,
    ShapeChanged,
    WidthChanged,
    HeightChanged,
    DrillDiameterChanged,
    BoardSideChanged,
  };
  Signal<FootprintPad, Event> onEdited;
  typedef Slot<FootprintPad, Event> OnEditedSlot;

  // Constructors / Destructor
  FootprintPad() = delete;
  FootprintPad(const FootprintPad& other) noexcept;
  FootprintPad(const Uuid& uuid, const FootprintPad& other) noexcept;
  FootprintPad(const Uuid& uuid, const tl::optional<Uuid>& pkgPadUuid,
               const Point& pos, const Angle& rot, Shape shape,
               const PositiveLength& width, const PositiveLength& height,
               const UnsignedLength& drillDiameter, BoardSide side) noexcept;
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
  const UnsignedLength& getDrillDiameter() const noexcept {
    return mDrillDiameter;
  }
  BoardSide getBoardSide() const noexcept { return mBoardSide; }
  QString getLayerName() const noexcept;
  bool isOnLayer(const QString& name) const noexcept;
  Path getOutline(const Length& expansion = Length(0)) const noexcept;
  QPainterPath toQPainterPathPx(const Length& expansion = Length(0)) const
      noexcept;

  // Setters
  bool setPackagePadUuid(const tl::optional<Uuid>& pad) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rot) noexcept;
  bool setShape(Shape shape) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setDrillDiameter(const UnsignedLength& diameter) noexcept;
  bool setBoardSide(BoardSide side) noexcept;

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

protected:  // Data
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
  UnsignedLength mDrillDiameter;  // no effect if BoardSide != THT!
  BoardSide mBoardSide;
};

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

#endif

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

#ifndef LIBREPCB_CORE_VIA_H
#define LIBREPCB_CORE_VIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"
#include "../types/point.h"
#include "path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Via
 ******************************************************************************/

/**
 * @brief The Via class represents a via of a board
 *
 * The main purpose of this class is to serialize and deserialize vias
 * contained in boards.
 */
class Via final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Via)

public:
  // Signals
  enum class Event {
    UuidChanged,
    PositionChanged,
    ShapeChanged,
    SizeChanged,
    DrillDiameterChanged,
  };
  Signal<Via, Event> onEdited;
  typedef Slot<Via, Event> OnEditedSlot;

  // Public Types
  enum class Shape { Round, Square, Octagon };

  // Constructors / Destructor
  Via() = delete;
  Via(const Via& other) noexcept;
  Via(const Uuid& uuid, const Via& other) noexcept;
  Via(const Uuid& uuid, const Point& position, Shape shape,
      const PositiveLength& size, const PositiveLength& drillDiameter) noexcept;
  Via(const SExpression& node, const Version& fileFormat);
  ~Via() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Point& getPosition() const noexcept { return mPosition; }
  Shape getShape() const noexcept { return mShape; }
  const PositiveLength& getSize() const noexcept { return mSize; }
  const PositiveLength& getDrillDiameter() const noexcept {
    return mDrillDiameter;
  }
  Path getOutline(const Length& expansion = Length(0)) const noexcept;
  Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;
  QPainterPath toQPainterPathPx(const Length& expansion = Length(0)) const
      noexcept;

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setPosition(const Point& position) noexcept;
  bool setShape(Shape shape) noexcept;
  bool setSize(const PositiveLength& size) noexcept;
  bool setDrillDiameter(const PositiveLength& diameter) noexcept;

  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Via& rhs) const noexcept;
  bool operator!=(const Via& rhs) const noexcept { return !(*this == rhs); }
  Via& operator=(const Via& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  Point mPosition;
  Shape mShape;
  PositiveLength mSize;
  PositiveLength mDrillDiameter;
};

/*******************************************************************************
 *  Class ViaList
 ******************************************************************************/

struct ViaListNameProvider {
  static constexpr const char* tagname = "via";
};
using ViaList = SerializableObjectList<Via, ViaListNameProvider, Via::Event>;

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const Via::Shape& obj) {
  switch (obj) {
    case Via::Shape::Round:
      return SExpression::createToken("round");
    case Via::Shape::Square:
      return SExpression::createToken("square");
    case Via::Shape::Octagon:
      return SExpression::createToken("octagon");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline Via::Shape deserialize(const SExpression& sexpr,
                              const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  QString str = sexpr.getValue();
  if (str == "round")
    return Via::Shape::Round;
  else if (str == "square")
    return Via::Shape::Square;
  else if (str == "octagon")
    return Via::Shape::Octagon;
  else
    throw RuntimeError(__FILE__, __LINE__,
                       Via::tr("Unknown via shape: \"%1\"").arg(str));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

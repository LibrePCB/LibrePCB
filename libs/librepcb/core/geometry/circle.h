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

#ifndef LIBREPCB_CORE_CIRCLE_H
#define LIBREPCB_CORE_CIRCLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"
#include "../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class Circle
 ******************************************************************************/

/**
 * @brief The Circle class
 */
class Circle final {
  Q_DECLARE_TR_FUNCTIONS(Circle)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerChanged,
    LineWidthChanged,
    IsFilledChanged,
    IsGrabAreaChanged,
    CenterChanged,
    DiameterChanged,
  };
  Signal<Circle, Event> onEdited;
  typedef Slot<Circle, Event> OnEditedSlot;

  // Constructors / Destructor
  Circle() = delete;
  Circle(const Circle& other) noexcept;
  Circle(const Uuid& uuid, const Circle& other) noexcept;
  Circle(const Uuid& uuid, const Layer& layer, const UnsignedLength& lineWidth,
         bool fill, bool isGrabArea, const Point& center,
         const PositiveLength& diameter) noexcept;
  explicit Circle(const SExpression& node);
  ~Circle() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getLayer() const noexcept { return *mLayer; }
  const UnsignedLength& getLineWidth() const noexcept { return mLineWidth; }
  bool isFilled() const noexcept { return mIsFilled; }
  bool isGrabArea() const noexcept { return mIsGrabArea; }
  const Point& getCenter() const noexcept { return mCenter; }
  const PositiveLength& getDiameter() const noexcept { return mDiameter; }

  // Setters
  bool setLayer(const Layer& layer) noexcept;
  bool setLineWidth(const UnsignedLength& width) noexcept;
  bool setIsFilled(bool isFilled) noexcept;
  bool setIsGrabArea(bool isGrabArea) noexcept;
  bool setCenter(const Point& center) noexcept;
  bool setDiameter(const PositiveLength& dia) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Circle& rhs) const noexcept;
  bool operator!=(const Circle& rhs) const noexcept { return !(*this == rhs); }
  Circle& operator=(const Circle& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  const Layer* mLayer;
  UnsignedLength mLineWidth;
  bool mIsFilled;
  bool mIsGrabArea;
  Point mCenter;
  PositiveLength mDiameter;
};

/*******************************************************************************
 *  Class CircleList
 ******************************************************************************/

struct CircleListNameProvider {
  static constexpr const char* tagname = "circle";
};
using CircleList =
    SerializableObjectList<Circle, CircleListNameProvider, Circle::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

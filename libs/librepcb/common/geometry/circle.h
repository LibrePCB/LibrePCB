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

#ifndef LIBREPCB_CIRCLE_H
#define LIBREPCB_CIRCLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../graphics/graphicslayername.h"
#include "../units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Circle
 ******************************************************************************/

/**
 * @brief The Circle class
 */
class Circle : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Circle)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerNameChanged,
    LineWidthChanged,
    IsFilledChanged,
    IsGrabAreaChanged,
    CenterChanged,
    DiameterChanged,
  };
  Signal<Circle, Event>       onEdited;
  typedef Slot<Circle, Event> OnEditedSlot;

  // Constructors / Destructor
  Circle() = delete;
  Circle(const Circle& other) noexcept;
  Circle(const Uuid& uuid, const Circle& other) noexcept;
  Circle(const Uuid& uuid, const GraphicsLayerName& layerName,
         const UnsignedLength& lineWidth, bool fill, bool isGrabArea,
         const Point& center, const PositiveLength& diameter) noexcept;
  explicit Circle(const SExpression& node);
  virtual ~Circle() noexcept;

  // Getters
  const Uuid&              getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayerName() const noexcept { return mLayerName; }
  const UnsignedLength&    getLineWidth() const noexcept { return mLineWidth; }
  bool                     isFilled() const noexcept { return mIsFilled; }
  bool                     isGrabArea() const noexcept { return mIsGrabArea; }
  const Point&             getCenter() const noexcept { return mCenter; }
  const PositiveLength&    getDiameter() const noexcept { return mDiameter; }

  // Setters
  bool setLayerName(const GraphicsLayerName& name) noexcept;
  bool setLineWidth(const UnsignedLength& width) noexcept;
  bool setIsFilled(bool isFilled) noexcept;
  bool setIsGrabArea(bool isGrabArea) noexcept;
  bool setCenter(const Point& center) noexcept;
  bool setDiameter(const PositiveLength& dia) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Circle& rhs) const noexcept;
  bool operator!=(const Circle& rhs) const noexcept { return !(*this == rhs); }
  Circle& operator=(const Circle& rhs) noexcept;

private:  // Data
  Uuid              mUuid;
  GraphicsLayerName mLayerName;
  UnsignedLength    mLineWidth;
  bool              mIsFilled;
  bool              mIsGrabArea;
  Point             mCenter;
  PositiveLength    mDiameter;
};

/*******************************************************************************
 *  Class CircleList
 ******************************************************************************/

struct CircleListNameProvider {
  static constexpr const char* tagname = "circle";
};
using CircleList =
    SerializableObjectList<Circle, CircleListNameProvider, Circle::Event>;
using CmdCircleInsert =
    CmdListElementInsert<Circle, CircleListNameProvider, Circle::Event>;
using CmdCircleRemove =
    CmdListElementRemove<Circle, CircleListNameProvider, Circle::Event>;
using CmdCirclesSwap =
    CmdListElementsSwap<Circle, CircleListNameProvider, Circle::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_CIRCLE_H

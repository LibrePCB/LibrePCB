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

#ifndef LIBREPCB_HOLE_H
#define LIBREPCB_HOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../units/all_length_units.h"
#include "../version.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Hole
 ******************************************************************************/

/**
 * @brief The Hole class
 */
class Hole final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Hole)

public:
  // Signals
  enum class Event {
    UuidChanged,
    PositionChanged,
    DiameterChanged,
    LengthChanged,
    RotationChanged,
  };
  Signal<Hole, Event>       onEdited;
  typedef Slot<Hole, Event> OnEditedSlot;

  // Constructors / Destructor
  Hole() = delete;
  Hole(const Hole& other) noexcept;
  Hole(const Uuid& uuid, const Hole& other) noexcept;
  Hole(const Uuid& uuid, const Point& position, const PositiveLength& diameter,
       const UnsignedLength& length, const Angle& rotation) noexcept;
  explicit Hole(const SExpression& node, const Version& projectVersion);
  ~Hole() noexcept;

  // Getters
  const Uuid&           getUuid() const noexcept { return mUuid; }
  const Point&          getPosition() const noexcept { return mPosition; }
  const PositiveLength& getDiameter() const noexcept { return mDiameter; }
  const UnsignedLength& getLength() const noexcept { return mLength; }
  const Angle&          getRotation() const noexcept { return mRotation; }

  // Setters
  bool setPosition(const Point& position) noexcept;
  bool setDiameter(const PositiveLength& diameter) noexcept;
  bool setLength(const UnsignedLength& length) noexcept;
  bool setRotation(const Angle& rotation) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool  operator==(const Hole& rhs) const noexcept;
  bool  operator!=(const Hole& rhs) const noexcept { return !(*this == rhs); }
  Hole& operator=(const Hole& rhs) noexcept;

private:  // Data
  Uuid           mUuid;
  Point          mPosition;
  PositiveLength mDiameter;
  UnsignedLength mLength;
  Angle          mRotation;
};

/*******************************************************************************
 *  Class HoleList
 ******************************************************************************/

struct HoleListNameProvider {
  static constexpr const char* tagname = "hole";
};
using HoleList =
    SerializableObjectList<Hole, HoleListNameProvider, Hole::Event>;
using CmdHoleInsert =
    CmdListElementInsert<Hole, HoleListNameProvider, Hole::Event>;
using CmdHoleRemove =
    CmdListElementRemove<Hole, HoleListNameProvider, Hole::Event>;
using CmdHolesSwap =
    CmdListElementsSwap<Hole, HoleListNameProvider, Hole::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_HOLE_H

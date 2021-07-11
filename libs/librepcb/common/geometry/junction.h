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

#ifndef LIBREPCB_JUNCTION_H
#define LIBREPCB_JUNCTION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Junction
 ******************************************************************************/

/**
 * @brief The Junction class represents the connection point between netlines
 *        or traces
 *
 * The main purpose of this class is to serialize and deserialize junctions
 * contained in schematics or boards.
 */
class Junction final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Junction)

public:
  // Signals
  enum class Event {
    UuidChanged,
    PositionChanged,
  };
  Signal<Junction, Event> onEdited;
  typedef Slot<Junction, Event> OnEditedSlot;

  // Constructors / Destructor
  Junction() = delete;
  Junction(const Junction& other) noexcept;
  Junction(const Uuid& uuid, const Junction& other) noexcept;
  Junction(const Uuid& uuid, const Point& position) noexcept;
  Junction(const SExpression& node, const Version& fileFormat);
  ~Junction() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Point& getPosition() const noexcept { return mPosition; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setPosition(const Point& position) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Junction& rhs) const noexcept;
  bool operator!=(const Junction& rhs) const noexcept {
    return !(*this == rhs);
  }
  Junction& operator=(const Junction& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  Point mPosition;
};

/*******************************************************************************
 *  Class JunctionList
 ******************************************************************************/

struct JunctionListNameProvider {
  static constexpr const char* tagname = "junction";
};
using JunctionList =
    SerializableObjectList<Junction, JunctionListNameProvider, Junction::Event>;
using CmdJunctionInsert =
    CmdListElementInsert<Junction, JunctionListNameProvider, Junction::Event>;
using CmdJunctionRemove =
    CmdListElementRemove<Junction, JunctionListNameProvider, Junction::Event>;
using CmdJunctionsSwap =
    CmdListElementsSwap<Junction, JunctionListNameProvider, Junction::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

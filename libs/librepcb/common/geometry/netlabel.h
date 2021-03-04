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

#ifndef LIBREPCB_NETLABEL_H
#define LIBREPCB_NETLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../units/all_length_units.h"
#include "../alignment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NetLabel
 ******************************************************************************/

/**
 * @brief The NetLabel class represents a net text label of a schematic
 *
 * The main purpose of this class is to serialize and deserialize net labels
 * contained in schematics.
 */
class NetLabel final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(NetLabel)

public:
  // Signals
  enum class Event {
    UuidChanged,
    PositionChanged,
    RotationChanged,
    AlignmentChanged,
  };
  Signal<NetLabel, Event> onEdited;
  typedef Slot<NetLabel, Event> OnEditedSlot;

  // Constructors / Destructor
  NetLabel() = delete;
  NetLabel(const NetLabel& other) noexcept;
  NetLabel(const Uuid& uuid, const NetLabel& other) noexcept;
  NetLabel(const Uuid& uuid, const Point& position,
           const Angle& rotation, const Alignment& alignment) noexcept;
  NetLabel(const SExpression& node, const Version& fileFormat);
  ~NetLabel() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  const Alignment& getAlignment() const noexcept { return mAlignment; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setPosition(const Point& position) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setAlignment(const Alignment& alignment) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const NetLabel& rhs) const noexcept;
  bool operator!=(const NetLabel& rhs) const noexcept {
    return !(*this == rhs);
  }
  NetLabel& operator=(const NetLabel& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  Point mPosition;
  Angle mRotation;
  Alignment mAlignment;
};

/*******************************************************************************
 *  Class NetLabelList
 ******************************************************************************/

struct NetLabelListNameProvider {
  static constexpr const char* tagname = "label";
};
using NetLabelList =
    SerializableObjectList<NetLabel, NetLabelListNameProvider, NetLabel::Event>;
using CmdNetLabelInsert =
    CmdListElementInsert<NetLabel, NetLabelListNameProvider, NetLabel::Event>;
using CmdNetLabelRemove =
    CmdListElementRemove<NetLabel, NetLabelListNameProvider, NetLabel::Event>;
using CmdNetLabelsSwap =
    CmdListElementsSwap<NetLabel, NetLabelListNameProvider, NetLabel::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_CORE_SYMBOLPIN_H
#define LIBREPCB_CORE_SYMBOLPIN_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializableobjectlist.h"
#include "../../types/alignment.h"
#include "../../types/angle.h"
#include "../../types/circuitidentifier.h"
#include "../../types/length.h"
#include "../../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SymbolPin
 ******************************************************************************/

/**
 * @brief The SymbolPin class represents one pin of a symbol
 *
 * Following information is considered as the "interface" of a pin and must
 * therefore never be changed:
 *  - UUID
 */
class SymbolPin final {
  Q_DECLARE_TR_FUNCTIONS(SymbolPin)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
    PositionChanged,
    LengthChanged,
    RotationChanged,
    NamePositionChanged,
    NameRotationChanged,
    NameHeightChanged,
    NameAlignmentChanged,
  };
  Signal<SymbolPin, Event> onEdited;
  typedef Slot<SymbolPin, Event> OnEditedSlot;

  // Constructors / Destructor
  SymbolPin() = delete;
  SymbolPin(const SymbolPin& other) noexcept;
  SymbolPin(const Uuid& uuid, const CircuitIdentifier& name,
            const Point& position, const UnsignedLength& length,
            const Angle& rotation, const Point& namePosition,
            const Angle& nameRotation, const PositiveLength& nameHeight,
            const Alignment& nameAlign) noexcept;
  explicit SymbolPin(const SExpression& node);
  ~SymbolPin() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  const Point& getPosition() const noexcept { return mPosition; }
  const UnsignedLength& getLength() const noexcept { return mLength; }
  const Angle& getRotation() const noexcept { return mRotation; }
  const Point& getNamePosition() const noexcept { return mNamePosition; }
  const Angle& getNameRotation() const noexcept { return mNameRotation; }
  const PositiveLength& getNameHeight() const noexcept { return mNameHeight; }
  const Alignment& getNameAlignment() const noexcept { return mNameAlignment; }

  // Setters
  bool setPosition(const Point& pos) noexcept;
  bool setLength(const UnsignedLength& length) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setName(const CircuitIdentifier& name) noexcept;
  bool setNamePosition(const Point& position) noexcept;
  bool setNameRotation(const Angle& rotation) noexcept;
  bool setNameHeight(const PositiveLength& height) noexcept;
  bool setNameAlignment(const Alignment& align) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const SymbolPin& rhs) const noexcept;
  bool operator!=(const SymbolPin& rhs) const noexcept {
    return !(*this == rhs);
  }
  SymbolPin& operator=(const SymbolPin& rhs) noexcept;

  // Static Methods
  static Point getDefaultNamePosition(const UnsignedLength& length) noexcept {
    return Point(length + Length(1270000), 0);
  }
  static PositiveLength getDefaultNameHeight() noexcept {
    return PositiveLength(2500000);
  }
  static Alignment getDefaultNameAlignment() noexcept {
    return Alignment(HAlign::left(), VAlign::center());
  }

private:  // Data
  Uuid mUuid;
  CircuitIdentifier mName;
  Point mPosition;
  UnsignedLength mLength;
  Angle mRotation;
  Point mNamePosition;
  Angle mNameRotation;
  PositiveLength mNameHeight;
  Alignment mNameAlignment;
};

/*******************************************************************************
 *  Class SymbolPinList
 ******************************************************************************/

struct SymbolPinListNameProvider {
  static constexpr const char* tagname = "pin";
};
using SymbolPinList =
    SerializableObjectList<SymbolPin, SymbolPinListNameProvider,
                           SymbolPin::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

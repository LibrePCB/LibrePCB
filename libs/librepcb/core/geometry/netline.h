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

#ifndef LIBREPCB_CORE_NETLINE_H
#define LIBREPCB_CORE_NETLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NetLineAnchor
 ******************************************************************************/

/**
 * @brief The NetLineAnchor class
 */
class NetLineAnchor final {
  Q_DECLARE_TR_FUNCTIONS(NetLineAnchor)

public:
  // Types
  struct PinAnchor {
    Uuid symbol;
    Uuid pin;

    bool operator==(const PinAnchor& rhs) const noexcept {
      return (symbol == rhs.symbol) && (pin == rhs.pin);
    }
  };

  // Constructors / Destructor
  NetLineAnchor() = delete;
  NetLineAnchor(const NetLineAnchor& other) noexcept;
  explicit NetLineAnchor(const SExpression& node);
  ~NetLineAnchor() noexcept;

  // Getters
  const std::optional<Uuid>& tryGetJunction() const noexcept {
    return mJunction;
  }
  const std::optional<PinAnchor>& tryGetPin() const noexcept { return mPin; }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const NetLineAnchor& rhs) const noexcept;
  bool operator!=(const NetLineAnchor& rhs) const noexcept {
    return !(*this == rhs);
  }
  NetLineAnchor& operator=(const NetLineAnchor& rhs) noexcept;

  // Static Methods
  static NetLineAnchor junction(const Uuid& junction) noexcept;
  static NetLineAnchor pin(const Uuid& symbol, const Uuid& pin) noexcept;

private:  // Methods
  NetLineAnchor(const std::optional<Uuid>& junction,
                const std::optional<PinAnchor>& pin) noexcept;

private:  // Data
  std::optional<Uuid> mJunction;
  std::optional<PinAnchor> mPin;
};

/*******************************************************************************
 *  Class NetLine
 ******************************************************************************/

/**
 * @brief The NetLine class represents a net line within a schematic
 *
 * The main purpose of this class is to serialize and deserialize schematic
 * net lines.
 */
class NetLine final {
  Q_DECLARE_TR_FUNCTIONS(NetLine)

public:
  // Signals
  enum class Event {
    UuidChanged,
    WidthChanged,
    StartPointChanged,
    EndPointChanged,
  };
  Signal<NetLine, Event> onEdited;
  typedef Slot<NetLine, Event> OnEditedSlot;

  // Constructors / Destructor
  NetLine() = delete;
  NetLine(const NetLine& other) noexcept;
  NetLine(const Uuid& uuid, const NetLine& other) noexcept;
  NetLine(const Uuid& uuid, const UnsignedLength& width,
          const NetLineAnchor& start, const NetLineAnchor& end) noexcept;
  explicit NetLine(const SExpression& node);
  ~NetLine() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const UnsignedLength& getWidth() const noexcept { return mWidth; }
  const NetLineAnchor& getStartPoint() const noexcept { return mStart; }
  const NetLineAnchor& getEndPoint() const noexcept { return mEnd; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setWidth(const UnsignedLength& width) noexcept;
  bool setStartPoint(const NetLineAnchor& start) noexcept;
  bool setEndPoint(const NetLineAnchor& end) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const NetLine& rhs) const noexcept;
  bool operator!=(const NetLine& rhs) const noexcept { return !(*this == rhs); }
  NetLine& operator=(const NetLine& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  UnsignedLength mWidth;
  NetLineAnchor mStart;
  NetLineAnchor mEnd;
};

/*******************************************************************************
 *  Class NetLineList
 ******************************************************************************/

struct NetLineListNameProvider {
  static constexpr const char* tagname = "line";
};
using NetLineList =
    SerializableObjectList<NetLine, NetLineListNameProvider, NetLine::Event>;

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline std::size_t qHash(const NetLineAnchor& key,
                         std::size_t seed = 0) noexcept {
  QString s;
  if (std::optional<Uuid> anchor = key.tryGetJunction()) {
    s += anchor->toStr();
  }
  if (std::optional<NetLineAnchor::PinAnchor> anchor = key.tryGetPin()) {
    s += anchor->symbol.toStr();
    s += anchor->pin.toStr();
  }
  Q_ASSERT(!s.isEmpty());

  return ::qHash(s, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

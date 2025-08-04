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

#ifndef LIBREPCB_CORE_DEVICEPADSIGNALMAP_H
#define LIBREPCB_CORE_DEVICEPADSIGNALMAP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializableobjectlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DevicePadSignalMapItem
 ******************************************************************************/

/**
 * @brief The DevicePadSignalMapItem class
 */
class DevicePadSignalMapItem final {
public:
  // Signals
  enum class Event {
    PadUuidChanged,
    SignalUuidChanged,
  };
  Signal<DevicePadSignalMapItem, Event> onEdited;
  typedef Slot<DevicePadSignalMapItem, Event> OnEditedSlot;

  // Constructors / Destructor
  DevicePadSignalMapItem() = delete;
  DevicePadSignalMapItem(const DevicePadSignalMapItem& other) noexcept;
  DevicePadSignalMapItem(const Uuid& pad,
                         const std::optional<Uuid>& signal) noexcept;
  explicit DevicePadSignalMapItem(const SExpression& node);
  ~DevicePadSignalMapItem() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept {
    return mPadUuid;
  }  // used for UuidObjectMap
  const Uuid& getPadUuid() const noexcept { return mPadUuid; }
  const std::optional<Uuid>& getSignalUuid() const noexcept {
    return mSignalUuid;
  }

  // Setters
  bool setSignalUuid(const std::optional<Uuid>& uuid) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const DevicePadSignalMapItem& rhs) const noexcept;
  bool operator!=(const DevicePadSignalMapItem& rhs) const noexcept {
    return !(*this == rhs);
  }
  DevicePadSignalMapItem& operator=(const DevicePadSignalMapItem& rhs) noexcept;

private:  // Data
  Uuid mPadUuid;  ///< must be valid
  std::optional<Uuid>
      mSignalUuid;  ///< std::nullopt if not connected to a signal
};

/*******************************************************************************
 *  Class DevicePadSignalMap
 ******************************************************************************/

struct DevicePadSignalMapNameProvider {
  static constexpr const char* tagname = "pad";
};
using DevicePadSignalMap =
    SerializableObjectList<DevicePadSignalMapItem,
                           DevicePadSignalMapNameProvider,
                           DevicePadSignalMapItem::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

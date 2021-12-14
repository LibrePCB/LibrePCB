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

#ifndef LIBREPCB_LIBRARY_DEVICEPADSIGNALMAP_H
#define LIBREPCB_LIBRARY_DEVICEPADSIGNALMAP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/serializableobjectlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class DevicePadSignalMapItem
 ******************************************************************************/

/**
 * @brief The DevicePadSignalMapItem class
 */
class DevicePadSignalMapItem final : public SerializableObject {
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
                         const tl::optional<Uuid>& signal) noexcept;
  DevicePadSignalMapItem(const SExpression& node, const Version& fileFormat);
  ~DevicePadSignalMapItem() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept {
    return mPadUuid;
  }  // used for UuidObjectMap
  const Uuid& getPadUuid() const noexcept { return mPadUuid; }
  const tl::optional<Uuid>& getSignalUuid() const noexcept {
    return mSignalUuid;
  }

  // Setters
  bool setSignalUuid(const tl::optional<Uuid>& uuid) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const DevicePadSignalMapItem& rhs) const noexcept;
  bool operator!=(const DevicePadSignalMapItem& rhs) const noexcept {
    return !(*this == rhs);
  }
  DevicePadSignalMapItem& operator=(const DevicePadSignalMapItem& rhs) noexcept;

private:  // Data
  Uuid mPadUuid;  ///< must be valid
  tl::optional<Uuid> mSignalUuid;  ///< tl::nullopt if not connected to a signal
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
 *  Class DevicePadSignalMapHelpers
 ******************************************************************************/

class DevicePadSignalMapHelpers {
public:
  DevicePadSignalMapHelpers() = delete;  // disable instantiation

  static tl::optional<Uuid> tryGetSignalUuid(const DevicePadSignalMap& map,
                                             const Uuid& pad) noexcept {
    std::shared_ptr<const DevicePadSignalMapItem> item = map.find(pad);
    return item ? item->getSignalUuid() : tl::nullopt;
  }

  static DevicePadSignalMap create(const QSet<Uuid> pads) noexcept {
    DevicePadSignalMap map;
    foreach (const Uuid& pad, pads) {
      map.append(std::make_shared<DevicePadSignalMapItem>(pad, tl::nullopt));
    }
    return map;
  }

  static void setPads(DevicePadSignalMap& map, QSet<Uuid> pads) noexcept {
    foreach (const Uuid& pad, map.getUuidSet() - pads) { map.remove(pad); }
    foreach (const Uuid& pad, pads - map.getUuidSet()) {
      map.append(std::make_shared<DevicePadSignalMapItem>(pad, tl::nullopt));
    }
    Q_ASSERT(map.getUuidSet() == pads);
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_CORE_ZONE_H
#define LIBREPCB_CORE_ZONE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Zone
 ******************************************************************************/

/**
 * @brief The Zone class
 */
class Zone final {
  Q_DECLARE_TR_FUNCTIONS(Zone)

public:
  enum class Layer : quint32 {
    Top = 1 << 0,
    Inner = 1 << 1,
    Bottom = 1 << 2,
  };
  Q_DECLARE_FLAGS(Layers, Layer)

  enum class Rule : quint32 {
    NoCopper = (1 << 0),  ///< Except planes!
    NoPlanes = (1 << 1),
    NoExposure = (1 << 2),
    NoDevices = (1 << 3),
    All = NoCopper | NoPlanes | NoExposure | NoDevices,
  };
  Q_DECLARE_FLAGS(Rules, Rule)

  // Signals
  enum class Event {
    UuidChanged,
    LayersChanged,
    RulesChanged,
    OutlineChanged,
  };
  Signal<Zone, Event> onEdited;
  typedef Slot<Zone, Event> OnEditedSlot;

  // Constructors / Destructor
  Zone() = delete;
  Zone(const Zone& other) noexcept;
  Zone(const Uuid& uuid, const Zone& other) noexcept;
  Zone(const Uuid& uuid, Layers layers, Rules rules,
       const Path& outline) noexcept;
  explicit Zone(const SExpression& node);
  ~Zone() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  Layers getLayers() const noexcept { return mLayers; }
  Rules getRules() const noexcept { return mRules; }
  const Path& getOutline() const noexcept { return mOutline; }

  // Setters
  bool setLayers(Layers layers) noexcept;
  bool setRules(Rules rules) noexcept;
  bool setOutline(const Path& outline) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Zone& rhs) const noexcept;
  bool operator!=(const Zone& rhs) const noexcept { return !(*this == rhs); }
  Zone& operator=(const Zone& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  Layers mLayers;
  Rules mRules;
  Path mOutline;
};

/*******************************************************************************
 *  Class ZoneList
 ******************************************************************************/

struct ZoneListNameProvider {
  static constexpr const char* tagname = "zone";
};
using ZoneList =
    SerializableObjectList<Zone, ZoneListNameProvider, Zone::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Zone::Layer)
Q_DECLARE_METATYPE(librepcb::Zone::Rule)
Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::Zone::Layers)
Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::Zone::Rules)

#endif

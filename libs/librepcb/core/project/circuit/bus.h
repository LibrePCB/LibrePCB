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

#ifndef LIBREPCB_CORE_BUS_H
#define LIBREPCB_CORE_BUS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/busname.h"
#include "../../types/length.h"
#include "../../types/uuid.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class NetSignal;
class SI_BusSegment;

/*******************************************************************************
 *  Class Bus
 ******************************************************************************/

/**
 * @brief The Bus class
 */
class Bus final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  Bus() = delete;
  Bus(const Bus& other) = delete;
  explicit Bus(Circuit& circuit, const Uuid& uuid, const BusName& name,
               bool autoName, bool prefixNetNames,
               const std::optional<UnsignedLength>& maxTraceLengthDifference);
  ~Bus() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const BusName& getName() const noexcept { return mName; }
  bool hasAutoName() const noexcept { return mHasAutoName; }
  bool getPrefixNetNames() const noexcept { return mPrefixNetNames; }
  const std::optional<UnsignedLength>& getMaxTraceLengthDifference()
      const noexcept {
    return mMaxTraceLengthDifference;
  }

  // Getters: General
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const QList<SI_BusSegment*>& getSchematicBusSegments() const noexcept {
    return mRegisteredSchematicBusSegments;
  }
  QSet<NetSignal*> getConnectedNetSignals() const noexcept;
  bool isUsed() const noexcept;
  bool isAddedToCircuit() const noexcept { return mIsAddedToCircuit; }

  // Setters
  void setName(const BusName& name, bool isAutoName) noexcept;
  void setPrefixNetNames(bool prefix) noexcept;
  void setMaxTraceLengthDifference(
      const std::optional<UnsignedLength>& diff) noexcept;

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerSchematicBusSegment(SI_BusSegment& s);
  void unregisterSchematicBusSegment(SI_BusSegment& s);

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  Bus& operator=(const Bus& rhs) = delete;
  bool operator==(const Bus& rhs) const noexcept { return (this == &rhs); }
  bool operator!=(const Bus& rhs) const noexcept { return (this != &rhs); }

signals:
  void nameChanged(const BusName& newName);

private:
  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;

  // Attributes
  Uuid mUuid;
  BusName mName;
  bool mHasAutoName;
  bool mPrefixNetNames;
  std::optional<UnsignedLength> mMaxTraceLengthDifference;

  // Registered Elements of this bus
  QList<SI_BusSegment*> mRegisteredSchematicBusSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

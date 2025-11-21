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
#include "../../types/circuitidentifier.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class NetSignal;

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
  explicit Bus(Circuit& circuit, const Uuid& uuid,
               const CircuitIdentifier& name, bool autoName);
  ~Bus() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  bool hasAutoName() const noexcept { return mHasAutoName; }

  // Getters: General
  Circuit& getCircuit() const noexcept { return mCircuit; }
  int getRegisteredElementsCount() const noexcept;
  bool isUsed() const noexcept;
  bool isAddedToCircuit() const noexcept { return mIsAddedToCircuit; }
  const QSet<NetSignal*>& getNets() noexcept { return mNetSignals; }

  // Setters
  void setName(const CircuitIdentifier& name, bool isAutoName) noexcept;

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void addNet(NetSignal& net);
  void removeNet(NetSignal& net);

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
  void nameChanged(const CircuitIdentifier& newName);

private:
  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;

  // Attributes
  Uuid mUuid;
  CircuitIdentifier mName;
  bool mHasAutoName;

  // Contained net signals
  QSet<NetSignal*> mNetSignals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

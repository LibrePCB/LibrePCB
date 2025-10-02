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

#ifndef LIBREPCB_CORE_NETCLASS_H
#define LIBREPCB_CORE_NETCLASS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/elementname.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class NetSignal;
class SExpression;

/*******************************************************************************
 *  Class NetClass
 ******************************************************************************/

/**
 * @brief The NetClass class
 */
class NetClass final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  NetClass() = delete;
  NetClass(const NetClass& other) = delete;
  explicit NetClass(Circuit& circuit, const Uuid& uuid,
                    const ElementName& name);
  explicit NetClass(Circuit& circuit, const SExpression& node);
  ~NetClass() noexcept;

  // Getters
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  const std::optional<PositiveLength>& getDefaultTraceWidth() const noexcept {
    return mDefaultTraceWidth;
  }
  int getNetSignalCount() const noexcept {
    return mRegisteredNetSignals.count();
  }
  bool isUsed() const noexcept { return (getNetSignalCount() > 0); }

  // Setters
  void setName(const ElementName& name) noexcept;
  void setDefaultTraceWidth(
      const std::optional<PositiveLength>& value) noexcept;

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerNetSignal(NetSignal& signal);
  void unregisterNetSignal(NetSignal& signal);

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  NetClass& operator=(const NetClass& rhs) = delete;

private:
  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;

  // Attributes
  Uuid mUuid;
  ElementName mName;

  // Design Rules
  // Note: If `std::nullopt` (the default), the values from the corresponding
  // board's design rules are used instead.
  std::optional<PositiveLength> mDefaultTraceWidth;

  // Registered Elements
  /// @brief all registered netsignals
  QHash<Uuid, NetSignal*> mRegisteredNetSignals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

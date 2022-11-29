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
#include "../erc/if_ercmsgprovider.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class ErcMsg;
class NetSignal;

/*******************************************************************************
 *  Class NetClass
 ******************************************************************************/

/**
 * @brief The NetClass class
 */
class NetClass final : public QObject, public IF_ErcMsgProvider {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(NetClass)

public:
  // Constructors / Destructor
  NetClass() = delete;
  NetClass(const NetClass& other) = delete;
  NetClass(Circuit& circuit, const SExpression& node,
           const Version& fileFormat);
  explicit NetClass(Circuit& circuit, const ElementName& name);
  ~NetClass() noexcept;

  // Getters
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  int getNetSignalCount() const noexcept {
    return mRegisteredNetSignals.count();
  }
  bool isUsed() const noexcept { return (getNetSignalCount() > 0); }

  // Setters
  void setName(const ElementName& name) noexcept;

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
  void updateErcMessages() noexcept;

  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;

  // Attributes
  Uuid mUuid;
  ElementName mName;

  // Registered Elements
  /// @brief all registered netsignals
  QHash<Uuid, NetSignal*> mRegisteredNetSignals;

  // ERC Messages
  /// @brief the ERC message for unused netclasses
  QScopedPointer<ErcMsg> mErcMsgUnusedNetClass;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_CORE_NETSIGNAL_H
#define LIBREPCB_CORE_NETSIGNAL_H

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

class BI_NetSegment;
class BI_Plane;
class Circuit;
class ComponentSignalInstance;
class NetClass;
class SI_NetSegment;

/*******************************************************************************
 *  Class NetSignal
 ******************************************************************************/

/**
 * @brief The NetSignal class
 */
class NetSignal final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  NetSignal() = delete;
  NetSignal(const NetSignal& other) = delete;
  explicit NetSignal(Circuit& circuit, const Uuid& uuid, NetClass& netclass,
                     const CircuitIdentifier& name, bool autoName);
  ~NetSignal() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  bool hasAutoName() const noexcept { return mHasAutoName; }
  NetClass& getNetClass() const noexcept { return mNetClass; }
  bool isHighlighted() const noexcept { return mIsHighlighted; }

  // Getters: General
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const QList<ComponentSignalInstance*>& getComponentSignals() const noexcept {
    return mRegisteredComponentSignals;
  }
  const QList<SI_NetSegment*>& getSchematicNetSegments() const noexcept {
    return mRegisteredSchematicNetSegments;
  }
  const QList<BI_NetSegment*>& getBoardNetSegments() const noexcept {
    return mRegisteredBoardNetSegments;
  }
  const QList<BI_Plane*>& getBoardPlanes() const noexcept {
    return mRegisteredBoardPlanes;
  }
  int getRegisteredElementsCount() const noexcept;
  bool isUsed() const noexcept;
  bool isNameForced() const noexcept;
  bool isAddedToCircuit() const noexcept { return mIsAddedToCircuit; }

  // Setters
  void setName(const CircuitIdentifier& name, bool isAutoName) noexcept;
  void setHighlighted(bool hl) noexcept;

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerComponentSignal(ComponentSignalInstance& signal);
  void unregisterComponentSignal(ComponentSignalInstance& signal);
  void registerSchematicNetSegment(SI_NetSegment& netsegment);
  void unregisterSchematicNetSegment(SI_NetSegment& netsegment);
  void registerBoardNetSegment(BI_NetSegment& netsegment);
  void unregisterBoardNetSegment(BI_NetSegment& netsegment);
  void registerBoardPlane(BI_Plane& plane);
  void unregisterBoardPlane(BI_Plane& plane);

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  NetSignal& operator=(const NetSignal& rhs) = delete;
  bool operator==(const NetSignal& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const NetSignal& rhs) noexcept { return (this != &rhs); }

signals:

  void nameChanged(const CircuitIdentifier& newName);
  void highlightedChanged(bool isHighlighted);

private:
  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;
  bool mIsHighlighted;

  // Attributes
  Uuid mUuid;
  CircuitIdentifier mName;
  bool mHasAutoName;
  NetClass& mNetClass;

  // Registered Elements of this NetSignal
  QList<ComponentSignalInstance*> mRegisteredComponentSignals;
  QList<SI_NetSegment*> mRegisteredSchematicNetSegments;
  QList<BI_NetSegment*> mRegisteredBoardNetSegments;
  QList<BI_Plane*> mRegisteredBoardPlanes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

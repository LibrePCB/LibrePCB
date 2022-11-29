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

#ifndef LIBREPCB_CORE_COMPONENTSIGNALINSTANCE_H
#define LIBREPCB_CORE_COMPONENTSIGNALINSTANCE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/circuitidentifier.h"
#include "../erc/if_ercmsgprovider.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_FootprintPad;
class Circuit;
class ComponentInstance;
class ComponentSignal;
class ErcMsg;
class NetSignal;
class SI_SymbolPin;

/*******************************************************************************
 *  Class ComponentSignalInstance
 ******************************************************************************/

/**
 * @brief The ComponentSignalInstance class
 */
class ComponentSignalInstance final : public QObject, public IF_ErcMsgProvider {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(ComponentSignalInstance)

public:
  // Constructors / Destructor
  ComponentSignalInstance() = delete;
  ComponentSignalInstance(const ComponentSignalInstance& other) = delete;
  ComponentSignalInstance(Circuit& circuit, ComponentInstance& cmpInstance,
                          const SExpression& node, const Version& fileFormat);
  explicit ComponentSignalInstance(Circuit& circuit,
                                   ComponentInstance& cmpInstance,
                                   const ComponentSignal& cmpSignal,
                                   NetSignal* netsignal = nullptr);
  ~ComponentSignalInstance() noexcept;

  // Getters
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const ComponentSignal& getCompSignal() const noexcept {
    return *mComponentSignal;
  }
  NetSignal* getNetSignal() const noexcept { return mNetSignal; }
  ComponentInstance& getComponentInstance() const noexcept {
    return mComponentInstance;
  }
  bool isNetSignalNameForced() const noexcept;
  QString getForcedNetSignalName() const noexcept;
  const QList<SI_SymbolPin*>& getRegisteredSymbolPins() const noexcept {
    return mRegisteredSymbolPins;
  }
  const QList<BI_FootprintPad*>& getRegisteredFootprintPads() const noexcept {
    return mRegisteredFootprintPads;
  }
  int getRegisteredElementsCount() const noexcept;
  bool isUsed() const noexcept { return (getRegisteredElementsCount() > 0); }
  bool arePinsOrPadsUsed() const noexcept;

  // Setters

  /**
   * @brief (Re-)Connect/Disconnect this component signal to/from a circuit's
   * netsignal
   *
   * @warning This method must always be called from inside an UndoCommand!
   *
   * @param netsignal     - (Re-)Connect: A Pointer to the new netsignal
   *                      - Disconnect: 0
   *
   * @throw Exception     This method throws an exception in case of an error
   */
  void setNetSignal(NetSignal* netsignal);

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerSymbolPin(SI_SymbolPin& pin);
  void unregisterSymbolPin(SI_SymbolPin& pin);
  void registerFootprintPad(BI_FootprintPad& pad);
  void unregisterFootprintPad(BI_FootprintPad& pad);

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  ComponentSignalInstance& operator=(const ComponentSignalInstance& rhs) =
      delete;

signals:
  void netSignalChanged(NetSignal* from, NetSignal* to);

private slots:

  void netSignalNameChanged(const CircuitIdentifier& newName) noexcept;
  void updateErcMessages() noexcept;

private:
  void init();
  bool checkAttributesValidity() const noexcept;

  // General
  Circuit& mCircuit;
  ComponentInstance& mComponentInstance;
  const ComponentSignal* mComponentSignal;
  bool mIsAddedToCircuit;

  // Attributes
  NetSignal* mNetSignal;

  // Registered Elements
  QList<SI_SymbolPin*> mRegisteredSymbolPins;
  QList<BI_FootprintPad*> mRegisteredFootprintPads;

  // ERC Messages
  /// @brief The ERC message for an unconnected required component signal
  QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredSignal;
  /// @brief The ERC message for a global net signal name mismatch
  QScopedPointer<ErcMsg> mErcMsgForcedNetSignalNameConflict;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

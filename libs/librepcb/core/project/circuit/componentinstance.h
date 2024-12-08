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

#ifndef LIBREPCB_CORE_COMPONENTINSTANCE_H
#define LIBREPCB_CORE_COMPONENTINSTANCE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attribute.h"
#include "../../types/circuitidentifier.h"
#include "../../types/uuid.h"
#include "componentassemblyoption.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Circuit;
class Component;
class ComponentSignalInstance;
class ComponentSymbolVariant;
class SI_Symbol;

/*******************************************************************************
 *  Class ComponentInstance
 ******************************************************************************/

/**
 * @brief The ComponentInstance class
 */
class ComponentInstance : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentInstance() = delete;
  ComponentInstance(const ComponentInstance& other) = delete;
  explicit ComponentInstance(Circuit& circuit, const Uuid& uuid,
                             const Component& cmp, const Uuid& symbVar,
                             const CircuitIdentifier& name);
  ~ComponentInstance() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  const QString& getValue() const noexcept { return mValue; }
  const QPointer<const BI_Device>& getPrimaryDevice() const noexcept {
    return mPrimaryDevice;
  }
  const Component& getLibComponent() const noexcept { return mLibComponent; }
  const ComponentSymbolVariant& getSymbolVariant() const noexcept {
    return *mCompSymbVar;
  }
  const QMap<Uuid, ComponentSignalInstance*>& getSignals() const noexcept {
    return mSignals;
  }
  ComponentSignalInstance* getSignalInstance(
      const Uuid& signalUuid) const noexcept {
    return mSignals.value(signalUuid);
  }
  const AttributeList& getAttributes() const noexcept { return *mAttributes; }
  const ComponentAssemblyOptionList& getAssemblyOptions() const noexcept {
    return mAssemblyOptions;
  }
  QSet<Uuid> getCompatibleDevices() const noexcept;
  QVector<std::shared_ptr<const Part>> getParts(
      const std::optional<Uuid>& assemblyVariant) const noexcept;
  bool getLockAssembly() const noexcept { return mLockAssembly; }

  // Getters: General
  Circuit& getCircuit() const noexcept { return mCircuit; }
  const QHash<Uuid, SI_Symbol*>& getSymbols() const noexcept {
    return mRegisteredSymbols;
  }
  const QList<BI_Device*>& getDevices() const noexcept {
    return mRegisteredDevices;
  }
  QSet<Uuid> getUsedDeviceUuids() const noexcept;
  int getRegisteredElementsCount() const noexcept;
  bool isUsed() const noexcept;
  bool isAddedToCircuit() const noexcept { return mIsAddedToCircuit; }

  // Setters

  /**
   * @brief Set the name of this component instance in the circuit
   *
   * @warning You have to check if there is no other component with the same
   * name in the whole circuit! This method will not check if the name is
   * unique. The best way to do this is to call
   * ::librepcb::Circuit::setComponentInstanceName().
   *
   * @param name  The new name of this component in the circuit
   */
  void setName(const CircuitIdentifier& name) noexcept;

  /**
   * @brief Set the value of this component instance in the circuit
   *
   * @param value  The new value
   */
  void setValue(const QString& value) noexcept;

  void setAttributes(const AttributeList& attributes) noexcept;

  void setAssemblyOptions(const ComponentAssemblyOptionList& options) noexcept;

  void setLockAssembly(bool lock) noexcept { mLockAssembly = lock; }

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerSymbol(SI_Symbol& symbol);
  void unregisterSymbol(SI_Symbol& symbol);
  void registerDevice(BI_Device& device);
  void unregisterDevice(BI_Device& device);

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  ComponentInstance& operator=(const ComponentInstance& rhs) = delete;

signals:
  void attributesChanged();
  void primaryDeviceChanged(const BI_Device* device);

private:
  void updatePrimaryDevice() noexcept;
  bool checkAttributesValidity() const noexcept;
  const QStringList& getLocaleOrder() const noexcept;

  // General
  Circuit& mCircuit;
  bool mIsAddedToCircuit;

  // Attributes

  /// @brief The unique UUID of this component instance in the circuit
  Uuid mUuid;

  /// @brief The unique name of this component instance in the circuit (e.g.
  /// "R42")
  CircuitIdentifier mName;

  /// @brief The value of this component instance in the circuit (e.g. the
  /// resistance of a resistor)
  QString mValue;

  /// @brief Reference to the component in the project's library
  const Component& mLibComponent;

  /// @brief Pointer to the used symbol variant of #mLibComponent
  const ComponentSymbolVariant* mCompSymbVar;

  /// @brief All attributes of this component
  QScopedPointer<AttributeList> mAttributes;

  /// @brief All signal instances (Key: component signal UUID)
  QMap<Uuid, ComponentSignalInstance*> mSignals;

  /// @brief Assembly options including MPNs
  ComponentAssemblyOptionList mAssemblyOptions;

  /// @brief Whether #mAssemblyOptions can be modified from the board editor
  ///        or not
  bool mLockAssembly;

  // Registered Elements

  /**
   * @brief All registered symbols
   *
   * - Key:   UUID of the symbol variant item
   * (::librepcb::ComponentSymbolVariantItem)
   * - Value: Pointer to the registered symbol
   *
   * @see #registerSymbol(), #unregisterSymbol()
   */
  QHash<Uuid, SI_Symbol*> mRegisteredSymbols;

  /**
   * @brief All registered devices (of all boards)
   *
   * @see #registerDevice(), #unregisterDevice()
   */
  QList<BI_Device*> mRegisteredDevices;

  // Cached Properties
  QPointer<const BI_Device> mPrimaryDevice;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_PROJECT_COMPONENTINSTANCE_H
#define LIBREPCB_PROJECT_COMPONENTINSTANCE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../erc/if_ercmsgprovider.h"

#include <librepcb/common/attributes/attribute.h>
#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class DomElement;

namespace library {
class Component;
class ComponentSymbolVariant;
}  // namespace library

namespace project {

class Circuit;
class ComponentSignalInstance;
class BI_Device;
class SI_Symbol;
class ErcMsg;

/*******************************************************************************
 *  Class ComponentInstance
 ******************************************************************************/

/**
 * @brief The ComponentInstance class
 */
class ComponentInstance : public QObject,
                          public AttributeProvider,
                          public IF_ErcMsgProvider,
                          public SerializableObject {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(ComponentInstance)

public:
  // Constructors / Destructor
  ComponentInstance() = delete;
  ComponentInstance(const ComponentInstance& other) = delete;
  explicit ComponentInstance(Circuit& circuit, const SExpression& node);
  explicit ComponentInstance(
      Circuit& circuit, const library::Component& cmp, const Uuid& symbVar,
      const CircuitIdentifier& name,
      const tl::optional<Uuid>& defaultDevice = tl::nullopt);
  ~ComponentInstance() noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const CircuitIdentifier& getName() const noexcept { return mName; }
  QString getValue(bool replaceAttributes = false) const noexcept;
  const tl::optional<Uuid>& getDefaultDeviceUuid() const noexcept {
    return mDefaultDeviceUuid;
  }
  const library::Component& getLibComponent() const noexcept {
    return *mLibComponent;
  }
  const library::ComponentSymbolVariant& getSymbolVariant() const noexcept {
    return *mCompSymbVar;
  }
  ComponentSignalInstance* getSignalInstance(const Uuid& signalUuid) const
      noexcept {
    return mSignals.value(signalUuid);
  }
  const AttributeList& getAttributes() const noexcept { return *mAttributes; }

  // Getters: General
  Circuit& getCircuit() const noexcept { return mCircuit; }
  int getPlacedSymbolsCount() const noexcept {
    return mRegisteredSymbols.count();
  }
  int getUnplacedSymbolsCount() const noexcept;
  int getUnplacedRequiredSymbolsCount() const noexcept;
  int getUnplacedOptionalSymbolsCount() const noexcept;
  int getRegisteredElementsCount() const noexcept;
  bool isUsed() const noexcept;

  // Setters

  /**
   * @brief Set the name of this component instance in the circuit
   *
   * @warning You have to check if there is no other component with the same
   * name in the whole circuit! This method will not check if the name is
   * unique. The best way to do this is to call
   * ::librepcb::project::Circuit::setComponentInstanceName().
   *
   * @param name  The new name of this component in the circuit
   *         *
   * @undocmd{::librepcb::project::CmdComponentInstanceEdit}
   */
  void setName(const CircuitIdentifier& name) noexcept;

  /**
   * @brief Set the value of this component instance in the circuit
   *
   * @param value  The new value
   *
   * @undocmd{::librepcb::project::CmdComponentInstanceEdit}
   */
  void setValue(const QString& value) noexcept;

  void setAttributes(const AttributeList& attributes) noexcept;

  /**
   * @brief Set the default device of the component
   *
   * @param device  The new device UUID
   *
   * @undocmd{::librepcb::project::CmdComponentInstanceEdit}
   */
  void setDefaultDeviceUuid(const tl::optional<Uuid>& device) noexcept;

  // General Methods
  void addToCircuit();
  void removeFromCircuit();
  void registerSymbol(SI_Symbol& symbol);
  void unregisterSymbol(SI_Symbol& symbol);
  void registerDevice(BI_Device& device);
  void unregisterDevice(BI_Device& device);

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from AttributeProvider
  /// @copydoc librepcb::AttributeProvider::getUserDefinedAttributeValue()
  QString getUserDefinedAttributeValue(const QString& key) const
      noexcept override;
  /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Operator Overloadings
  ComponentInstance& operator=(const ComponentInstance& rhs) = delete;

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

private:
  void init();
  bool checkAttributesValidity() const noexcept;
  void updateErcMessages() noexcept;
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

  /// @brief THe default device when adding the component to a board
  tl::optional<Uuid> mDefaultDeviceUuid;

  /// @brief Pointer to the component in the project's library
  const library::Component* mLibComponent;

  /// @brief Pointer to the used symbol variant of #mLibComponent
  const library::ComponentSymbolVariant* mCompSymbVar;

  /// @brief All attributes of this component
  QScopedPointer<AttributeList> mAttributes;

  /// @brief All signal instances (Key: component signal UUID)
  QMap<Uuid, ComponentSignalInstance*> mSignals;

  // Registered Elements

  /**
   * @brief All registered symbols
   *
   * - Key:   UUID of the symbol variant item
   * (::librepcb::library::ComponentSymbolVariantItem)
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

  // ERC Messages

  /// @brief The ERC message for unplaced required symbols of this component
  QScopedPointer<ErcMsg> mErcMsgUnplacedRequiredSymbols;

  /// @brief The ERC message for unplaced optional symbols of this component
  QScopedPointer<ErcMsg> mErcMsgUnplacedOptionalSymbols;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_COMPONENTINSTANCE_H

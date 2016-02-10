/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/if_attributeprovider.h>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class XmlDomElement;

namespace library {
class Component;
class ComponentSymbolVariant;
}

namespace project {

class Circuit;
class ComponentAttributeInstance;
class ComponentSignalInstance;
class DeviceInstance;
class SI_Symbol;
class ErcMsg;

/*****************************************************************************************
 *  Class ComponentInstance
 ****************************************************************************************/

/**
 * @brief The ComponentInstance class
 */
class ComponentInstance : public QObject, public IF_AttributeProvider,
                          public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(ComponentInstance)

    public:

        // Constructors / Destructor
        explicit ComponentInstance(Circuit& circuit, const XmlDomElement& domElement) throw (Exception);
        explicit ComponentInstance(Circuit& circuit, const library::Component& cmp,
                                   const Uuid& symbVar, const QString& name) throw (Exception);
        ~ComponentInstance() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        QString getValue(bool replaceAttributes = false) const noexcept;
        int getPlacedSymbolsCount() const noexcept {return mSymbols.count();}
        int getUnplacedSymbolsCount() const noexcept;
        int getUnplacedRequiredSymbolsCount() const noexcept;
        int getUnplacedOptionalSymbolsCount() const noexcept;
        ComponentSignalInstance* getSignalInstance(const Uuid& signalUuid) const noexcept {return mSignals.value(signalUuid);}
        const library::Component& getLibComponent() const noexcept {return *mLibComponent;}
        const library::ComponentSymbolVariant& getSymbolVariant() const noexcept {return *mCompSymbVar;}


        // Setters

        /**
         * @brief Set the name of this component instance in the circuit
         *
         * @warning You have to check if there is no other component with the same name in
         *          the whole circuit! This method will not check if the name is unique.
         *          The best way to do this is to call #project#Circuit#setComponentInstanceName().
         *
         * @param name  The new name of this component in the circuit (must not be empty)
         *
         * @throw Exception If the new name is invalid, an exception will be thrown
         *
         * @undocmd{#project#CmdComponentInstanceEdit}
         */
        void setName(const QString& name) throw (Exception);

        /**
         * @brief Set the value of this component instance in the circuit
         *
         * @param value  The new value
         *
         * @undocmd{#project#CmdComponentInstanceEdit}
         */
        void setValue(const QString& value) noexcept;


        // Attribute Handling Methods
        const QList<ComponentAttributeInstance*>& getAttributes() const noexcept {return mAttributes;}
        ComponentAttributeInstance* getAttributeByKey(const QString& key) const noexcept;
        void addAttribute(ComponentAttributeInstance& attr) throw (Exception);
        void removeAttribute(ComponentAttributeInstance& attr) throw (Exception);


        // General Methods
        void addToCircuit() throw (Exception);
        void removeFromCircuit() throw (Exception);
        void registerSymbol(const SI_Symbol& symbol) throw (Exception);
        void unregisterSymbol(const SI_Symbol& symbol) throw (Exception);
        void registerDevice(const DeviceInstance& device) throw (Exception);
        void unregisterDevice(const DeviceInstance& device) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();


    private:

        // make some methods inaccessible...
        ComponentInstance();
        ComponentInstance(const ComponentInstance& other);
        ComponentInstance& operator=(const ComponentInstance& rhs);

        // Private Methods
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        Circuit& mCircuit;
        bool mAddedToCircuit;


        // Attributes

        /// @brief The unique UUID of this component instance in the circuit
        Uuid mUuid;

        /// @brief The unique name of this component instance in the circuit (e.g. "R42")
        QString mName;

        /// @brief The value of this component instance in the circuit (e.g. the resistance of a resistor)
        QString mValue;

        /// @brief Pointer to the component in the project's library
        const library::Component* mLibComponent;

        /// @brief Pointer to the used symbol variant of #mLibComponent
        const library::ComponentSymbolVariant* mCompSymbVar;

        /// @brief All attributes of this component
        QList<ComponentAttributeInstance*> mAttributes;

        /// @brief All signal instances (Key: component signal UUID)
        QHash<Uuid, ComponentSignalInstance*> mSignals;


        // Misc

        /**
         * @brief All registered symbols
         *
         * - Key:   UUID of the symbol variant item (#library#ComponentSymbolVariantItem)
         * - Value: Pointer to the registered symbol
         *
         * @see #registerSymbol(), #unregisterSymbol()
         */
        QHash<Uuid, const SI_Symbol*> mSymbols;

        /**
         * @brief All registered device instances
         *
         * @see #registerDevice(), #unregisterDevice()
         */
        QList<const DeviceInstance*> mDeviceInstances;

        /// @brief The ERC message for unplaced required symbols of this component
        QScopedPointer<ErcMsg> mErcMsgUnplacedRequiredSymbols;

        /// @brief The ERC message for unplaced optional symbols of this component
        QScopedPointer<ErcMsg> mErcMsgUnplacedOptionalSymbols;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_COMPONENTINSTANCE_H

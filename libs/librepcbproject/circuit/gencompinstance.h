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

#ifndef PROJECT_GENCOMPINSTANCE_H
#define PROJECT_GENCOMPINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/if_attributeprovider.h>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace project {
class Circuit;
class GenCompAttributeInstance;
class GenCompSignalInstance;
class DeviceInstance;
class SI_Symbol;
class ErcMsg;
}

namespace library {
class Component;
class ComponentSymbolVariant;
}

/*****************************************************************************************
 *  Class GenCompInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenCompInstance class
 */
class GenCompInstance : public QObject, public IF_AttributeProvider,
                        public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(GenCompInstance)

    public:

        // Constructors / Destructor
        explicit GenCompInstance(Circuit& circuit, const XmlDomElement& domElement) throw (Exception);
        explicit GenCompInstance(Circuit& circuit, const library::Component& genComp,
                                 const library::ComponentSymbolVariant& symbVar, const QString& name) throw (Exception);
        ~GenCompInstance() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        QString getValue(bool replaceAttributes = false) const noexcept;
        int getPlacedSymbolsCount() const noexcept {return mSymbols.count();}
        int getUnplacedSymbolsCount() const noexcept;
        int getUnplacedRequiredSymbolsCount() const noexcept;
        int getUnplacedOptionalSymbolsCount() const noexcept;
        GenCompSignalInstance* getSignalInstance(const QUuid& signalUuid) const noexcept {return mSignals.value(signalUuid);}
        const library::Component& getGenComp() const noexcept {return *mGenComp;}
        const library::ComponentSymbolVariant& getSymbolVariant() const noexcept {return *mGenCompSymbVar;}


        // Setters

        /**
         * @brief Set the name of this generic component instance in the circuit
         *
         * @warning You have to check if there is no other component with the same name in
         *          the whole circuit! This method will not check if the name is unique.
         *          The best way to do this is to call Circuit#setGenCompInstanceName().
         *
         * @param name  The new name of this component in the circuit (must not be empty)
         *
         * @throw Exception If the new name is invalid, an exception will be thrown
         *
         * @undocmd{project#CmdGenCompInstEdit}
         */
        void setName(const QString& name) throw (Exception);

        /**
         * @brief Set the value of this generic component instance in the circuit
         *
         * @param value  The new value
         *
         * @undocmd{project#CmdGenCompInstEdit}
         */
        void setValue(const QString& value) noexcept;


        // Attribute Handling Methods
        const QList<GenCompAttributeInstance*>& getAttributes() const noexcept {return mAttributes;}
        GenCompAttributeInstance* getAttributeByKey(const QString& key) const noexcept;
        void addAttribute(GenCompAttributeInstance& attr) throw (Exception);
        void removeAttribute(GenCompAttributeInstance& attr) throw (Exception);


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
        GenCompInstance();
        GenCompInstance(const GenCompInstance& other);
        GenCompInstance& operator=(const GenCompInstance& rhs);

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
        QUuid mUuid;

        /// @brief The unique name of this component instance in the circuit (e.g. "R42")
        QString mName;

        /// @brief The value of this component instance in the circuit (e.g. the resistance of a resistor)
        QString mValue;

        /// @brief Pointer to the generic component in the project's library
        const library::Component* mGenComp;

        /// @brief Pointer to the used symbol variant of #mGenComp
        const library::ComponentSymbolVariant* mGenCompSymbVar;

        /// @brief All attributes of this generic component
        QList<GenCompAttributeInstance*> mAttributes;

        /// @brief All signal instances (Key: generic component signal UUID)
        QHash<QUuid, GenCompSignalInstance*> mSignals;


        // Misc

        /**
         * @brief All registered symbols
         *
         * - Key:   UUID of the symbol variant item (library#GenCompSymbVarItem)
         * - Value: Pointer to the registered symbol
         *
         * @see #registerSymbol(), #unregisterSymbol()
         */
        QHash<QUuid, const SI_Symbol*> mSymbols;

        /**
         * @brief All registered device instances
         *
         * @see #registerDevice(), #unregisterDevice()
         */
        QList<const DeviceInstance*> mDeviceInstances;

        /// @brief The ERC message for unplaced required symbols of this generic component
        QScopedPointer<ErcMsg> mErcMsgUnplacedRequiredSymbols;

        /// @brief The ERC message for unplaced optional symbols of this generic component
        QScopedPointer<ErcMsg> mErcMsgUnplacedOptionalSymbols;
};

} // namespace project

#endif // PROJECT_GENCOMPINSTANCE_H

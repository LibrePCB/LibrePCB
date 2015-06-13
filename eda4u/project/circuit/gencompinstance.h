/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <eda4ucommon/if_attributeprovider.h>
#include "../erc/if_ercmsgprovider.h"
#include <eda4ucommon/fileio/if_xmlserializableobject.h>
#include <eda4ucommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace project {
class Circuit;
class GenCompAttributeInstance;
class GenCompSignalInstance;
class ComponentInstance;
class SI_Symbol;
class ErcMsg;
}

namespace library {
class GenericComponent;
class GenCompSymbVar;
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
        explicit GenCompInstance(Circuit& circuit, const library::GenericComponent& genComp,
                                 const library::GenCompSymbVar& symbVar, const QString& name) throw (Exception);
        ~GenCompInstance() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        QString getValue(bool replaceAttributes = false) const noexcept;
        uint getPlacedSymbolsCount() const noexcept {return mSymbols.count();}
        uint getUnplacedSymbolsCount() const noexcept;
        uint getUnplacedRequiredSymbolsCount() const noexcept;
        uint getUnplacedOptionalSymbolsCount() const noexcept;
        GenCompSignalInstance* getSignalInstance(const QUuid& signalUuid) const noexcept {return mSignals.value(signalUuid);}
        const library::GenericComponent& getGenComp() const noexcept {return *mGenComp;}
        const library::GenCompSymbVar& getSymbolVariant() const noexcept {return *mGenCompSymbVar;}


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
        void registerComponent(const ComponentInstance& component) throw (Exception);
        void unregisterComponent(const ComponentInstance& component) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

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
        bool checkAttributesValidity() const noexcept;
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
        const library::GenericComponent* mGenComp;

        /// @brief Pointer to the used symbol variant of #mGenComp
        const library::GenCompSymbVar* mGenCompSymbVar;

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
         * @brief All registered component instances
         *
         * @see #registerComponent(), #unregisterComponent()
         */
        QList<const ComponentInstance*> mComponentInstances;

        /// @brief The ERC message for unplaced required symbols of this generic component
        QScopedPointer<ErcMsg> mErcMsgUnplacedRequiredSymbols;

        /// @brief The ERC message for unplaced optional symbols of this generic component
        QScopedPointer<ErcMsg> mErcMsgUnplacedOptionalSymbols;
};

} // namespace project

#endif // PROJECT_GENCOMPINSTANCE_H

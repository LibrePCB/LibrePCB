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

#ifndef LIBREPCB_PROJECT_COMPONENTSIGNALINSTANCE_H
#define LIBREPCB_PROJECT_COMPONENTSIGNALINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class XmlDomElement;

namespace library {
class ComponentSignal;
}

namespace project {

class ComponentInstance;
class SI_SymbolPin;
class NetSignal;
class Circuit;
class ErcMsg;

/*****************************************************************************************
 *  Class ComponentSignalInstance
 ****************************************************************************************/

/**
 * @brief The ComponentSignalInstance class
 */
class ComponentSignalInstance final : public QObject, public IF_ErcMsgProvider,
                                      public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(ComponentSignalInstance)

    public:

        // Constructors / Destructor
        explicit ComponentSignalInstance(Circuit& circuit, ComponentInstance& cmpInstance,
                                         const XmlDomElement& domElement) throw (Exception);
        explicit ComponentSignalInstance(Circuit& circuit, ComponentInstance& cmpInstance,
                                         const library::ComponentSignal& cmpSignal,
                                         NetSignal* netsignal = nullptr) throw (Exception);
        ~ComponentSignalInstance() noexcept;

        // Getters
        const library::ComponentSignal& getCompSignal() const noexcept {return *mComponentSignal;}
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}
        bool isNetSignalNameForced() const noexcept;
        QString getForcedNetSignalName() const noexcept;


        // Setters

        /**
         * @brief (Re-)Connect/Disconnect this component signal to/from a circuit's netsignal
         *
         * @warning This method must always be called from inside an UndoCommand!
         *
         * @param netsignal     - (Re-)Connect: A Pointer to the new netsignal
         *                      - Disconnect: 0
         *
         * @throw Exception     This method throws an exception in case of an error
         */
        void setNetSignal(NetSignal* netsignal) throw (Exception);


        // General Methods
        void registerSymbolPin(SI_SymbolPin& pin) throw (Exception);
        void unregisterSymbolPin(SI_SymbolPin& pin) throw (Exception);
        void addToCircuit() throw (Exception);
        void removeFromCircuit() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private slots:

        void netSignalNameChanged(const QString& newName) noexcept;
        void updateErcMessages() noexcept;


    private:

        // make some methods inaccessible...
        ComponentSignalInstance();
        ComponentSignalInstance(const ComponentSignalInstance& other);
        ComponentSignalInstance& operator=(const ComponentSignalInstance& rhs);

        // Private Methods
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        Circuit& mCircuit;
        ComponentInstance& mComponentInstance;

        // Attributes
        const library::ComponentSignal* mComponentSignal;
        QList<SI_SymbolPin*> mRegisteredSymbolPins;
        NetSignal* mNetSignal;
        bool mAddedToCircuit;

        // Misc

        /// @brief The ERC message for an unconnected required component signal
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredSignal;
        /// @brief The ERC message for a global net signal name mismatch
        QScopedPointer<ErcMsg> mErcMsgForcedNetSignalNameConflict;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_COMPONENTSIGNALINSTANCE_H

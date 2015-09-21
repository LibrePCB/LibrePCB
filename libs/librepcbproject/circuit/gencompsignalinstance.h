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

#ifndef PROJECT_GENCOMPSIGNALINSTANCE_H
#define PROJECT_GENCOMPSIGNALINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace project {
class GenCompInstance;
class SI_SymbolPin;
class NetSignal;
class Circuit;
class ErcMsg;
}

namespace library {
class GenCompSignal;
}

/*****************************************************************************************
 *  Class GenCompSignalInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenCompSignalInstance class
 */
class GenCompSignalInstance final : public QObject, public IF_ErcMsgProvider,
                                    public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(GenCompSignalInstance)

    public:

        // Constructors / Destructor
        explicit GenCompSignalInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                       const XmlDomElement& domElement) throw (Exception);
        explicit GenCompSignalInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                       const library::GenCompSignal& genCompSignal,
                                       NetSignal* netsignal = nullptr) throw (Exception);
        ~GenCompSignalInstance() noexcept;

        // Getters
        const library::GenCompSignal& getCompSignal() const noexcept {return *mGenCompSignal;}
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
        GenCompSignalInstance();
        GenCompSignalInstance(const GenCompSignalInstance& other);
        GenCompSignalInstance& operator=(const GenCompSignalInstance& rhs);

        // Private Methods
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        Circuit& mCircuit;
        GenCompInstance& mGenCompInstance;

        // Attributes
        const library::GenCompSignal* mGenCompSignal;
        QList<SI_SymbolPin*> mRegisteredSymbolPins;
        NetSignal* mNetSignal;
        bool mAddedToCircuit;

        // Misc

        /// @brief The ERC message for an unconnected required generic component signal
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredSignal;
        /// @brief The ERC message for a global net signal name mismatch
        QScopedPointer<ErcMsg> mErcMsgForcedNetSignalNameConflict;
};

} // namespace project

#endif // PROJECT_GENCOMPSIGNALINSTANCE_H

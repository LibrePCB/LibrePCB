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

#ifndef PROJECT_GENCOMPSIGNALINSTANCE_H
#define PROJECT_GENCOMPSIGNALINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomElement>
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class GenCompInstance;
class SymbolPinInstance;
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
class GenCompSignalInstance final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenCompSignalInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                       const QDomElement& domElement) throw (Exception);
        ~GenCompSignalInstance() noexcept;

        // Getters
        const library::GenCompSignal& getCompSignal() const noexcept {return *mGenCompSignal;}
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}


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
        void registerSymbolPinInstance(SymbolPinInstance* pin) throw (Exception);
        void unregisterSymbolPinInstance(SymbolPinInstance* pin) throw (Exception);
        void addToCircuit() throw (Exception);
        void removeFromCircuit() throw (Exception);


    private:

        // make some methods inaccessible...
        GenCompSignalInstance();
        GenCompSignalInstance(const GenCompSignalInstance& other);
        GenCompSignalInstance& operator=(const GenCompSignalInstance& rhs);

        // General
        Circuit& mCircuit;
        GenCompInstance& mGenCompInstance;
        QDomElement mDomElement;

        // Attributes
        const library::GenCompSignal* mGenCompSignal;
        QList<SymbolPinInstance*> mSymbolPinInstances;
        NetSignal* mNetSignal;
        bool mAddedToCircuit;

        // Misc

        /// @brief The ERC message for an unconnected required generic component signal
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredSignal;
};

} // namespace project

#endif // PROJECT_GENCOMPSIGNALINSTANCE_H

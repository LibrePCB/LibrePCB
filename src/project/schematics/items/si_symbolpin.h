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

#ifndef PROJECT_SI_SYMBOLPIN_H
#define PROJECT_SI_SYMBOLPIN_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include "../../erc/if_ercmsgprovider.h"
#include "../../../common/units/all_length_units.h"
#include "../graphicsitems/sgi_symbolpin.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QGraphicsItem;

namespace project {
class Circuit;
class GenCompSignalInstance;
class SI_Symbol;
class SI_NetPoint;
class ErcMsg;
}

namespace library {
class SymbolPin;
class GenCompSignal;
}

/*****************************************************************************************
 *  Class SI_SymbolPin
 ****************************************************************************************/

namespace project {

/**
 * @brief The SI_SymbolPin class
 */
class SI_SymbolPin final : public SI_Base, public IF_ErcMsgProvider
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(SI_SymbolPin)

    public:

        // Constructors / Destructor
        explicit SI_SymbolPin(SI_Symbol& symbol, const QUuid& pinUuid, QGraphicsItem& parentItem);
        ~SI_SymbolPin();

        // Getters
        const QUuid& getLibPinUuid() const noexcept;
        Point getPosition() const noexcept;
        QString getDisplayText(bool returnGenCompSignalNameIfEmpty = false,
                               bool returnPinNameIfEmpty = false) const noexcept;
        SI_Symbol& getSymbol() const noexcept {return mSymbol;}
        SI_NetPoint* getNetPoint() const noexcept {return mRegisteredNetPoint;}
        const library::SymbolPin& getLibPin() const noexcept {return *mSymbolPin;}
        const library::GenCompSignal* getGenCompSignal() const noexcept {return mGenCompSignal;}
        GenCompSignalInstance* getGenCompSignalInstance() const noexcept {return mGenCompSignalInstance;}

        // General Methods
        void updateNetPointPosition() noexcept;
        void registerNetPoint(SI_NetPoint& netpoint);
        void unregisterNetPoint(SI_NetPoint& netpoint);
        void addToSchematic() noexcept;
        void removeFromSchematic() noexcept;
        bool save(bool toOriginal, QStringList& errors) noexcept;


    private slots:

        void updateErcMessages() noexcept;


    private:

        // make some methods inaccessible...
        SI_SymbolPin();
        SI_SymbolPin(const SI_SymbolPin& other);
        SI_SymbolPin& operator=(const SI_SymbolPin& rhs);


        // General
        Circuit& mCircuit;
        SI_Symbol& mSymbol;
        const library::SymbolPin* mSymbolPin;
        const library::GenCompSignal* mGenCompSignal;
        GenCompSignalInstance* mGenCompSignalInstance;

        // Misc
        bool mAddedToSchematic;
        SI_NetPoint* mRegisteredNetPoint;
        SGI_SymbolPin* mGraphicsItem;

        /// @brief The ERC message for unconnected required pins
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredPin;
};

} // namespace project

#endif // PROJECT_SI_SYMBOLPIN_H

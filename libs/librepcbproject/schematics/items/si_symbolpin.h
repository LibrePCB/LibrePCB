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

#ifndef PROJECT_SI_SYMBOLPIN_H
#define PROJECT_SI_SYMBOLPIN_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include "../../erc/if_ercmsgprovider.h"
#include "../graphicsitems/sgi_symbolpin.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class ComponentSignalInstance;
class SI_Symbol;
class SI_NetPoint;
class ErcMsg;
}

namespace library {
class SymbolPin;
class ComponentSignal;
class ComponentPinSignalMapItem;
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
        explicit SI_SymbolPin(SI_Symbol& symbol, const Uuid& pinUuid);
        ~SI_SymbolPin();

        // Getters
        Project& getProject() const noexcept;
        Schematic& getSchematic() const noexcept;
        const Uuid& getLibPinUuid() const noexcept;
        QString getDisplayText(bool returnCmpSignalNameIfEmpty = false,
                               bool returnPinNameIfEmpty = false) const noexcept;
        SI_Symbol& getSymbol() const noexcept {return mSymbol;}
        SI_NetPoint* getNetPoint() const noexcept {return mRegisteredNetPoint;}
        const library::SymbolPin& getLibPin() const noexcept {return *mSymbolPin;}
        const library::ComponentSignal* getComponentSignal() const noexcept {return mComponentSignal;}
        ComponentSignalInstance* getComponentSignalInstance() const noexcept {return mComponentSignalInstance;}

        // General Methods
        void updatePosition() noexcept;
        void registerNetPoint(SI_NetPoint& netpoint);
        void unregisterNetPoint(SI_NetPoint& netpoint);
        void addToSchematic(GraphicsScene& scene) noexcept;
        void removeFromSchematic(GraphicsScene& scene) noexcept;

        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::SymbolPin;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


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
        const library::ComponentSignal* mComponentSignal;
        const library::ComponentPinSignalMapItem* mPinSignalMapItem;
        ComponentSignalInstance* mComponentSignalInstance;
        Point mPosition;
        Angle mRotation;

        // Misc
        bool mAddedToSchematic;
        SI_NetPoint* mRegisteredNetPoint;
        SGI_SymbolPin* mGraphicsItem;

        /// @brief The ERC message for unconnected required pins
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredPin;
};

} // namespace project

#endif // PROJECT_SI_SYMBOLPIN_H

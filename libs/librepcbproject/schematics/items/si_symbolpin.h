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

#ifndef LIBREPCB_PROJECT_SI_SYMBOLPIN_H
#define LIBREPCB_PROJECT_SI_SYMBOLPIN_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "si_base.h"
#include "../../erc/if_ercmsgprovider.h"
#include "../graphicsitems/sgi_symbolpin.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class SymbolPin;
class ComponentSignal;
class ComponentPinSignalMapItem;
}

namespace project {

class Circuit;
class ComponentSignalInstance;
class SI_Symbol;
class SI_NetPoint;
class ErcMsg;


/*****************************************************************************************
 *  Class SI_SymbolPin
 ****************************************************************************************/

/**
 * @brief The SI_SymbolPin class
 */
class SI_SymbolPin final : public SI_Base, public IF_ErcMsgProvider
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(SI_SymbolPin)

    public:

        // Constructors / Destructor
        SI_SymbolPin() = delete;
        SI_SymbolPin(const SI_SymbolPin& other) = delete;
        explicit SI_SymbolPin(SI_Symbol& symbol, const Uuid& pinUuid);
        ~SI_SymbolPin();

        // Getters
        const Uuid& getLibPinUuid() const noexcept;
        QString getDisplayText(bool returnCmpSignalNameIfEmpty = false,
                               bool returnPinNameIfEmpty = false) const noexcept;
        SI_Symbol& getSymbol() const noexcept {return mSymbol;}
        SI_NetPoint* getNetPoint() const noexcept {return mRegisteredNetPoint;}
        const library::SymbolPin& getLibPin() const noexcept {return *mSymbolPin;}
        //const library::ComponentSignal* getComponentSignal() const noexcept {return mComponentSignal;}
        ComponentSignalInstance* getComponentSignalInstance() const noexcept {return mComponentSignalInstance;}
        bool isRequired() const noexcept;
        bool isUsed() const noexcept {return mRegisteredNetPoint ? true : false;}

        // General Methods
        void addToSchematic(GraphicsScene& scene) throw (Exception) override;
        void removeFromSchematic(GraphicsScene& scene) throw (Exception) override;
        void registerNetPoint(SI_NetPoint& netpoint) throw (Exception);
        void unregisterNetPoint(SI_NetPoint& netpoint) throw (Exception);
        void updatePosition() noexcept;

        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::SymbolPin;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        SI_SymbolPin& operator=(const SI_SymbolPin& rhs) = delete;


    private slots:

        void updateErcMessages() noexcept;


    private:

        // General
        SI_Symbol& mSymbol;
        const library::SymbolPin* mSymbolPin;
        const library::ComponentPinSignalMapItem* mPinSignalMapItem;
        ComponentSignalInstance* mComponentSignalInstance;

        // Misc
        Point mPosition;
        Angle mRotation;
        SI_NetPoint* mRegisteredNetPoint;
        QScopedPointer<SGI_SymbolPin> mGraphicsItem;

        /// @brief The ERC message for unconnected required pins
        QScopedPointer<ErcMsg> mErcMsgUnconnectedRequiredPin;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SI_SYMBOLPIN_H

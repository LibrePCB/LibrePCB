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

#ifndef PROJECT_SI_NETPOINT_H
#define PROJECT_SI_NETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include "../../../common/file_io/if_xmlserializableobject.h"
#include "../../erc/if_ercmsgprovider.h"
#include "../../../common/units/all_length_units.h"
#include "../graphicsitems/sgi_netpoint.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QGraphicsItem;

namespace project {
class Circuit;
class Schematic;
class NetSignal;
class SI_NetLine;
class SI_Symbol;
class SI_SymbolPin;
class ErcMsg;
}

/*****************************************************************************************
 *  Class SI_NetPoint
 ****************************************************************************************/

namespace project {

/**
 * @brief The SI_NetPoint class
 */
class SI_NetPoint final : public SI_Base, public IF_XmlSerializableObject,
                          public IF_ErcMsgProvider
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(SI_NetPoint)

    public:

        // Constructors / Destructor
        explicit SI_NetPoint(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SI_NetPoint(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception);
        explicit SI_NetPoint(Schematic& schematic, SI_SymbolPin& pin) throw (Exception);
        ~SI_NetPoint() noexcept;

        // Getters
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        bool isAttached() const noexcept {return mAttached;}
        const Point& getPosition() const noexcept {return mPosition;}
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}
        SI_SymbolPin* getSymbolPin() const noexcept {return mSymbolPin;}
        const QList<SI_NetLine*>& getLines() const noexcept {return mLines;}

        // Setters

        /**
         * @brief Change the netsignal of this netpoint
         *
         * @warning - This method must always be called from inside an UndoCommand!
         *          - This method must be called also on attached netpoints
         *
         * @param netsignal     A reference to the new netsignal
         *
         * @throw Exception     This method throws an exception in case of an error
         */
        void setNetSignal(NetSignal& netsignal) throw (Exception);
        void setPosition(const Point& position) noexcept;

        // General Methods
        void detachFromPin() throw (Exception);
        void attachToPin(SI_SymbolPin& pin) throw (Exception);
        void updateLines() const noexcept;
        void registerNetLine(SI_NetLine& netline) noexcept;
        void unregisterNetLine(SI_NetLine& netline) noexcept;
        void addToSchematic() throw (Exception);
        void removeFromSchematic() throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Static Methods
        static uint extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                             QList<SI_NetPoint*>& netpoints,
                                             bool floatingPoints = true,
                                             bool attachedPoints = true,
                                             bool floatingPointsFromFloatingLines = false,
                                             bool attachedPointsFromFloatingLines = false,
                                             bool floatingPointsFromAttachedLines = false,
                                             bool attachedPointsFromAttachedLines = false,
                                             bool attachedPointsFromSymbols = false) noexcept;


    private:

        // make some methods inaccessible...
        SI_NetPoint();
        SI_NetPoint(const SI_NetPoint& other);
        SI_NetPoint& operator=(const SI_NetPoint& rhs);

        // Private Methods
        void init() throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // General
        Circuit& mCircuit;
        Schematic& mSchematic;
        SGI_NetPoint* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        bool mAttached;
        Point mPosition;
        NetSignal* mNetSignal;
        SI_SymbolPin* mSymbolPin;    ///< only needed if mAttached == true

        // Misc
        QList<SI_NetLine*> mLines;    ///< all registered netlines

        /// @brief The ERC message for dead netpoints
        QScopedPointer<ErcMsg> mErcMsgDeadNetPoint;
};

} // namespace project

#endif // PROJECT_SI_NETPOINT_H

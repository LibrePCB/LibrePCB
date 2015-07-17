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

#ifndef PROJECT_SI_NETPOINT_H
#define PROJECT_SI_NETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include "../../erc/if_ercmsgprovider.h"
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
        Project& getProject() const noexcept;
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        bool isAttached() const noexcept {return mAttached;}
        bool isVisible() const noexcept {return ((mLines.count() > 1) && mAttached) || (mLines.count() > 2);}
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
        void addToSchematic(GraphicsScene& scene) throw (Exception);
        void removeFromSchematic(GraphicsScene& scene) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(int version) const throw (Exception) override;


        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::NetPoint;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


    private:

        // make some methods inaccessible...
        SI_NetPoint();
        SI_NetPoint(const SI_NetPoint& other);
        SI_NetPoint& operator=(const SI_NetPoint& rhs);

        // Private Methods
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


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

/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_PROJECT_BI_NETPOINT_H
#define LIBREPCB_PROJECT_BI_NETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/fileio/if_xmlserializableobject.h>
#include <librepcb/common/uuid.h>
#include "../../erc/if_ercmsgprovider.h"
#include "../graphicsitems/bgi_netpoint.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
class QGraphicsItem;

namespace librepcb {

class BoardLayer;

namespace project {

class NetSignal;
class BI_NetLine;
class BI_Footprint;
class BI_FootprintPad;
class BI_Via;
class ErcMsg;

/*****************************************************************************************
 *  Class BI_NetPoint
 ****************************************************************************************/

/**
 * @brief The BI_NetPoint class
 */
class BI_NetPoint final : public BI_Base, public IF_XmlSerializableObject,
                          public IF_ErcMsgProvider
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(BI_NetPoint)

    public:

        // Constructors / Destructor
        BI_NetPoint() = delete;
        BI_NetPoint(const BI_NetPoint& other) = delete;
        BI_NetPoint(Board& board, const BI_NetPoint& other, BI_FootprintPad* pad,
                    BI_Via* via) throw (Exception);
        BI_NetPoint(Board& board, const XmlDomElement& domElement) throw (Exception);
        BI_NetPoint(Board& board, BoardLayer& layer, NetSignal& netsignal,
                    const Point& position) throw (Exception);
        BI_NetPoint(Board& board, BoardLayer& layer, NetSignal& netsignal,
                    BI_FootprintPad& pad) throw (Exception);
        BI_NetPoint(Board& board, BoardLayer& layer, NetSignal& netsignal,
                    BI_Via& via) throw (Exception);
        ~BI_NetPoint() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        BoardLayer& getLayer() const noexcept {return *mLayer;}
        bool isAttachedToPad() const noexcept {return (mFootprintPad ? true : false);}
        bool isAttachedToVia() const noexcept {return (mVia ? true : false);}
        bool isAttached() const noexcept {return (isAttachedToPad() || isAttachedToVia());}
        NetSignal& getNetSignal() const noexcept {return *mNetSignal;}
        BI_FootprintPad* getFootprintPad() const noexcept {return mFootprintPad;}
        BI_Via* getVia() const noexcept {return mVia;}
        const QList<BI_NetLine*>& getLines() const noexcept {return mRegisteredLines;}
        bool isUsed() const noexcept {return (mRegisteredLines.count() > 0);}
        Length getMaxLineWidth() const noexcept;
        bool isSelectable() const noexcept override;

        // Setters

        void setLayer(BoardLayer& layer) throw (Exception);
        void setNetSignal(NetSignal& netsignal) throw (Exception);
        void setPadToAttach(BI_FootprintPad* pad) throw (Exception);
        void setViaToAttach(BI_Via* via) throw (Exception);
        void setPosition(const Point& position) noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;
        void registerNetLine(BI_NetLine& netline) throw (Exception);
        void unregisterNetLine(BI_NetLine& netline) throw (Exception);
        void updateLines() const noexcept;


        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Inherited from SI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::NetPoint;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_NetPoint& operator=(const BI_NetPoint& rhs) = delete;
        bool operator==(const BI_NetPoint& rhs) noexcept {return (this == &rhs);}
        bool operator!=(const BI_NetPoint& rhs) noexcept {return (this != &rhs);}


    private:

        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        QScopedPointer<BGI_NetPoint> mGraphicsItem;
        QMetaObject::Connection mHighlightChangedConnection;

        // Attributes
        Uuid mUuid;
        Point mPosition;
        BoardLayer* mLayer;
        NetSignal* mNetSignal;
        BI_FootprintPad* mFootprintPad; ///< only needed if the netpoint is attached to a pad
        BI_Via* mVia;                   ///< only needed if the netpoint is attached to a via

        // Registered Elements
        QList<BI_NetLine*> mRegisteredLines;    ///< all registered netlines

        // ERC Messages
        /// @brief The ERC message for dead netpoints
        QScopedPointer<ErcMsg> mErcMsgDeadNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_NETPOINT_H

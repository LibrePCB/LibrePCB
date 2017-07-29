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

#ifndef LIBREPCB_PROJECT_BI_NETLINE_H
#define LIBREPCB_PROJECT_BI_NETLINE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/uuid.h>
#include "../graphicsitems/bgi_netline.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class BoardLayer;

namespace project {

class NetSignal;
class BI_NetPoint;

/*****************************************************************************************
 *  Class BI_NetLine
 ****************************************************************************************/

/**
 * @brief The BI_NetLine class
 */
class BI_NetLine final : public BI_Base, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_NetLine() = delete;
        BI_NetLine(const BI_NetLine& other) = delete;
        BI_NetLine(Board& board, const BI_NetLine& other, BI_NetPoint& startPoint,
                   BI_NetPoint& endPoint) throw (Exception);
        BI_NetLine(Board& board, const DomElement& domElement) throw (Exception);
        BI_NetLine(Board& board, BI_NetPoint& startPoint, BI_NetPoint& endPoint,
                   const Length& width) throw (Exception);
        ~BI_NetLine() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Length& getWidth() const noexcept {return mWidth;}
        BI_NetPoint& getStartPoint() const noexcept {return *mStartPoint;}
        BI_NetPoint& getEndPoint() const noexcept {return *mEndPoint;}
        NetSignal& getNetSignal() const noexcept;
        BoardLayer& getLayer() const noexcept;
        bool isAttached() const noexcept;
        bool isAttachedToFootprint() const noexcept;
        bool isAttachedToVia() const noexcept;
        bool isSelectable() const noexcept override;

        // Setters
        void setWidth(const Length& width) noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;
        void updateLine() noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;


        // Inherited from SI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::NetLine;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_NetLine& operator=(const BI_NetLine& rhs) = delete;


    private:

        void init() throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // General
        QScopedPointer<BGI_NetLine> mGraphicsItem;
        Point mPosition; ///< the center of startpoint and endpoint
        QMetaObject::Connection mHighlightChangedConnection;

        // Attributes
        Uuid mUuid;
        BI_NetPoint* mStartPoint;
        BI_NetPoint* mEndPoint;
        Length mWidth;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_NETLINE_H

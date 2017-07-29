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

#ifndef LIBREPCB_PROJECT_BI_VIA_H
#define LIBREPCB_PROJECT_BI_VIA_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/uuid.h>
#include "../graphicsitems/bgi_via.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
class QGraphicsItem;

namespace librepcb {

class BoardLayer;

namespace project {

class NetSignal;
class BI_NetPoint;
class ErcMsg;

/*****************************************************************************************
 *  Class BI_Via
 ****************************************************************************************/

/**
 * @brief The BI_Via class
 */
class BI_Via final : public BI_Base, public SerializableObject
{
        Q_OBJECT

    public:

        // Public Types
        enum class Shape {Round, Square, Octagon};

        // Constructors / Destructor
        BI_Via() = delete;
        BI_Via(const BI_Via& other) = delete;
        BI_Via(Board& board, const BI_Via& other) throw (Exception);
        BI_Via(Board& board, const DomElement& domElement) throw (Exception);
        BI_Via(Board& board, const Point& position, BI_Via::Shape shape,
               const Length& size, const Length& drillDiameter, NetSignal* netsignal) throw (Exception);
        ~BI_Via() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        Shape getShape() const noexcept {return mShape;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        const Length& getSize() const noexcept {return mSize;}
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}
        const QMap<int, BI_NetPoint*>& getNetPoints() const noexcept {return mRegisteredNetPoints;}
        BI_NetPoint* getNetPointOfLayer(int layerId) const noexcept {return mRegisteredNetPoints.value(layerId, nullptr);}
        bool isUsed() const noexcept {return (mRegisteredNetPoints.count() > 0);}
        bool isOnLayer(int layerId) const noexcept;
        QPainterPath toQPainterPathPx(const Length& clearance, bool hole) const noexcept;
        bool isSelectable() const noexcept override;

        // Setters
        void setNetSignal(NetSignal* netsignal) throw (Exception);
        void setPosition(const Point& position) noexcept;
        void setShape(Shape shape) noexcept;
        void setSize(const Length& size) noexcept;
        void setDrillDiameter(const Length& diameter) noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;
        void registerNetPoint(BI_NetPoint& netpoint) throw (Exception);
        void unregisterNetPoint(BI_NetPoint& netpoint) throw (Exception);
        void updateNetPoints() const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;


        // Inherited from SI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Via;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_Via& operator=(const BI_Via& rhs) = delete;
        bool operator==(const BI_Via& rhs) noexcept {return (this == &rhs);}
        bool operator!=(const BI_Via& rhs) noexcept {return (this != &rhs);}


    private:

        void init() throw (Exception);
        void boardAttributesChanged();
        bool checkAttributesValidity() const noexcept;


        // General
        QScopedPointer<BGI_Via> mGraphicsItem;
        QMetaObject::Connection mHighlightChangedConnection;

        // Attributes
        Uuid mUuid;
        Point mPosition;
        Shape mShape;
        Length mSize;
        Length mDrillDiameter;
        NetSignal* mNetSignal;

        // Registered Elements
        QMap<int, BI_NetPoint*> mRegisteredNetPoints;   ///< key: layer ID
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_VIA_H

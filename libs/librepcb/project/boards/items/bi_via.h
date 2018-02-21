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
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/uuid.h>
#include "../graphicsitems/bgi_via.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
class QGraphicsItem;

namespace librepcb {

class BoardLayer;

namespace project {

class BI_NetSegment;
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
        BI_Via(BI_NetSegment& netsegment, const BI_Via& other);
        BI_Via(BI_NetSegment& netsegment, const SExpression& node);
        BI_Via(BI_NetSegment& netsegment, const Point& position, BI_Via::Shape shape,
               const Length& size, const Length& drillDiameter);
        ~BI_Via() noexcept;

        // Getters
        BI_NetSegment& getNetSegment() const noexcept {return mNetSegment;}
        NetSignal& getNetSignalOfNetSegment() const noexcept;
        const Uuid& getUuid() const noexcept {return mUuid;}
        Shape getShape() const noexcept {return mShape;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        const Length& getSize() const noexcept {return mSize;}
        const QMap<QString, BI_NetPoint*>& getNetPoints() const noexcept {return mRegisteredNetPoints;}
        BI_NetPoint* getNetPointOfLayer(const QString& layerName) const noexcept {return mRegisteredNetPoints.value(layerName, nullptr);}
        bool isUsed() const noexcept {return (mRegisteredNetPoints.count() > 0);}
        bool isOnLayer(const QString& layerName) const noexcept;
        bool isSelectable() const noexcept override;
        Path getOutline(const Length& expansion = Length(0)) const noexcept;
        Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;
        QPainterPath toQPainterPathPx(const Length& expansion = Length(0)) const noexcept;

        // Setters
        void setPosition(const Point& position) noexcept;
        void setShape(Shape shape) noexcept;
        void setSize(const Length& size) noexcept;
        void setDrillDiameter(const Length& diameter) noexcept;

        // General Methods
        void addToBoard() override;
        void removeFromBoard() override;
        void registerNetPoint(BI_NetPoint& netpoint);
        void unregisterNetPoint(BI_NetPoint& netpoint);
        void updateNetPoints() const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;


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

        void init();
        void boardAttributesChanged();
        bool checkAttributesValidity() const noexcept;


        // General
        BI_NetSegment& mNetSegment;
        QScopedPointer<BGI_Via> mGraphicsItem;
        QMetaObject::Connection mHighlightChangedConnection;

        // Attributes
        Uuid mUuid;
        Point mPosition;
        Shape mShape;
        Length mSize;
        Length mDrillDiameter;

        // Registered Elements
        QMap<QString, BI_NetPoint*> mRegisteredNetPoints;   ///< key: layer name
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_VIA_H

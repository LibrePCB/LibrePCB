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

#ifndef LIBREPCB_PROJECT_BI_FOOTPRINTPAD_H
#define LIBREPCB_PROJECT_BI_FOOTPRINTPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include "../graphicsitems/bgi_footprintpad.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class FootprintPad;
class ComponentSignal;
}

namespace project {

class BI_Footprint;
class BI_NetPoint;
class Circuit;
class ComponentSignalInstance;

/*****************************************************************************************
 *  Class BI_FootprintPad
 ****************************************************************************************/

/**
 * @brief The BI_FootprintPad class
 */
class BI_FootprintPad final : public BI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_FootprintPad() = delete;
        BI_FootprintPad(const BI_FootprintPad& other) = delete;
        BI_FootprintPad(BI_Footprint& footprint, const Uuid& padUuid);
        ~BI_FootprintPad();

        // Getters
        const Uuid& getLibPadUuid() const noexcept;
        QString getDisplayText() const noexcept;
        const Angle& getRotation() const noexcept {return mRotation;}
        BI_Footprint& getFootprint() const noexcept {return mFootprint;}
        const QMap<QString, BI_NetPoint*>& getNetPoints() const noexcept {return mRegisteredNetPoints;}
        BI_NetPoint* getNetPointOfLayer(const QString& layerName) const noexcept {return mRegisteredNetPoints.value(layerName, nullptr);}
        QString getLayerName() const noexcept;
        bool isOnLayer(const QString& layerName) const noexcept;
        const library::FootprintPad& getLibPad() const noexcept {return *mFootprintPad;}
        ComponentSignalInstance* getComponentSignalInstance() const noexcept {return mComponentSignalInstance;}
        NetSignal* getCompSigInstNetSignal() const noexcept;
        bool isUsed() const noexcept {return (mRegisteredNetPoints.count() > 0);}
        bool isSelectable() const noexcept override;

        // General Methods
        void addToBoard(GraphicsScene& scene) override;
        void removeFromBoard(GraphicsScene& scene) override;
        void registerNetPoint(BI_NetPoint& netpoint);
        void unregisterNetPoint(BI_NetPoint& netpoint);
        void updatePosition() noexcept;


        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::FootprintPad;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override;
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_FootprintPad& operator=(const BI_FootprintPad& rhs) = delete;


    private slots:

        void footprintAttributesChanged();


    private:

        void updateGraphicsItemTransform() noexcept;


        // General
        BI_Footprint& mFootprint;
        const library::FootprintPad* mFootprintPad;
        const library::PackagePad* mPackagePad;
        ComponentSignalInstance* mComponentSignalInstance;
        QMetaObject::Connection mHighlightChangedConnection;

        // Misc
        Point mPosition;
        Angle mRotation;
        QMap<QString, BI_NetPoint*> mRegisteredNetPoints; ///< key: layer name
        QScopedPointer<BGI_FootprintPad> mGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_FOOTPRINTPAD_H

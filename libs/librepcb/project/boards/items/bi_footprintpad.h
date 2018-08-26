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
#include "./bi_netline.h"
#include "../graphicsitems/bgi_footprintpad.h"
#include <librepcb/common/geometry/path.h>

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
class ComponentSignalInstance;

/*****************************************************************************************
 *  Class BI_FootprintPad
 ****************************************************************************************/

/**
 * @brief The BI_FootprintPad class
 */
class BI_FootprintPad final : public BI_Base, public BI_NetLineAnchor
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
        QString getLayerName() const noexcept;
        bool isOnLayer(const QString& layerName) const noexcept;
        const library::FootprintPad& getLibPad() const noexcept {return *mFootprintPad;}
        ComponentSignalInstance* getComponentSignalInstance() const noexcept {return mComponentSignalInstance;}
        NetSignal* getCompSigInstNetSignal() const noexcept;
        bool isUsed() const noexcept {return (mRegisteredNetLines.count() > 0);}
        bool isSelectable() const noexcept override;
        Path getOutline(const Length& expansion = Length(0)) const noexcept;
        Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;

        // General Methods
        void addToBoard() override;
        void removeFromBoard() override;
        void updatePosition() noexcept;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::FootprintPad;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override;
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Inherited from BI_NetLineAnchor
        void registerNetLine(BI_NetLine& netline) override;
        void unregisterNetLine(BI_NetLine& netline) override;
        const QSet<BI_NetLine*>& getNetLines() const noexcept override {return mRegisteredNetLines;}

        // Operator Overloadings
        BI_FootprintPad& operator=(const BI_FootprintPad& rhs) = delete;


    private slots:

        void footprintAttributesChanged();
        void componentSignalInstanceNetSignalChanged(NetSignal* from, NetSignal* to);


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
        QScopedPointer<BGI_FootprintPad> mGraphicsItem;

        // Registered Elements
        QSet<BI_NetLine*> mRegisteredNetLines;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_FOOTPRINTPAD_H

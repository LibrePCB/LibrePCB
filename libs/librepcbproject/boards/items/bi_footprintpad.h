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
}

namespace project {

class BI_Footprint;
class Circuit;

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
        //QString getDisplayText(bool returnCmpSignalNameIfEmpty = false,
        //                       bool returnPinNameIfEmpty = false) const noexcept;
        BI_Footprint& getFootprint() const noexcept {return mFootprint;}
        //SI_NetPoint* getNetPoint() const noexcept {return mRegisteredNetPoint;}
        const library::FootprintPad& getLibPad() const noexcept {return *mFootprintPad;}
        //const library::ComponentSignal* getComponentSignal() const noexcept {return mComponentSignal;}
        //ComponentSignalInstance* getComponentSignalInstance() const noexcept {return mComponentSignalInstance;}

        // General Methods
        void updatePosition() noexcept;
        //void registerNetPoint(SI_NetPoint& netpoint);
        //void unregisterNetPoint(SI_NetPoint& netpoint);
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;

        // Inherited from SI_Base
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
        //const library::ComponentSignal* mComponentSignal;
        //ComponentSignalInstance* mComponentSignalInstance;
        Point mPosition;
        Angle mRotation;

        // Misc
        //SI_NetPoint* mRegisteredNetPoint;
        QScopedPointer<BGI_FootprintPad> mGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_FOOTPRINTPAD_H

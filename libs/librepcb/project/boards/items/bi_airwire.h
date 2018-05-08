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

#ifndef LIBREPCB_PROJECT_BI_AIRWIRE_H
#define LIBREPCB_PROJECT_BI_AIRWIRE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include "../graphicsitems/bgi_airwire.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Class BI_AirWire
 ****************************************************************************************/

/**
 * @brief The BI_AirWire class
 */
class BI_AirWire final : public BI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_AirWire() = delete;
        BI_AirWire(const BI_AirWire& other) = delete;
        BI_AirWire(Board& board, const NetSignal& netsignal,
                   const Point& p1, const Point& p2);
        ~BI_AirWire() noexcept;

        // Getters
        const NetSignal& getNetSignal() const noexcept {return mNetSignal;}
        const Point& getP1() const noexcept {return mP1;}
        const Point& getP2() const noexcept {return mP2;}
        bool isVertical() const noexcept {return mP1 == mP2;}

        // General Methods
        void addToBoard() override;
        void removeFromBoard() override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::AirWire;}
        const Point& getPosition() const noexcept override {return mP1;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;
        bool isSelectable() const noexcept override;

        // Operator Overloadings
        BI_AirWire& operator=(const BI_AirWire& rhs) = delete;


    private:
        QScopedPointer<BGI_AirWire> mGraphicsItem;
        QMetaObject::Connection mHighlightChangedConnection;
        const NetSignal& mNetSignal;
        Point mP1;
        Point mP2;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_AIRWIRE_H

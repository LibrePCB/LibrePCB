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

#ifndef LIBREPCB_PROJECT_BGI_VIA_H
#define LIBREPCB_PROJECT_BGI_VIA_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "bgi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class BI_Via;

/*****************************************************************************************
 *  Class BGI_Via
 ****************************************************************************************/

/**
 * @brief The BGI_Via class
 */
class BGI_Via final : public BGI_Base
{
    public:

        // Constructors / Destructor
        explicit BGI_Via(BI_Via& via) noexcept;
        ~BGI_Via() noexcept;

        // Getters
        bool isSelectable() const noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        BGI_Via() = delete;
        BGI_Via(const BGI_Via& other) = delete;
        BGI_Via& operator=(const BGI_Via& rhs) = delete;

        // Private Methods
        GraphicsLayer* getLayer(const QString& name) const noexcept;


        // General Attributes
        BI_Via& mVia;
        GraphicsLayer* mViaLayer;
        GraphicsLayer* mTopStopMaskLayer;
        GraphicsLayer* mBottomStopMaskLayer;

        // Cached Attributes
        bool mDrawStopMask;
        QPainterPath mShape;
        QPainterPath mCopper;
        QPainterPath mStopMask;
        QPainterPath mCreamMask;
        QRectF mBoundingRect;
        QFont mFont;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BGI_VIA_H

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

#ifndef LIBREPCB_PROJECT_SGI_NETLABEL_H
#define LIBREPCB_PROJECT_SGI_NETLABEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "sgi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class SchematicLayer;

namespace project {

class SI_NetLabel;

/*****************************************************************************************
 *  Class SGI_NetLabel
 ****************************************************************************************/

/**
 * @brief The SGI_NetLabel class
 */
class SGI_NetLabel final : public SGI_Base
{
    public:

        // Constructors / Destructor
        explicit SGI_NetLabel(SI_NetLabel& netlabel) noexcept;
        ~SGI_NetLabel() noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const {return mBoundingRect;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        SGI_NetLabel() = delete;
        SGI_NetLabel(const SGI_NetLabel& other) = delete;
        SGI_NetLabel& operator=(const SGI_NetLabel& rhs) = delete;

        // Private Methods
        SchematicLayer* getSchematicLayer(int id) const noexcept;


        // Attributes
        SI_NetLabel& mNetLabel;

        // Cached Attributes
        QStaticText mStaticText;
        QFont mFont;
        bool mRotate180;
        QPointF mTextOrigin;
        QRectF mBoundingRect;

        // Static Stuff
        static QVector<QLineF> sOriginCrossLines;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SGI_NETLABEL_H

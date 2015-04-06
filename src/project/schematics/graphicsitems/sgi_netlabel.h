/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_SGI_NETLABEL_H
#define PROJECT_SGI_NETLABEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "sgi_base.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace project {
class SI_NetLabel;
}

/*****************************************************************************************
 *  Class SGI_NetLabel
 ****************************************************************************************/

namespace project {

/**
 * @brief The SGI_NetLabel class
 */
class SGI_NetLabel final : public SGI_Base
{
    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = Schematic::Type_NetLabel};

        // Constructors / Destructor
        explicit SGI_NetLabel(SI_NetLabel& netlabel) noexcept;
        ~SGI_NetLabel() noexcept;

        // Getters
        SI_NetLabel& getNetLabel() const {return mNetLabel;}

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        QRectF boundingRect() const {return mBoundingRect;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        SGI_NetLabel() = delete;
        SGI_NetLabel(const SGI_NetLabel& other) = delete;
        SGI_NetLabel& operator=(const SGI_NetLabel& rhs) = delete;

        // Attributes
        SI_NetLabel& mNetLabel;
        SchematicLayer* mOriginCrossLayer;
        SchematicLayer* mTextLayer;

        // Cached Attributes
        QFont mFont;
        bool mRotate180;
        int mFlags;
        QRectF mBoundingRect;

        // Static Stuff
        static QVector<QLineF> sOriginCrossLines;
};

} // namespace project

#endif // PROJECT_SGI_NETLABEL_H

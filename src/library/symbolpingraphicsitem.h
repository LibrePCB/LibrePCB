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

#ifndef LIBRARY_SYMBOLPINGRAPHICSITEM_H
#define LIBRARY_SYMBOLPINGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QGraphicsItem>
#include "../common/exceptions.h"
#include "../common/units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace library {
class SymbolPin;
class SymbolGraphicsItem;
}
namespace project {
class SymbolPinInstance;
}

/*****************************************************************************************
 *  Class SymbolPinGraphicsItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPinGraphicsItem class
 *
 * @todo The methods #paint() and #updateBoundingRectAndShape() are not yet finished.
 */
class SymbolPinGraphicsItem final : public QGraphicsItem
{
    public:

        // Constructors / Destructor
        explicit SymbolPinGraphicsItem(SymbolGraphicsItem& symbol, const SymbolPin& pin,
                                       project::SymbolPinInstance* instance = 0) throw (Exception);
        ~SymbolPinGraphicsItem() noexcept;


        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
        QVariant itemChange(GraphicsItemChange change, const QVariant& value);


    private:

        // make some methods inaccessible...
        SymbolPinGraphicsItem();
        SymbolPinGraphicsItem(const SymbolPinGraphicsItem& other);
        SymbolPinGraphicsItem& operator=(const SymbolPinGraphicsItem& rhs);

        // Private Methods
        bool updateBoundingRectAndShape() noexcept;
        SchematicLayer* getSchematicLayer(unsigned int id) const noexcept;


        // General Attributes
        SymbolGraphicsItem& mSymbolGraphicsItem;
        const SymbolPin& mPin;
        project::SymbolPinInstance* mPinInstance;

        // Symbol Pin Graphics Item Attributes
        QRectF mBoundingRect;
        QPainterPath mShape;
};

} // namespace library

#endif // LIBRARY_SYMBOLPINGRAPHICSITEM_H

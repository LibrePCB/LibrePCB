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

#ifndef LIBRARY_SYMBOLGRAPHICSITEM_H
#define LIBRARY_SYMBOLGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QGraphicsItem>
#include "../common/exceptions.h"
#include "../common/units.h"
#include "../common/cadscene.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace library {
class Symbol;
class SymbolPinGraphicsItem;
}
namespace project {
class SymbolInstance;
}

/*****************************************************************************************
 *  Class SymbolGraphicsItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolGraphicsItem class
 *
 * @todo The methods #paint() and #updateBoundingRectAndShape() are not yet finished.
 */
class SymbolGraphicsItem final : public QGraphicsItem
{
    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = CADScene::Type_Symbol};

        // Constructors / Destructor
        explicit SymbolGraphicsItem(const Symbol& symbol,
                                    project::SymbolInstance* instance = 0) throw (Exception);
        ~SymbolGraphicsItem() noexcept;

        // Getters
        project::SymbolInstance* getSymbolInstance() const noexcept {return mSymbolInstance;}

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    private:

        // make some methods inaccessible...
        SymbolGraphicsItem();
        SymbolGraphicsItem(const SymbolGraphicsItem& other);
        SymbolGraphicsItem& operator=(const SymbolGraphicsItem& rhs);

        // Private Methods
        bool updateBoundingRectAndShape() noexcept;
        SchematicLayer* getSchematicLayer(unsigned int id) const noexcept;


        // General Attributes
        const Symbol& mSymbol;
        project::SymbolInstance* mSymbolInstance;

        // Symbol Graphics Item Attributes
        QRectF mBoundingRect;
        QPainterPath mShape;
        QHash<QUuid, SymbolPinGraphicsItem*> mPinItems;
};

} // namespace library

#endif // LIBRARY_SYMBOLGRAPHICSITEM_H

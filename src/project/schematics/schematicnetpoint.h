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

#ifndef PROJECT_SCHEMATICNETPOINT_H
#define PROJECT_SCHEMATICNETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QDomElement>
#include "../../common/cadscene.h"
#include "../../common/units.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace project {
class Schematic;
class NetSignal;
class SchematicNetLine;
class SchematicNetPoint;
class SymbolInstance;
class SymbolPinInstance;
}

/*****************************************************************************************
 *  Class SchematicNetPointGraphicsItem
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetPointGraphicsItem class
 */
class SchematicNetPointGraphicsItem final : public QGraphicsItem
{
    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = CADScene::Type_SchematicNetPoint};

        // Constructors / Destructor
        explicit SchematicNetPointGraphicsItem(Schematic& schematic,
                                               SchematicNetPoint& point) throw (Exception);

        ~SchematicNetPointGraphicsItem() noexcept;

        // Getters
        SchematicNetPoint& getNetPoint() const {return mPoint;}

        // Inherited from QGraphicsItem
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        QRectF boundingRect() const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        SchematicNetPointGraphicsItem();
        SchematicNetPointGraphicsItem(const SchematicNetPointGraphicsItem& other);
        SchematicNetPointGraphicsItem& operator=(const SchematicNetPointGraphicsItem& rhs);

        // Attributes
        Schematic& mSchematic;
        SchematicNetPoint& mPoint;
        SchematicLayer* mLayer;
};

} // namespace project

/*****************************************************************************************
 *  Class SchematicNetPoint
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetPoint class
 */
class SchematicNetPoint final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicNetPoint(Schematic& schematic, const QDomElement& domElement)
                                   throw (Exception);
        ~SchematicNetPoint() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        NetSignal* getNetSignal() const noexcept;
        bool isAttached() const noexcept {return mAttached;}
        const Point& getPosition() const noexcept {return mPosition;}
        const QList<SchematicNetLine*>& getLines() const noexcept {return mLines;}

        // Setters
        void setPosition(const Point& position) noexcept;

        // General Methods
        void updateLines() const noexcept;
        void registerNetLine(SchematicNetLine* netline) noexcept;
        void unregisterNetLine(SchematicNetLine* netline) noexcept;
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;

        // Static Methods
        static const Length& getCircleRadius() noexcept {return sCircleRadius;}
        static SchematicNetPoint* create(Schematic& schematic, QDomDocument& doc,
                                         const QUuid& netsignal, const Point& position)
                                         throw (Exception);
        static SchematicNetPoint* create(Schematic& schematic, QDomDocument& doc,
                                         const QUuid& symbol, const QUuid& pin)
                                         throw (Exception);

    private:

        // make some methods inaccessible...
        SchematicNetPoint();
        SchematicNetPoint(const SchematicNetPoint& other);
        SchematicNetPoint& operator=(const SchematicNetPoint& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        SchematicNetPointGraphicsItem* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        NetSignal* mNetSignal;
        bool mAttached;
        Point mPosition;
        SymbolInstance* mSymbolInstance;
        SymbolPinInstance* mPinInstance;

        QList<SchematicNetLine*> mLines;

        // Static Members
        static const Length sCircleRadius;
};

} // namespace project

#endif // PROJECT_SCHEMATICNETPOINT_H

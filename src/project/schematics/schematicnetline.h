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

#ifndef PROJECT_SCHEMATICNETLINE_H
#define PROJECT_SCHEMATICNETLINE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QDomElement>
#include "../../common/cadscene.h"
#include "../../common/units/all_length_units.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;

namespace project {
class NetSignal;
class Schematic;
class SchematicNetPoint;
class SchematicNetLine;
}

/*****************************************************************************************
 *  Class SchematicNetLineGraphicsItem
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetLineGraphicsItem class
 */
class SchematicNetLineGraphicsItem final : public QGraphicsLineItem
{
        Q_DECLARE_TR_FUNCTIONS(SchematicNetLineGraphicsItem)

    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = CADScene::Type_SchematicNetLine};

        // Constructors / Destructor
        explicit SchematicNetLineGraphicsItem(Schematic& schematic,
                                              SchematicNetLine& line) throw (Exception);

        ~SchematicNetLineGraphicsItem() noexcept;

        // Getters
        SchematicNetLine& getNetLine() const {return mLine;}

        // Inherited from QGraphicsItem
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        QPainterPath shape() const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    private:

        // make some methods inaccessible...
        SchematicNetLineGraphicsItem();
        SchematicNetLineGraphicsItem(const SchematicNetLineGraphicsItem& other);
        SchematicNetLineGraphicsItem& operator=(const SchematicNetLineGraphicsItem& rhs);

        // Attributes
        Schematic& mSchematic;
        SchematicNetLine& mLine;
        SchematicLayer* mLayer;
};

} // namespace project

/*****************************************************************************************
 *  Class SchematicNetLine
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetLine class
 */
class SchematicNetLine final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicNetLine(Schematic& schematic, const QDomElement& domElement)
                                  throw (Exception);
        ~SchematicNetLine() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Length& getWidth() const noexcept {return mWidth;}
        SchematicNetPoint& getStartPoint() const noexcept {return *mStartPoint;}
        SchematicNetPoint& getEndPoint() const noexcept {return *mEndPoint;}
        NetSignal* getNetSignal() const noexcept;
        bool isAttachedToSymbol() const noexcept;

        // Setters
        void setWidth(const Length& width) noexcept;

        // General Methods
        void updateLine() noexcept;
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;

        // Static Methods
        static uint extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                             QList<SchematicNetLine*>& netlines,
                                             bool floatingLines,
                                             bool attachedLines,
                                             bool attachedLinesFromSymbols = false) noexcept;
        static SchematicNetLine* create(Schematic& schematic, QDomDocument& doc,
                                        const QUuid& startPoint, const QUuid& endPoint,
                                        const Length& width) throw (Exception);

    private:

        // make some methods inaccessible...
        SchematicNetLine();
        SchematicNetLine(const SchematicNetLine& other);
        SchematicNetLine& operator=(const SchematicNetLine& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        SchematicNetLineGraphicsItem* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        SchematicNetPoint* mStartPoint;
        SchematicNetPoint* mEndPoint;
        Length mWidth;
};

} // namespace project

#endif // PROJECT_SCHEMATICNETLINE_H

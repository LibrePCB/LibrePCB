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

#ifndef PROJECT_SCHEMATICNETLABEL_H
#define PROJECT_SCHEMATICNETLABEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../../common/file_io/if_xmlserializableobject.h"
#include "../../common/cadscene.h"
#include "../../common/units/all_length_units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class Schematic;
class NetSignal;
class SchematicNetLabel;
}

/*****************************************************************************************
 *  Class SchematicNetLabelGraphicsItem
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetLabelGraphicsItem class
 */
class SchematicNetLabelGraphicsItem final : public QGraphicsItem
{
        Q_DECLARE_TR_FUNCTIONS(SchematicNetLabelGraphicsItem)

    public:

        // Types

        /// to make  qgraphicsitem_cast() working
        enum {Type = CADScene::Type_SchematicNetLabel};

        // Constructors / Destructor
        explicit SchematicNetLabelGraphicsItem(Schematic& schematic, SchematicNetLabel& label) throw (Exception);
        ~SchematicNetLabelGraphicsItem() noexcept;

        // Getters
        SchematicNetLabel& getNetLabel() const {return mLabel;}

        // Inherited from QGraphicsItem
        int type() const {return Type;} ///< to make  qgraphicsitem_cast() working
        QRectF boundingRect() const;
        QPainterPath shape() const noexcept;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);


    private:

        // make some methods inaccessible...
        SchematicNetLabelGraphicsItem();
        SchematicNetLabelGraphicsItem(const SchematicNetLabelGraphicsItem& other);
        SchematicNetLabelGraphicsItem& operator=(const SchematicNetLabelGraphicsItem& rhs);

        // Attributes
        Schematic& mSchematic;
        SchematicNetLabel& mLabel;
        QFont mFont;
};

} // namespace project

/*****************************************************************************************
 *  Class SchematicNetLabel
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicNetLabel class
 */
class SchematicNetLabel final : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicNetLabel(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SchematicNetLabel(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception);
        ~SchematicNetLabel() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getAngle() const noexcept {return mAngle;}
        NetSignal& getNetSignal() const noexcept {return *mNetSignal;}

        // Setters
        void setNetSignal(NetSignal& netsignal) noexcept;
        void setPosition(const Point& position) noexcept;
        void setAngle(const Angle& angle) noexcept;

        // General Methods
        void updateText() noexcept;
        void addToSchematic() throw (Exception);
        void removeFromSchematic() throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Static Methods
        static uint extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                             QList<SchematicNetLabel*>& netlabels) noexcept;


    private:

        // make some methods inaccessible...
        SchematicNetLabel() = delete;
        SchematicNetLabel(const SchematicNetLabel& other) = delete;
        SchematicNetLabel& operator=(const SchematicNetLabel& rhs) = delete;

        // Private Methods
        void init() throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // General
        Circuit& mCircuit;
        Schematic& mSchematic;
        SchematicNetLabelGraphicsItem* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        Point mPosition;
        Angle mAngle;
        NetSignal* mNetSignal;
};

} // namespace project

#endif // PROJECT_SCHEMATICNETLABEL_H

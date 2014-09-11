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
#include <QDomElement>
#include "../../common/units.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QGraphicsEllipseItem;
class SchematicLayer;

namespace project {
class Schematic;
class NetSignal;
class SchematicNetLine;
}

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
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}
        const Point& getPosition() const noexcept {return mPosition;}
        int getLinesCount() const noexcept {return mLines.count();}

        // Setters
        void setPosition(const Point& position) noexcept;

        // General Methods
        void registerNetLine(SchematicNetLine* netline) noexcept;
        void unregisterNetLine(SchematicNetLine* netline) noexcept;
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);

        // Static Methods
        static SchematicNetPoint* create(Schematic& schematic, QDomDocument& doc,
                                         const QUuid& netsignal, const Point& position) throw (Exception);

    private:

        // make some methods inaccessible...
        SchematicNetPoint();
        SchematicNetPoint(const SchematicNetPoint& other);
        SchematicNetPoint& operator=(const SchematicNetPoint& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        QGraphicsEllipseItem* mItem;
        SchematicLayer* mLayer;

        // Attributes
        QUuid mUuid;
        NetSignal* mNetSignal;
        bool mAttached;
        Point mPosition;

        QList<SchematicNetLine*> mLines;
};

} // namespace project

#endif // PROJECT_SCHEMATICNETPOINT_H

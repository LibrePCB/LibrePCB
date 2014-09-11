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
#include <QDomElement>
#include "../../common/units.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QGraphicsLineItem;
class SchematicLayer;

namespace project {
class Schematic;
class SchematicNetPoint;
}

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

        // General Methods
        void updateLine() noexcept;
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);

        // Static Methods
        static SchematicNetLine* create(Schematic& schematic, QDomDocument& doc,
                                        const QUuid& startPoint, const QUuid& endPoint) throw (Exception);

    private:

        // make some methods inaccessible...
        SchematicNetLine();
        SchematicNetLine(const SchematicNetLine& other);
        SchematicNetLine& operator=(const SchematicNetLine& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        QGraphicsLineItem* mItem;
        SchematicLayer* mLayer;

        // Attributes
        QUuid mUuid;
        SchematicNetPoint* mStartPoint;
        SchematicNetPoint* mEndPoint;
};

} // namespace project

#endif // PROJECT_SCHEMATICNETLINE_H

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

#ifndef PROJECT_SYMBOLINSTANCE_H
#define PROJECT_SYMBOLINSTANCE_H

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

class QGraphicsRectItem;
class SchematicLayer;

namespace project {
class Schematic;
class GenericComponentInstance;
}

namespace library {
class Symbol;
}

/*****************************************************************************************
 *  Class SymbolInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The SymbolInstance class
 *
 * @author ubruhin
 * @date 2014-08-23
 */
class SymbolInstance final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SymbolInstance(Schematic& schematic, const QDomElement& domElement)
                                throw (Exception);
        ~SymbolInstance() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}

        // General Methods
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);

        // Static Methods
        static SymbolInstance* create(Schematic& schematic, QDomDocument& doc,
                                      const QUuid& genCompInstance,
                                      const QUuid& symbolItem) throw (Exception);

    private:

        // make some methods inaccessible...
        SymbolInstance();
        SymbolInstance(const SymbolInstance& other);
        SymbolInstance& operator=(const SymbolInstance& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        QGraphicsRectItem* mItem;
        SchematicLayer* mOutlineLayer;

        // Attributes
        QUuid mUuid;
        GenericComponentInstance* mGenCompInstance;
        Point mPosition;
};

} // namespace project

#endif // PROJECT_SYMBOLINSTANCE_H

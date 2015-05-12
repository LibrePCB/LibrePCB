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

#ifndef PROJECT_SCHEMATICCLIPBOARD_H
#define PROJECT_SCHEMATICCLIPBOARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace project {
class Schematic;
}

/*****************************************************************************************
 *  Class SchematicClipboard
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicClipboard class
 *
 * @author ubruhin
 * @author 2015-03-07
 */
class SchematicClipboard final : public QObject
{
        Q_OBJECT

    public:

        // General Methods
        //void clear() noexcept;
        //void cut(const QList<SymbolInstance*>& symbols) throw (Exception);
        //void copy(const QList<SymbolInstance*>& symbols) throw (Exception);
        //void paste(Schematic& schematic, QList<SymbolInstance*>& symbols) throw (Exception);


        // Static Methods
        static SchematicClipboard& instance() noexcept {static SchematicClipboard i; return i;}

    private:

        // Private Methods
        SchematicClipboard() noexcept;
        ~SchematicClipboard() noexcept;
        //void setElements(const QList<SymbolInstance*>& symbols) throw (Exception);


        // Attributes
        bool mCutActive;
        //QList<XmlDomElement*> mSymbolInstances;

};

} // namespace project

#endif // PROJECT_SCHEMATICCLIPBOARD_H

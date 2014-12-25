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

#ifndef PROJECT_SYMBOLPININSTANCE_H
#define PROJECT_SYMBOLPININSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class SymbolInstance;
class GenCompSignalInstance;
class SchematicNetPoint;
class SchematicNetPointGraphicsItem;
}

namespace library {
class SymbolPin;
class GenCompSignal;
class SymbolPinGraphicsItem;
}

/*****************************************************************************************
 *  Class SymbolPinInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The SymbolPinInstance class
 */
class SymbolPinInstance final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SymbolPinInstance(SymbolInstance& symbolInstance, const QUuid& pinUuid);
        ~SymbolPinInstance();

        // Getters
        const QUuid& getLibPinUuid() const noexcept;
        Point getPosition() const noexcept;
        QString getDisplayText() const noexcept;
        SymbolInstance& getSymbolInstance() const noexcept {return mSymbolInstance;}
        SchematicNetPoint* getSchematicNetPoint() const noexcept {return mRegisteredSchematicNetPoint;}
        const library::SymbolPin& getSymbolPin() const noexcept {return *mSymbolPin;}
        const library::GenCompSignal* getGenCompSignal() const noexcept {return mGenCompSignal;}
        GenCompSignalInstance* getGenCompSignalInstance() const noexcept {return mGenCompSignalInstance;}

        // General Methods
        void updateNetPointPosition() noexcept;
        void registerNetPoint(SchematicNetPoint* netpoint);
        void unregisterNetPoint(SchematicNetPoint* netpoint);
        void registerPinGraphicsItem(library::SymbolPinGraphicsItem* item);
        void unregisterPinGraphicsItem(library::SymbolPinGraphicsItem* item);
        void addToSchematic() noexcept;
        void removeFromSchematic() noexcept;
        bool save(bool toOriginal, QStringList& errors) noexcept;


    private:

        // make some methods inaccessible...
        SymbolPinInstance();
        SymbolPinInstance(const SymbolPinInstance& other);
        SymbolPinInstance& operator=(const SymbolPinInstance& rhs);


        // General
        SymbolInstance& mSymbolInstance;
        const library::SymbolPin* mSymbolPin;
        const library::GenCompSignal* mGenCompSignal;
        GenCompSignalInstance* mGenCompSignalInstance;
        SchematicNetPoint* mRegisteredSchematicNetPoint;
        library::SymbolPinGraphicsItem* mRegisteredPinGraphicsItem;
};

} // namespace project

#endif // SYMBOLPININSTANCE_H

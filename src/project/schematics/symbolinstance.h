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
#include "../../common/units/all_length_units.h"
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QGraphicsItem;
class SchematicLayer;

namespace project {
class Schematic;
class GenCompInstance;
class SymbolPinInstance;
}

namespace library {
class Symbol;
class GenCompSymbVarItem;
class SymbolGraphicsItem;
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
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getAngle() const noexcept {return mAngle;}
        QString getName() const noexcept;
        SymbolPinInstance* getPinInstance(const QUuid& pinUuid) const noexcept {return mPinInstances.value(pinUuid);}
        const QHash<QUuid, SymbolPinInstance*>& getPinInstances() const noexcept {return mPinInstances;}
        GenCompInstance& getGenCompInstance() const noexcept {return *mGenCompInstance;}
        const library::Symbol& getSymbol() const noexcept {return *mSymbol;}
        const library::GenCompSymbVarItem& getGenCompSymbVarItem() const noexcept {return *mSymbVarItem;}

        // Setters
        void setPosition(const Point& newPos) throw (Exception);
        void setAngle(const Angle& newAngle) throw (Exception);

        // General Methods
        void addToSchematic(Schematic& schematic, bool addNode,
                            QDomElement& parent) throw (Exception);
        void removeFromSchematic(Schematic& schematic, bool removeNode,
                                 QDomElement& parent) throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;

        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;

        // Static Methods
        static uint extractFromGraphicsItems(const QList<QGraphicsItem*>& items,
                                             QList<SymbolInstance*>& symbols) noexcept;
        static SymbolInstance* create(Schematic& schematic, QDomDocument& doc,
                                      const QUuid& genCompInstance, const QUuid& symbolItem,
                                      const Point& position = Point(), const Angle& angle = Angle(),
                                      bool mirror = false) throw (Exception);

    private:

        // make some methods inaccessible...
        SymbolInstance();
        SymbolInstance(const SymbolInstance& other);
        SymbolInstance& operator=(const SymbolInstance& rhs);

        // General
        Schematic& mSchematic;
        QDomElement mDomElement;
        const library::GenCompSymbVarItem* mSymbVarItem;
        const library::Symbol* mSymbol;
        QHash<QUuid, SymbolPinInstance*> mPinInstances; ///< key: symbol pin UUID
        library::SymbolGraphicsItem* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        GenCompInstance* mGenCompInstance;
        Point mPosition;
        Angle mAngle;
};

} // namespace project

#endif // PROJECT_SYMBOLINSTANCE_H

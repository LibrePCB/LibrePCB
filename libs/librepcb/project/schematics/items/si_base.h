/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_SI_BASE_H
#define LIBREPCB_PROJECT_SI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsScene;

namespace project {

class Project;
class Circuit;
class Schematic;
class SGI_Base;

/*****************************************************************************************
 *  Class SI_Base
 ****************************************************************************************/

/**
 * @brief The Schematic Item Base (SI_Base) class
 */
class SI_Base : public QObject
{
        Q_OBJECT

    public:

        // Types
        enum class Type_t {
            NetPoint,   ///< librepcb#project#SI_NetPoint
            NetLine,    ///< librepcb#project#SI_NetLine
            NetLabel,   ///< librepcb#project#SI_NetLabel
            Symbol,     ///< librepcb#project#SI_Symbol
            SymbolPin,  ///< librepcb#project#SI_SymbolPin
        };

        // Constructors / Destructor
        SI_Base() = delete;
        SI_Base(const SI_Base& other) = delete;
        SI_Base(Schematic& schematic) noexcept;
        virtual ~SI_Base() noexcept;

        // Getters
        Project& getProject() const noexcept;
        Circuit& getCircuit() const noexcept;
        Schematic& getSchematic() const noexcept {return mSchematic;}
        virtual Type_t getType() const noexcept = 0;
        virtual const Point& getPosition() const noexcept = 0;
        virtual QPainterPath getGrabAreaScenePx() const noexcept = 0;
        virtual bool isAddedToSchematic() const noexcept {return mIsAddedToSchematic;}
        virtual bool isSelected() const noexcept {return mIsSelected;}

        // Setters
        virtual void setSelected(bool selected) noexcept;

        // General Methods
        virtual void addToSchematic(GraphicsScene& scene) throw (Exception) = 0;
        virtual void removeFromSchematic(GraphicsScene& scene) throw (Exception) = 0;

        // Operator Overloadings
        SI_Base& operator=(const SI_Base& rhs) = delete;


    protected:

        // General Methods
        void addToSchematic(GraphicsScene& scene, SGI_Base& item) noexcept;
        void removeFromSchematic(GraphicsScene& scene, SGI_Base& item) noexcept;


    protected:

        Schematic& mSchematic;


    private:

        // General Attributes
        bool mIsAddedToSchematic;
        bool mIsSelected;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SI_BASE_H

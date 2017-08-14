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

#ifndef LIBREPCB_HOLE_H
#define LIBREPCB_HOLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../units/all_length_units.h"
#include "../fileio/serializableobject.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Hole
 ****************************************************************************************/

/**
 * @brief The Hole class
 */
class Hole final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Hole)

    public:

        // Constructors / Destructor
        explicit Hole(const Point& position, const Length& diameter) noexcept;
        explicit Hole(const DomElement& domElement);
        ~Hole() noexcept;

        // Getters
        const Point& getPosition() const noexcept {return mPosition;}
        const Length& getDiameter() const noexcept {return mDiameter;}

        // Setters
        void setPosition(const Point& position) noexcept;
        void setDiameter(const Length& diameter) noexcept;

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const override;


    private:

        // make some methods inaccessible...
        Hole() = delete;
        Hole(const Hole& other) = delete;
        Hole& operator=(const Hole& rhs) = delete;

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Polygon Attributes
        Point mPosition;
        Length mDiameter;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_HOLE_H

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

#ifndef LIBREPCB_VERTEX_H
#define LIBREPCB_VERTEX_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobject.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Vertex
 ****************************************************************************************/

/**
 * @brief The Vertex class
 */
class Vertex final : public SerializableObject
{
    public:

        // Constructors / Destructor
        Vertex() noexcept : mPos(), mAngle() {}
        Vertex(const Vertex& other) noexcept : mPos(other.mPos), mAngle(other.mAngle) {}
        explicit Vertex(const Point& pos, const Angle& angle = Angle::deg0()) noexcept :
            mPos(pos), mAngle(angle) {}
        explicit Vertex(const SExpression& node);
        ~Vertex() noexcept {}

        // Getters
        const Point& getPos() const noexcept {return mPos;}
        const Angle& getAngle() const noexcept {return mAngle;}

        // Setters
        void setPos(const Point& pos) noexcept {mPos = pos;}
        void setAngle(const Angle& angle) noexcept {mAngle = angle;}

        // General Methods
        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Vertex& rhs) const noexcept;
        bool operator!=(const Vertex& rhs) const noexcept {return !(*this == rhs);}
        Vertex& operator=(const Vertex& rhs) noexcept;


    private: // Data
        Point mPos;
        Angle mAngle;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_VERTEX_H

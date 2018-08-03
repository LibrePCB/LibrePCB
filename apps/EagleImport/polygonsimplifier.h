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

#ifndef POLYGONSIMPLIFIER_H
#define POLYGONSIMPLIFIER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcb/common/geometry/polygon.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class PolygonSimplifier
 ****************************************************************************************/

/**
 * @brief The PolygonSimplifier class
 */
template <typename LibElemType>
class PolygonSimplifier
{
    public:

        // Constructors / Destructor
        PolygonSimplifier(LibElemType& libraryElement);
        ~PolygonSimplifier();

        // General Methods
        void convertLineRectsToPolygonRects(bool fillArea, bool isGrabArea) noexcept;


    private:

        // Private Methods
        bool findLineRectangle(QList<librepcb::Polygon*>& lines) noexcept;
        bool findHLine(const QList<Polygon*>& lines, librepcb::Point& p,
                       UnsignedLength* width, librepcb::Polygon** line) noexcept;
        bool findVLine(const QList<Polygon*>& lines, librepcb::Point& p,
                       UnsignedLength* width, librepcb::Polygon** line) noexcept;


        // Attributes
        LibElemType& mLibraryElement;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // POLYGONSIMPLIFIER_H

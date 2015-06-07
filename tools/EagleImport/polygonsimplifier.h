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

#ifndef POLYGONSIMPLIFIER_H
#define POLYGONSIMPLIFIER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Class PolygonSimplifier
 ****************************************************************************************/

/**
 * @brief The PolygonSimplifier class
 */
template <typename LibElemType, typename PolygonType, typename SegmentType>
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
        bool findLineRectangle(QList<const PolygonType*>& lines) noexcept;
        bool findHLine(const QList<const PolygonType*>& lines, Point& p, Length* width,
                       const PolygonType** line) noexcept;
        bool findVLine(const QList<const PolygonType*>& lines, Point& p, Length* width,
                       const PolygonType** line) noexcept;


        // Attributes
        LibElemType& mLibraryElement;
};

#endif // POLYGONSIMPLIFIER_H

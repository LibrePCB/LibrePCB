/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __CONVEX_HULL_H
#define __CONVEX_HULL_H


#include <vector>
class wxPoint;      // Defined in wxWidgets
class SHAPE_POLY_SET;

/**
 * Calculate the convex hull of a list of points
 * in counter-clockwise order.
 * @param aResult = a vector to store the convex polygon.
 * @param aPoly is the list of points.
 */

void BuildConvexHull( std::vector<wxPoint>& aResult, const std::vector<wxPoint>& aPoly);


/**
 * Calculate the convex hull of a SHAPE_POLY_SET
 * @param aResult = a vector to store the convex polygon.
 * @param aPolygons = the SHAPE_POLY_SET
 */
void BuildConvexHull( std::vector<wxPoint>& aResult, const SHAPE_POLY_SET& aPolygons );

/**
 * Calculate the convex hull (rotated and moved) of a SHAPE_POLY_SET
 * @param aResult = a vector to store the convex polygon.
 * @param aPolygons is the set of polygons
 * @param aPosition = the final position of the convex hull
 * @param aRotation = the rotation of the convex hull
 */
void BuildConvexHull( std::vector<wxPoint>& aResult, const SHAPE_POLY_SET& aPolygons,
                             wxPoint aPosition, double aRotation );
#endif // __CONVEX_HULL_H

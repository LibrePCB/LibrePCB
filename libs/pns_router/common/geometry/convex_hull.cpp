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


/*
 *  Implementation of Andrew's monotone chain 2D convex hull algorithm.
 *  Asymptotic complexity: O(n log n).
 *  See http://www.algorithmist.com/index.php/Monotone_Chain_Convex_Hull
 *  (Licence GNU Free Documentation License 1.2)
 *
 *  Pseudo-code:
 *
 *  Input: a list P of points in the plane.
 *
 *  Sort the points of P by x-coordinate (in case of a tie, sort by y-coordinate).
 *
 *  Initialize U and L as empty lists.
 *  The lists will hold the vertices of upper and lower hulls respectively.
 *
 *  for i = 1, 2, ..., n:
 *   while L contains at least two points and the sequence of last two points
 *           of L and the point P[i] does not make a counter-clockwise turn:
 *       remove the last point from L
 *   append P[i] to L
 *
 *  for i = n, n-1, ..., 1:
 *   while U contains at least two points and the sequence of last two points
 *           of U and the point P[i] does not make a counter-clockwise turn:
 *       remove the last point from U
 *   append P[i] to U
 *
 *  Remove the last point of each list (it's the same as the first point of the other list).
 *  Concatenate L and U to obtain the convex hull of P.
 *  Points in the result will be listed in counter-clockwise order.
 */

#include <geometry/shape_poly_set.h>
#include <geometry/convex_hull.h>

#include <algorithm>
#include <wx/wx.h>
#include <trigo.h>


typedef long long coord2_t;     // must be big enough to hold 2*max(|coordinate|)^2

// this function is used to sort points.
// Andrew's monotone chain 2D convex hull algorithm needs a sorted set of points
static bool compare_point( const wxPoint& ref, const wxPoint& p )
{
    return ref.x < p.x || (ref.x == p.x && ref.y < p.y);
}


// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
static coord2_t cross_product( const wxPoint& O, const wxPoint& A, const wxPoint& B )
{
    return (coord2_t) (A.x - O.x) * (coord2_t) (B.y - O.y)
           - (coord2_t) (A.y - O.y) * (coord2_t) (B.x - O.x);
}


// Fills aResult with a list of points on the convex hull in counter-clockwise order.
void BuildConvexHull( std::vector<wxPoint>& aResult, const std::vector<wxPoint>& aPoly )
{
    std::vector<wxPoint> poly = aPoly;
    int point_count = poly.size();

    if( point_count < 2 )     // Should not happen, but who know
        return;

    // Sort points lexicographically
    // Andrew's monotone chain 2D convex hull algorithm needs that
    std::sort( poly.begin(), poly.end(), compare_point );

    int k = 0;

    // Store room (2 * n points) for result:
    // The actual convex hull use less points. the room will be adjusted later
    aResult.resize( 2 * point_count );

    // Build lower hull
    for( int ii = 0; ii < point_count; ++ii )
    {
        while( k >= 2 && cross_product( aResult[k - 2], aResult[k - 1], poly[ii] ) <= 0 )
            k--;

        aResult[k++] = poly[ii];
    }

    // Build upper hull
    for( int ii = point_count - 2, t = k + 1; ii >= 0; ii-- )
    {
        while( k >= t && cross_product( aResult[k - 2], aResult[k - 1], poly[ii] ) <= 0 )
            k--;

        aResult[k++] = poly[ii];
    }

    // The last point in the list is the same as the first one.
    // This is not needed, and sometimes create issues ( 0 length polygon segment:
    // remove it:

    if( k > 1 && aResult[0] == aResult[k - 1] )
        k -= 1;

    aResult.resize( k );
}


void BuildConvexHull( std::vector<wxPoint>& aResult,
                             const SHAPE_POLY_SET& aPolygons )
{
    BuildConvexHull( aResult, aPolygons, wxPoint( 0, 0 ), 0.0 );
}


void BuildConvexHull( std::vector<wxPoint>& aResult,
                      const SHAPE_POLY_SET& aPolygons,
                      wxPoint aPosition, double aRotation )
{
    // Build the convex hull of the SHAPE_POLY_SET
    std::vector<wxPoint> buf;

    for( int cnt = 0; cnt < aPolygons.OutlineCount(); cnt++ )
    {
        const SHAPE_LINE_CHAIN& poly = aPolygons.COutline( cnt );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
        {
            buf.push_back( wxPoint( poly.CPoint( ii ).x, poly.CPoint( ii ).y ) );
        }
    }

    BuildConvexHull(aResult, buf );

    // Move and rotate the points according to aPosition and aRotation

    for( unsigned ii = 0; ii < aResult.size(); ii++ )
    {
        RotatePoint( &aResult[ii], aRotation );
        aResult[ii] += aPosition;
    }
}

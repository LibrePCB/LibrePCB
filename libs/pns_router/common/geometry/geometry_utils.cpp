/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file geometry_utils.cpp
 * @brief a few functions useful in geometry calculations.
 */

#include <math.h>
#include <algorithm>
//#include <common.h>
#include <geometry/geometry_utils.h>

static const double correction_factor[58] =
{
        1.1547005383792515, 1.1099162641747424, 1.0823922002923940, 1.0641777724759121,
        1.0514622242382672, 1.0422171162264056, 1.0352761804100830, 1.0299278309497275,
        1.0257168632725540, 1.0223405948650293, 1.0195911582083184, 1.0173218375167883,
        1.0154266118857451, 1.0138272827109369, 1.0124651257880029, 1.0112953333155177,
        1.0102832265380361, 1.0094016211705981, 1.0086289605801528, 1.0079479708092973,
        1.0073446768656829, 1.0068076733095861, 1.0063275765801780, 1.0058966090203618,
        1.0055082795635164, 1.0051571362062028, 1.0048385723763114, 1.0045486741757732,
        1.0042840989156745, 1.0040419778191385, 1.0038198375433474, 1.0036155364690280,
        1.0034272126621453, 1.0032532411243213, 1.0030921984828256, 1.0029428336753463,
        1.0028040434931396, 1.0026748520830480, 1.0025543936921142, 1.0024418980811722,
        1.0023366781455456, 1.0022381193690537, 1.0021456708072995, 1.0020588373518127,
        1.0019771730711422, 1.0019002754608142, 1.0018277804630289, 1.0017593581404958,
        1.0016947089079804, 1.0016335602408475, 1.0015756637927993, 1.0015207928656586,
        1.0014687401828848, 1.0014193159258358, 1.0013723459979209, 1.0013276704868976,
        1.0012851422998732, 1.0012446259491854
};

static inline int KiROUND( double v )
{
    return int( v < 0 ? v - 0.5 : v + 0.5 );
}


int GetArcToSegmentCount( int aRadius, int aErrorMax, double aArcAngleDegree )
{
    // calculate the number of segments to approximate a circle by segments
    // given the max distance between the middle of a segment and the circle

    // error relative to the radius value:
    double rel_error = (double)aErrorMax / aRadius;
    // minimal arc increment in degrees:
    double step = 180 / M_PI * acos( 1.0 - rel_error ) * 2;
    // the minimal seg count for a arc
    int segCount = KiROUND( fabs( aArcAngleDegree ) / step );

    // Ensure at least one segment is used
    return std::max( segCount, 1 );
}


double GetCircletoPolyCorrectionFactor( int aSegCountforCircle )
{
    /* calculates the coeff to compensate radius reduction of circle
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / aSegCountforCircle / 2)
     * this is the distance between the center and the middle of the segment.
     * therfore, to move the  middle of the segment to the circle (distance = radius)
     * the correctionFactor is 1 /cos( PI/aSegCountforCircle  )
     */
    if( aSegCountforCircle < 6 )
        aSegCountforCircle = 6;
    if( 1 || aSegCountforCircle > 64 )
        return 1.0 / cos( M_PI / aSegCountforCircle );

    return correction_factor[ aSegCountforCircle - 6 ];
}


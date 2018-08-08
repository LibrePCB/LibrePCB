/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>
#include "pns_solid.h"

#include "geometry/shape_simple.h"
#include "pns_utils.h"

namespace PNS {

const SHAPE_LINE_CHAIN SOLID::Hull( int aClearance, int aWalkaroundThickness ) const
{
    int cl = aClearance + ( aWalkaroundThickness + 1 )/ 2;

    switch( m_shape->Type() )
    {
    case SH_RECT:
    {
        SHAPE_RECT* rect = static_cast<SHAPE_RECT*>( m_shape );
        return OctagonalHull( rect->GetPosition(), rect->GetSize(), cl + 1, 0.2 * cl );
    }

    case SH_CIRCLE:
    {
        SHAPE_CIRCLE* circle = static_cast<SHAPE_CIRCLE*>( m_shape );
        int r = circle->GetRadius();
        return OctagonalHull( circle->GetCenter() - VECTOR2I( r, r ), VECTOR2I( 2 * r, 2 * r ),
                              cl + 1, 0.52 * ( r + cl ) );
    }

    case SH_SEGMENT:
    {
        SHAPE_SEGMENT* seg = static_cast<SHAPE_SEGMENT*>( m_shape );
        return SegmentHull( *seg, aClearance, aWalkaroundThickness );
    }

    case SH_SIMPLE:
    {
        SHAPE_SIMPLE* convex = static_cast<SHAPE_SIMPLE*>( m_shape );

        return ConvexHull( *convex, cl );
    }

    default:
        break;
    }

    return SHAPE_LINE_CHAIN();
}


ITEM* SOLID::Clone() const
{
    ITEM* solid = new SOLID( *this );
    return solid;
}

}

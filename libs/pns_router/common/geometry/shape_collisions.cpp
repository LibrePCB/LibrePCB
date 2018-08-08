/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <math/vector2d.h>
#include <math.h>

#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include "geometry/shape_simple.h"

typedef VECTOR2I::extended_type ecoord;

static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    ecoord min_dist = aClearance + aA.GetRadius() + aB.GetRadius();
    ecoord min_dist_sq = min_dist * min_dist;

    const VECTOR2I delta = aB.GetCenter() - aA.GetCenter();

    ecoord dist_sq = delta.SquaredEuclideanNorm();

    if( dist_sq >= min_dist_sq )
        return false;

    if( aNeedMTV )
        aMTV = delta.Resize( min_dist - sqrt( dist_sq ) + 3 );  // fixme: apparent rounding error

    return true;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const VECTOR2I c = aB.GetCenter();
    const VECTOR2I p0 = aA.GetPosition();
    const VECTOR2I size = aA.GetSize();
    const int r = aB.GetRadius();
    const int min_dist = aClearance + r;

    const VECTOR2I vts[] =
    {
        VECTOR2I( p0.x,          p0.y ),
        VECTOR2I( p0.x,          p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y ),
        VECTOR2I( p0.x,          p0.y )
    };

    int nearest_seg_dist = INT_MAX;
    VECTOR2I nearest;

    bool inside = c.x >= p0.x && c.x <= ( p0.x + size.x )
                  && c.y >= p0.y && c.y <= ( p0.y + size.y );


    if( !aNeedMTV && inside )
        return true;

    for( int i = 0; i < 4; i++ )
    {
        const SEG seg( vts[i], vts[i + 1] );

        VECTOR2I pn = seg.NearestPoint( c );

        int d = ( pn - c ).EuclideanNorm();

        if( ( d < min_dist ) && !aNeedMTV )
            return true;

        if( d < nearest_seg_dist )
        {
            nearest = pn;
            nearest_seg_dist = d;
        }
    }

    if( nearest_seg_dist >= min_dist && !inside )
        return false;

    VECTOR2I delta = c - nearest;

    if( !aNeedMTV )
        return true;


    if( inside )
        aMTV = -delta.Resize( abs( min_dist + 1 + nearest_seg_dist ) + 1 );
    else
        aMTV = delta.Resize( abs( min_dist + 1 - nearest_seg_dist ) + 1 );


    return true;
}


static VECTOR2I pushoutForce( const SHAPE_CIRCLE& aA, const SEG& aB, int aClearance )
{
    VECTOR2I f( 0, 0 );

    const VECTOR2I c = aA.GetCenter();
    const VECTOR2I nearest = aB.NearestPoint( c );

    const int r = aA.GetRadius();

    int dist = ( nearest - c ).EuclideanNorm();
    int min_dist = aClearance + r;

    if( dist < min_dist )
    {
        for( int corr = 0; corr < 5; corr++ )
        {
            f = ( aA.GetCenter() - nearest ).Resize( min_dist - dist + corr );

            if( aB.Distance( c + f ) >= min_dist )
                break;
        }
    }

    return f;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    bool found = false;

    for( int s = 0; s < aB.SegmentCount(); s++ )
    {
        if( aA.Collide( aB.CSegment( s ), aClearance ) )
        {
            found = true;
            break;
        }
    }

    if( !aNeedMTV || !found )
        return found;

    SHAPE_CIRCLE cmoved( aA );
    VECTOR2I f_total( 0, 0 );

    for( int s = 0; s < aB.SegmentCount(); s++ )
    {
        VECTOR2I f = pushoutForce( cmoved, aB.CSegment( s ), aClearance );
        cmoved.SetCenter( cmoved.GetCenter() + f );
        f_total += f;
    }

    aMTV = f_total;
    return found;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_SIMPLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    bool found;
    const SHAPE_LINE_CHAIN& lc( aB.Vertices() );

    found = lc.Distance( aA.GetCenter() ) <= aClearance + aA.GetRadius();

    if( !aNeedMTV || !found )
        return found;

    SHAPE_CIRCLE cmoved( aA );
    VECTOR2I f_total( 0, 0 );

    for( int s = 0; s < lc.SegmentCount(); s++ )
    {
        VECTOR2I f = pushoutForce( cmoved, lc.CSegment( s ), aClearance );
        cmoved.SetCenter( cmoved.GetCenter() + f );
        f_total += f;
    }

    aMTV = f_total;
    return found;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_SEGMENT& aSeg, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    bool col = aA.Collide( aSeg.GetSeg(), aClearance + aSeg.GetWidth() / 2);

    if( col && aNeedMTV )
    {
        aMTV = -pushoutForce( aA, aSeg.GetSeg(), aClearance + aSeg.GetWidth() / 2);
    }
    return col;
}


static inline bool Collide( const SHAPE_LINE_CHAIN& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    for( int i = 0; i < aB.SegmentCount(); i++ )
        if( aA.Collide( aB.CSegment( i ), aClearance ) )
            return true;

    return false;
}


static inline bool Collide( const SHAPE_LINE_CHAIN& aA, const SHAPE_SIMPLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide( aA, aB.Vertices(), aClearance, aNeedMTV, aMTV );
}


static inline bool Collide( const SHAPE_SIMPLE& aA, const SHAPE_SIMPLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide( aA.Vertices(), aB.Vertices(), aClearance, aNeedMTV, aMTV );
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    for( int s = 0; s < aB.SegmentCount(); s++ )
    {
        SEG seg = aB.CSegment( s );

        if( aA.Collide( seg, aClearance ) )
            return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_SIMPLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide( aA, aB.Vertices(), aClearance, aNeedMTV, aMTV );
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_SEGMENT& aSeg, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return aA.Collide( aSeg.GetSeg(), aClearance + aSeg.GetWidth() / 2 );
}


static inline bool Collide( const SHAPE_SEGMENT& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2 );
}


static inline bool Collide( const SHAPE_LINE_CHAIN& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    if( aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2 ) )
        return true;

    return false;
}


static inline bool Collide( const SHAPE_SIMPLE& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide( aA.Vertices(), aB, aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_RECT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide( aA.Outline(), aB.Outline(), aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_RECT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lc = aA.ConvertToPolyline();
    return Collide( lc, aB.Outline(), aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lc = aA.ConvertToPolyline();
    bool rv = Collide( aB, lc, aClearance, aNeedMTV, aMTV );

    if( rv && aNeedMTV )
        aMTV = -aMTV;

    return rv;
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lc = aA.ConvertToPolyline();
    return Collide( lc, aB, aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lc = aA.ConvertToPolyline();
    return Collide( lc, aB, aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_SIMPLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lc = aA.ConvertToPolyline();
    return Collide( lc, aB.Vertices(), aClearance, aNeedMTV, aMTV );
}

static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_ARC& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    const auto lcA = aA.ConvertToPolyline();
    const auto lcB = aB.ConvertToPolyline();
    return Collide( lcA, lcB, aClearance, aNeedMTV, aMTV );
}

template<class ShapeAType, class ShapeBType>
inline bool CollCase( const SHAPE* aA, const SHAPE* aB, int aClearance, bool aNeedMTV, VECTOR2I& aMTV )
{
    return Collide (*static_cast<const ShapeAType*>( aA ),
                    *static_cast<const ShapeBType*>( aB ),
                    aClearance, aNeedMTV, aMTV);
}

template<class ShapeAType, class ShapeBType>
inline bool CollCaseReversed ( const SHAPE* aA, const SHAPE* aB, int aClearance, bool aNeedMTV, VECTOR2I& aMTV )
{
    bool rv = Collide (*static_cast<const ShapeBType*>( aB ),
                       *static_cast<const ShapeAType*>( aA ),
                        aClearance, aNeedMTV, aMTV);
    if(rv && aNeedMTV)
        aMTV = -aMTV;
    return rv;
}


bool CollideShapes( const SHAPE* aA, const SHAPE* aB, int aClearance, bool aNeedMTV, VECTOR2I& aMTV )
{
    switch( aA->Type() )
    {
        case SH_RECT:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCase<SHAPE_RECT, SHAPE_RECT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCase<SHAPE_RECT, SHAPE_CIRCLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_RECT, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_RECT, SHAPE_SIMPLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCaseReversed<SHAPE_RECT, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        case SH_CIRCLE:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCaseReversed<SHAPE_CIRCLE, SHAPE_RECT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCase<SHAPE_CIRCLE, SHAPE_CIRCLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_CIRCLE, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_CIRCLE, SHAPE_SIMPLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCaseReversed<SHAPE_CIRCLE, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        case SH_LINE_CHAIN:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_LINE_CHAIN, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_LINE_CHAIN, SHAPE_SIMPLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCaseReversed<SHAPE_LINE_CHAIN, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        case SH_SEGMENT:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCase<SHAPE_RECT, SHAPE_SEGMENT>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCaseReversed<SHAPE_SEGMENT, SHAPE_CIRCLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_LINE_CHAIN, SHAPE_SEGMENT>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_SEGMENT, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_SIMPLE, SHAPE_SEGMENT>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCaseReversed<SHAPE_SEGMENT, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        case SH_SIMPLE:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCase<SHAPE_RECT, SHAPE_SIMPLE>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCase<SHAPE_CIRCLE, SHAPE_SIMPLE>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_LINE_CHAIN, SHAPE_SIMPLE>( aB, aA, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_SIMPLE, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_SIMPLE, SHAPE_SIMPLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCaseReversed<SHAPE_SIMPLE, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        case SH_ARC:
            switch( aB->Type() )
            {
                case SH_RECT:
                    return CollCase<SHAPE_ARC, SHAPE_RECT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_CIRCLE:
                    return CollCase<SHAPE_ARC, SHAPE_CIRCLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_LINE_CHAIN:
                    return CollCase<SHAPE_ARC, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SEGMENT:
                    return CollCase<SHAPE_ARC, SHAPE_SEGMENT>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_SIMPLE:
                    return CollCase<SHAPE_ARC, SHAPE_SIMPLE>( aA, aB, aClearance, aNeedMTV, aMTV );

                case SH_ARC:
                    return CollCase<SHAPE_ARC, SHAPE_ARC>( aA, aB, aClearance, aNeedMTV, aMTV );

                default:
                    break;
            }
            break;

        default:
            break;
    }

    bool unsupported_collision = true;
    (void) unsupported_collision;   // make gcc quiet

    assert( unsupported_collision == false );

    return false;
}


bool SHAPE::Collide( const SHAPE* aShape, int aClearance, VECTOR2I& aMTV ) const
{
    return CollideShapes( this, aShape, aClearance, true, aMTV );
}


bool SHAPE::Collide( const SHAPE* aShape, int aClearance ) const
{
    VECTOR2I dummy;

    return CollideShapes( this, aShape, aClearance, false, dummy );
}


bool SHAPE_RECT::Collide( const SEG& aSeg, int aClearance ) const
{
    //VECTOR2I pmin = VECTOR2I( std::min( aSeg.a.x, aSeg.b.x ), std::min( aSeg.a.y, aSeg.b.y ) );
    //VECTOR2I pmax = VECTOR2I( std::max( aSeg.a.x, aSeg.b.x ), std::max( aSeg.a.y, aSeg.b.y ));
    //BOX2I r( pmin, VECTOR2I( pmax.x - pmin.x, pmax.y - pmin.y ) );

    //if( BBox( 0 ).SquaredDistance( r ) > aClearance * aClearance )
    //    return false;

    if( BBox( 0 ).Contains( aSeg.A ) || BBox( 0 ).Contains( aSeg.B ) )
        return true;

    VECTOR2I vts[] = { VECTOR2I( m_p0.x, m_p0.y ),
                        VECTOR2I( m_p0.x, m_p0.y + m_h ),
                        VECTOR2I( m_p0.x + m_w, m_p0.y + m_h ),
                        VECTOR2I( m_p0.x + m_w, m_p0.y ),
                        VECTOR2I( m_p0.x, m_p0.y ) };

    for( int i = 0; i < 4; i++ )
    {
        SEG s( vts[i], vts[i + 1], i );

        if( s.Distance( aSeg ) < aClearance )
            return true;
    }

    return false;
}

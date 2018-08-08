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

#include <geometry/seg.h>

template <typename T>
int sgn( T aVal )
{
    return ( T( 0 ) < aVal ) - ( aVal < T( 0 ) );
}


bool SEG::PointCloserThan( const VECTOR2I& aP, int aDist ) const
{
    VECTOR2I d = B - A;
    ecoord dist_sq = (ecoord) aDist * aDist;

    SEG::ecoord l_squared = d.Dot( d );
    SEG::ecoord t = d.Dot( aP - A );

    if( t <= 0 || !l_squared )
        return ( aP - A ).SquaredEuclideanNorm() < dist_sq;
    else if( t >= l_squared )
        return ( aP - B ).SquaredEuclideanNorm() < dist_sq;

    int dxdy = abs( d.x ) - abs( d.y );

    if( ( dxdy >= -1 && dxdy <= 1 ) || abs( d.x ) <= 1 || abs( d.y ) <= 1 )
    {
        int ca = -sgn( d.y );
        int cb = sgn( d.x );
        int cc = -ca * A.x - cb * A.y;

        ecoord num = (ecoord) ca * aP.x + (ecoord) cb * aP.y + cc;
        num *= num;

        if( ca && cb )
            num >>= 1;

        if( num > ( dist_sq + 100 ) )
            return false;

        else if( num < ( dist_sq - 100 ) )
            return true;
    }

    VECTOR2I nearest;
    nearest.x = A.x + rescale( t, (ecoord) d.x, l_squared );
    nearest.y = A.y + rescale( t, (ecoord) d.y, l_squared );

    return ( nearest - aP ).SquaredEuclideanNorm() <= dist_sq;
}


SEG::ecoord SEG::SquaredDistance( const SEG& aSeg ) const
{
    // fixme: rather inefficient....
    if( Intersect( aSeg ) )
        return 0;

    const VECTOR2I pts[4] =
    {
        aSeg.NearestPoint( A ) - A,
        aSeg.NearestPoint( B ) - B,
        NearestPoint( aSeg.A ) - aSeg.A,
        NearestPoint( aSeg.B ) - aSeg.B
    };

    ecoord m = VECTOR2I::ECOORD_MAX;

    for( int i = 0; i < 4; i++ )
        m = std::min( m, pts[i].SquaredEuclideanNorm() );

    return m;
}


OPT_VECTOR2I SEG::Intersect( const SEG& aSeg, bool aIgnoreEndpoints, bool aLines ) const
{
    const VECTOR2I  e( B - A );
    const VECTOR2I  f( aSeg.B - aSeg.A );
    const VECTOR2I  ac( aSeg.A - A );

    ecoord d = f.Cross( e );
    ecoord p = f.Cross( ac );
    ecoord q = e.Cross( ac );

    if( d == 0 )
        return OPT_VECTOR2I();

    if( !aLines && d > 0 && ( q < 0 || q > d || p < 0 || p > d ) )
        return OPT_VECTOR2I();

    if( !aLines && d < 0 && ( q < d || p < d || p > 0 || q > 0 ) )
        return OPT_VECTOR2I();

    if( !aLines && aIgnoreEndpoints && ( q == 0 || q == d ) && ( p == 0 || p == d ) )
        return OPT_VECTOR2I();

    VECTOR2I ip( aSeg.A.x + rescale( q, (ecoord) f.x, d ),
                 aSeg.A.y + rescale( q, (ecoord) f.y, d ) );

     return ip;
}


bool SEG::ccw( const VECTOR2I& aA, const VECTOR2I& aB, const VECTOR2I& aC ) const
{
    return (ecoord) ( aC.y - aA.y ) * ( aB.x - aA.x ) > (ecoord) ( aB.y - aA.y ) * ( aC.x - aA.x );
}


bool SEG::Collide( const SEG& aSeg, int aClearance ) const
{
    // check for intersection
    // fixme: move to a method
    if( ccw( A, aSeg.A, aSeg.B ) != ccw( B, aSeg.A, aSeg.B ) &&
            ccw( A, B, aSeg.A ) != ccw( A, B, aSeg.B ) )
        return true;

#define CHK( _seg, _pt ) \
    if( (_seg).PointCloserThan( _pt, aClearance ) ) return true;

    CHK( *this, aSeg.A );
    CHK( *this, aSeg.B );
    CHK( aSeg, A );
    CHK( aSeg, B );
#undef CHK

    return false;
}


bool SEG::Contains( const VECTOR2I& aP ) const
{
    return PointCloserThan( aP, 1 );
}

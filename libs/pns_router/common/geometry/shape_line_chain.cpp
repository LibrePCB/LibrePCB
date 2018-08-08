/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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

#include <algorithm>

//#include <common.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>

bool SHAPE_LINE_CHAIN::Collide( const VECTOR2I& aP, int aClearance ) const
{
    // fixme: ugly!
    SEG s( aP, aP );
    return this->Collide( s, aClearance );
}


void SHAPE_LINE_CHAIN::Rotate( double aAngle, const VECTOR2I& aCenter )
{
    for( std::vector<VECTOR2I>::iterator i = m_points.begin(); i != m_points.end(); ++i )
    {
        (*i) -= aCenter;
        (*i) = (*i).Rotate( aAngle );
        (*i) += aCenter;
    }
}


bool SHAPE_LINE_CHAIN::Collide( const SEG& aSeg, int aClearance ) const
{
    BOX2I box_a( aSeg.A, aSeg.B - aSeg.A );
    BOX2I::ecoord_type dist_sq = (BOX2I::ecoord_type) aClearance * aClearance;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG& s = CSegment( i );
        BOX2I box_b( s.A, s.B - s.A );

        BOX2I::ecoord_type d = box_a.SquaredDistance( box_b );

        if( d < dist_sq )
        {
            if( s.Collide( aSeg, aClearance ) )
                return true;
        }
    }

    return false;
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Reverse() const
{
    SHAPE_LINE_CHAIN a( *this );

    reverse( a.m_points.begin(), a.m_points.end() );
    a.m_closed = m_closed;

    return a;
}


int SHAPE_LINE_CHAIN::Length() const
{
    int l = 0;

    for( int i = 0; i < SegmentCount(); i++ )
        l += CSegment( i ).Length();

    return l;
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP )
{
    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    if( aStartIndex == aEndIndex )
        m_points[aStartIndex] = aP;
    else
    {
        m_points.erase( m_points.begin() + aStartIndex + 1, m_points.begin() + aEndIndex + 1 );
        m_points[aStartIndex] = aP;
    }
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const SHAPE_LINE_CHAIN& aLine )
{
    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    m_points.erase( m_points.begin() + aStartIndex, m_points.begin() + aEndIndex + 1 );
    m_points.insert( m_points.begin() + aStartIndex, aLine.m_points.begin(), aLine.m_points.end() );
}


void SHAPE_LINE_CHAIN::Remove( int aStartIndex, int aEndIndex )
{
    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    m_points.erase( m_points.begin() + aStartIndex, m_points.begin() + aEndIndex + 1 );
}


int SHAPE_LINE_CHAIN::Distance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    int d = INT_MAX;

    if( IsClosed() && PointInside( aP ) && !aOutlineOnly )
        return 0;

    for( int s = 0; s < SegmentCount(); s++ )
        d = std::min( d, CSegment( s ).Distance( aP ) );

    return d;
}


int SHAPE_LINE_CHAIN::Split( const VECTOR2I& aP )
{
    int ii = -1;
    int min_dist = 2;

    int found_index = Find( aP );

    for( int s = 0; s < SegmentCount(); s++ )
    {
        const SEG seg = CSegment( s );
        int dist = seg.Distance( aP );

        // make sure we are not producing a 'slightly concave' primitive. This might happen
        // if aP lies very close to one of already existing points.
        if( dist < min_dist && seg.A != aP && seg.B != aP )
        {
            min_dist = dist;
            if( found_index < 0 )
                ii = s;
            else if( s < found_index )
                ii = s;
        }
    }

    if( ii < 0 )
        ii = found_index;

    if( ii >= 0 )
    {
        m_points.insert( m_points.begin() + ii + 1, aP );

        return ii + 1;
    }

    return -1;
}


int SHAPE_LINE_CHAIN::Find( const VECTOR2I& aP ) const
{
    for( int s = 0; s < PointCount(); s++ )
        if( CPoint( s ) == aP )
            return s;

    return -1;
}


int SHAPE_LINE_CHAIN::FindSegment( const VECTOR2I& aP ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
        if( CSegment( s ).Distance( aP ) <= 1 )
            return s;

    return -1;
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Slice( int aStartIndex, int aEndIndex ) const
{
    SHAPE_LINE_CHAIN rv;

    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    for( int i = aStartIndex; i <= aEndIndex; i++ )
        rv.Append( m_points[i] );

    return rv;
}


struct compareOriginDistance
{
    compareOriginDistance( VECTOR2I& aOrigin ) :
        m_origin( aOrigin ) {};

    bool operator()( const SHAPE_LINE_CHAIN::INTERSECTION& aA,
                     const SHAPE_LINE_CHAIN::INTERSECTION& aB )
    {
        return ( m_origin - aA.p ).EuclideanNorm() < ( m_origin - aB.p ).EuclideanNorm();
    }

    VECTOR2I m_origin;
};


int SHAPE_LINE_CHAIN::Intersect( const SEG& aSeg, INTERSECTIONS& aIp ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        OPT_VECTOR2I p = CSegment( s ).Intersect( aSeg );

        if( p )
        {
            INTERSECTION is;
            is.our = CSegment( s );
            is.their = aSeg;
            is.p = *p;
            aIp.push_back( is );
        }
    }

    compareOriginDistance comp( aSeg.A );
    sort( aIp.begin(), aIp.end(), comp );

    return aIp.size();
}


int SHAPE_LINE_CHAIN::Intersect( const SHAPE_LINE_CHAIN& aChain, INTERSECTIONS& aIp ) const
{
    BOX2I bb_other = aChain.BBox();

    for( int s1 = 0; s1 < SegmentCount(); s1++ )
    {
        const SEG& a = CSegment( s1 );
        const BOX2I bb_cur( a.A, a.B - a.A );

        if( !bb_other.Intersects( bb_cur ) )
            continue;

        for( int s2 = 0; s2 < aChain.SegmentCount(); s2++ )
        {
            const SEG& b = aChain.CSegment( s2 );
            INTERSECTION is;

            if( a.Collinear( b ) )
            {
                is.our = a;
                is.their = b;

                if( a.Contains( b.A ) ) { is.p = b.A; aIp.push_back( is ); }
                if( a.Contains( b.B ) ) { is.p = b.B; aIp.push_back( is ); }
                if( b.Contains( a.A ) ) { is.p = a.A; aIp.push_back( is ); }
                if( b.Contains( a.B ) ) { is.p = a.B; aIp.push_back( is ); }
            }
            else
            {
                OPT_VECTOR2I p = a.Intersect( b );

                if( p )
                {
                    is.p = *p;
                    is.our = a;
                    is.their = b;
                    aIp.push_back( is );
                }
            }
        }
    }

    return aIp.size();
}


int SHAPE_LINE_CHAIN::PathLength( const VECTOR2I& aP ) const
{
    int sum = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG seg = CSegment( i );
        int d = seg.Distance( aP );

        if( d <= 1 )
        {
            sum += ( aP - seg.A ).EuclideanNorm();
            return sum;
        }
        else
            sum += seg.Length();
    }

    return -1;
}


bool SHAPE_LINE_CHAIN::PointInside( const VECTOR2I& aP ) const
{
    if( !m_closed || PointCount() < 3 || !BBox().Contains( aP ) )
        return false;

    bool inside = false;

    /**
     * To check for interior points, we draw a line in the positive x direction from
     * the point.  If it intersects an even number of segments, the point is outside the
     * line chain (it had to first enter and then exit).  Otherwise, it is inside the chain.
     *
     * Note: slope might be denormal here in the case of a horizontal line but we require our
     * y to move from above to below the point (or vice versa)
     */
    for( int i = 0; i < PointCount(); i++ )
    {
        const VECTOR2D p1 = CPoint( i );
        const VECTOR2D p2 = CPoint( i + 1 ); // CPoint wraps, so ignore counts
        const VECTOR2D diff = p2 - p1;

        if( ( ( p1.y > aP.y ) != ( p2.y > aP.y ) ) &&
                ( aP.x - p1.x < ( diff.x / diff.y ) * ( aP.y - p1.y ) ) )
            inside = !inside;
    }

    return inside;
}

static inline int KiROUND( double v )
{
    return int( v < 0 ? v - 0.5 : v + 0.5 );
}

bool SHAPE_LINE_CHAIN::PointOnEdge( const VECTOR2I& aP ) const
{
    if( !PointCount() )
        return false;
    else if( PointCount() == 1 )
        return m_points[0] == aP;

    for( int i = 0; i < PointCount(); i++ )
    {
        const VECTOR2I& p1 = CPoint( i );
        const VECTOR2I& p2 = CPoint( i + 1 );

        if( aP == p1 )
            return true;

        if( p1.x == p2.x && p1.x == aP.x && ( p1.y > aP.y ) != ( p2.y > aP.y ) )
            return true;

        const VECTOR2D diff = p2 - p1;
        if( aP.x >= p1.x && aP.x <= p2.x )
        {
            if( KiROUND( p1.y + ( diff.y / diff.x ) * ( aP.x - p1.x ) ) == aP.y )
                return true;
        }
    }

    return false;
}


bool SHAPE_LINE_CHAIN::CheckClearance( const VECTOR2I& aP, const int aDist) const
{
    if( !PointCount() )
        return false;

    else if( PointCount() == 1 )
        return m_points[0] == aP;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG s = CSegment( i );

        if( s.A == aP || s.B == aP )
            return true;

        if( s.Distance( aP ) <= aDist )
            return true;
    }

    return false;
}


const OPT<SHAPE_LINE_CHAIN::INTERSECTION> SHAPE_LINE_CHAIN::SelfIntersecting() const
{
    for( int s1 = 0; s1 < SegmentCount(); s1++ )
    {
        for( int s2 = s1 + 1; s2 < SegmentCount(); s2++ )
        {
            const VECTOR2I s2a = CSegment( s2 ).A, s2b = CSegment( s2 ).B;

            if( s1 + 1 != s2 && CSegment( s1 ).Contains( s2a ) )
            {
                INTERSECTION is;
                is.our = CSegment( s1 );
                is.their = CSegment( s2 );
                is.p = s2a;
                return is;
            }
            else if( CSegment( s1 ).Contains( s2b ) &&
                     // for closed polylines, the ending point of the
                     // last segment == starting point of the first segment
                     // this is a normal case, not self intersecting case
                     !( IsClosed() && s1 == 0 && s2 == SegmentCount()-1 ) )
            {
                INTERSECTION is;
                is.our = CSegment( s1 );
                is.their = CSegment( s2 );
                is.p = s2b;
                return is;
            }
            else
            {
                OPT_VECTOR2I p = CSegment( s1 ).Intersect( CSegment( s2 ), true );

                if( p )
                {
                    INTERSECTION is;
                    is.our = CSegment( s1 );
                    is.their = CSegment( s2 );
                    is.p = *p;
                    return is;
                }
            }
        }
    }

    return OPT<SHAPE_LINE_CHAIN::INTERSECTION>();
}


SHAPE_LINE_CHAIN& SHAPE_LINE_CHAIN::Simplify()
{
    std::vector<VECTOR2I> pts_unique;

    if( PointCount() < 2 )
    {
        return *this;
    }
    else if( PointCount() == 2 )
    {
        if( m_points[0] == m_points[1] )
            m_points.pop_back();

        return *this;
    }

    int i = 0;
    int np = PointCount();

    // stage 1: eliminate duplicate vertices
    while( i < np )
    {
        int j = i + 1;

        while( j < np && CPoint( i ) == CPoint( j ) )
            j++;

        pts_unique.push_back( CPoint( i ) );
        i = j;
    }

    m_points.clear();
    np = pts_unique.size();

    i = 0;

    // stage 1: eliminate collinear segments
    while( i < np - 2 )
    {
        const VECTOR2I p0 = pts_unique[i];
        const VECTOR2I p1 = pts_unique[i + 1];
        int n = i;

        while( n < np - 2 && SEG( p0, p1 ).LineDistance( pts_unique[n + 2] ) <= 1 )
            n++;

        m_points.push_back( p0 );

        if( n > i )
            i = n;

        if( n == np )
        {
            m_points.push_back( pts_unique[n - 1] );
            return *this;
        }

        i++;
    }

    if( np > 1 )
        m_points.push_back( pts_unique[np - 2] );

    m_points.push_back( pts_unique[np - 1] );

    return *this;
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const VECTOR2I& aP ) const
{
    int min_d = INT_MAX;
    int nearest = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        int d = CSegment( i ).Distance( aP );

        if( d < min_d )
        {
            min_d = d;
            nearest = i;
        }
    }

    return CSegment( nearest ).NearestPoint( aP );
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const SEG& aSeg, int& dist ) const
{
    int nearest = 0;

    dist = INT_MAX;
    for( int i = 0; i < PointCount(); i++ )
    {
        int d = aSeg.LineDistance( CPoint( i ) );

        if( d < dist )
        {
            dist = d;
            nearest = i;
        }
    }

    return CPoint( nearest );
}


const std::string SHAPE_LINE_CHAIN::Format() const
{
    std::stringstream ss;

    ss << m_points.size() << " " << ( m_closed ? 1 : 0 ) << " ";

    for( int i = 0; i < PointCount(); i++ )
        ss << m_points[i].x << " " << m_points[i].y << " "; // Format() << " ";

    return ss.str();
}


bool SHAPE_LINE_CHAIN::CompareGeometry ( const SHAPE_LINE_CHAIN & aOther ) const
{
    SHAPE_LINE_CHAIN a(*this), b( aOther );
    a.Simplify();
    b.Simplify();

    if( a.m_points.size() != b.m_points.size() )
        return false;

    for( int i = 0; i < a.PointCount(); i++)
        if( a.CPoint( i ) != b.CPoint( i ) )
            return false;
    return true;
}


bool SHAPE_LINE_CHAIN::Intersects( const SHAPE_LINE_CHAIN& aChain ) const
{
    INTERSECTIONS dummy;
    return Intersect( aChain, dummy ) != 0;
}


SHAPE* SHAPE_LINE_CHAIN::Clone() const
{
    return new SHAPE_LINE_CHAIN( *this );
}

bool SHAPE_LINE_CHAIN::Parse( std::stringstream& aStream )
{
    int n_pts;

    m_points.clear();
    aStream >> n_pts;

    // Rough sanity check, just make sure the loop bounds aren't absolutely outlandish
    if( n_pts < 0 || n_pts > int( aStream.str().size() ) )
        return false;

    aStream >> m_closed;

    for( int i = 0; i < n_pts; i++ )
    {
        int x, y;
        aStream >> x;
        aStream >> y;
        m_points.push_back( VECTOR2I( x, y ) );
    }

    return true;
}


const VECTOR2I SHAPE_LINE_CHAIN::PointAlong( int aPathLength ) const
{
    int total = 0;

    if( aPathLength == 0 )
        return CPoint( 0 );

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG& s = CSegment( i );
        int l = s.Length();

        if( total + l >= aPathLength )
        {
            VECTOR2I d( s.B - s.A );
            return s.A + d.Resize( aPathLength - total );
        }

        total += l;
    }

    return CPoint( -1 );
}

double SHAPE_LINE_CHAIN::Area() const
{
    // see https://www.mathopenref.com/coordpolygonarea2.html

    if( !m_closed )
        return 0.0;

    double area = 0.0;
    int size = m_points.size();

    for( int i = 0, j = size - 1; i < size; ++i )
    {
        area += ( (double) m_points[j].x + m_points[i].x ) * ( (double) m_points[j].y - m_points[i].y );
        j = i;
    }

    return -area * 0.5;
}

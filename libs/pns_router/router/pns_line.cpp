/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
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

#include <core/optional.h>

#include <math/vector2d.h>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"

#include <geometry/shape_rect.h>

namespace PNS {

LINE::LINE( const LINE& aOther ) :
        ITEM( aOther ),
        m_line( aOther.m_line ),
        m_width( aOther.m_width )
{
    m_net = aOther.m_net;
    m_movable = aOther.m_movable;
    m_layers = aOther.m_layers;
    m_via = aOther.m_via;
    m_hasVia = aOther.m_hasVia;
    m_marker = aOther.m_marker;
    m_rank = aOther.m_rank;

    copyLinks( &aOther );
}


LINE::~LINE()
{
}


const LINE& LINE::operator=( const LINE& aOther )
{
    m_line = aOther.m_line;
    m_width = aOther.m_width;
    m_net = aOther.m_net;
    m_movable = aOther.m_movable;
    m_layers = aOther.m_layers;
    m_via = aOther.m_via;
    m_hasVia = aOther.m_hasVia;
    m_marker = aOther.m_marker;
    m_rank = aOther.m_rank;

    copyLinks( &aOther );

    return *this;
}


LINE* LINE::Clone() const
{
    LINE* l = new LINE( *this );

    return l;
}


void LINE::Mark( int aMarker )
{
    m_marker = aMarker;

    for( SEGMENT* s : m_segmentRefs )
        s->Mark( aMarker );

}


void LINE::Unmark( int aMarker )
{
    for( SEGMENT* s : m_segmentRefs )
        s->Unmark( aMarker );

    m_marker = 0;
}


int LINE::Marker() const
{
    int marker = m_marker;

    for( SEGMENT* s : m_segmentRefs )
    {
        marker |= s->Marker();
    }

    return marker;
}


void LINE::copyLinks( const LINE* aParent )
{
    m_segmentRefs = aParent->m_segmentRefs;
}


SEGMENT* SEGMENT::Clone() const
{
    SEGMENT* s = new SEGMENT;

    s->m_seg = m_seg;
    s->m_net = m_net;
    s->m_layers = m_layers;
    s->m_marker = m_marker;
    s->m_rank = m_rank;

    return s;
}


int LINE::CountCorners( int aAngles ) const
{
    int count = 0;

    for( int i = 0; i < m_line.SegmentCount() - 1; i++ )
    {
        const SEG seg1 = m_line.CSegment( i );
        const SEG seg2 = m_line.CSegment( i + 1 );

        const DIRECTION_45 dir1( seg1 );
        const DIRECTION_45 dir2( seg2 );

        DIRECTION_45::AngleType a = dir1.Angle( dir2 );

        if( a & aAngles )
            count++;
    }

    return count;
}


bool LINE::Walkaround( SHAPE_LINE_CHAIN aObstacle, SHAPE_LINE_CHAIN& aPre,
                           SHAPE_LINE_CHAIN& aWalk, SHAPE_LINE_CHAIN& aPost, bool aCw ) const
{
    const SHAPE_LINE_CHAIN& line( CLine() );

    if( line.SegmentCount() < 1 )
        return false;

    if( aObstacle.PointInside( line.CPoint( 0 ) ) || aObstacle.PointInside( line.CPoint( -1 ) ) )
        return false;

    SHAPE_LINE_CHAIN::INTERSECTIONS ips;

    line.Intersect( aObstacle, ips );

    aWalk.Clear();
    aPost.Clear();

    int nearest_dist = INT_MAX;
    int farthest_dist = 0;

    SHAPE_LINE_CHAIN::INTERSECTION nearest, farthest;

    for( int i = 0; i < (int) ips.size(); i++ )
    {
        const VECTOR2I p = ips[i].p;
        int dist = line.PathLength( p );

        if( dist < 0 )
            return false;

        if( dist <= nearest_dist )
        {
            nearest_dist = dist;
            nearest = ips[i];
        }

        if( dist >= farthest_dist )
        {
            farthest_dist = dist;
            farthest = ips[i];
        }
    }

    if( ips.size() <= 1 || nearest.p == farthest.p )
    {
        aPre = line;
        return true;
    }

    aPre = line.Slice( 0, nearest.our.Index() );
    aPre.Append( nearest.p );
    aPre.Simplify();

    aWalk.Clear();
    aWalk.SetClosed( false );
    aWalk.Append( nearest.p );

    assert( nearest.their.Index() >= 0 );
    assert( farthest.their.Index() >= 0 );

    assert( nearest_dist <= farthest_dist );

    aObstacle.Split( nearest.p );
    aObstacle.Split( farthest.p );

    int i_first = aObstacle.Find( nearest.p );
    int i_last = aObstacle.Find( farthest.p );

    int i = i_first;

    if( i_first < 0 || i_last < 0 )
        return false;

    while( i != i_last )
    {
        aWalk.Append( aObstacle.CPoint( i ) );
        i += ( aCw ? 1 : -1 );

        if( i < 0 )
            i = aObstacle.PointCount() - 1;
        else if( i == aObstacle.PointCount() )
            i = 0;
    }

    aWalk.Append( farthest.p );
    aWalk.Simplify();

    aPost.Clear();
    aPost.Append( farthest.p );
    aPost.Append( line.Slice( farthest.our.Index() + 1, -1 ) );
    aPost.Simplify();

    return true;
}


bool LINE::Walkaround( const SHAPE_LINE_CHAIN& aObstacle, SHAPE_LINE_CHAIN& aPath, bool aCw ) const
{
    SHAPE_LINE_CHAIN walk, post;

    if( ! Walkaround( aObstacle, aPath, walk, post, aCw ) )
        return false;

    aPath.Append( walk );
    aPath.Append( post );
    aPath.Simplify();

    return true;
}


const SHAPE_LINE_CHAIN SEGMENT::Hull( int aClearance, int aWalkaroundThickness ) const
{
   return SegmentHull( m_seg, aClearance, aWalkaroundThickness );
}


bool LINE::Is45Degree() const
{
    for( int i = 0; i < m_line.SegmentCount(); i++ )
    {
        const SEG& s = m_line.CSegment( i );

        if( s.Length() < 10 )
            continue;

        double angle = 180.0 / M_PI *
                       atan2( (double) s.B.y - (double) s.A.y,
                              (double) s.B.x - (double) s.A.x );

        if( angle < 0 )
            angle += 360.0;

        double angle_a = fabs( fmod( angle, 45.0 ) );

        if( angle_a > 1.0 && angle_a < 44.0 )
            return false;
    }

    return true;
}


const LINE LINE::ClipToNearestObstacle( NODE* aNode ) const
{
    const int IterationLimit = 5;
    int i;
    LINE l( *this );

    for( i = 0; i < IterationLimit; i++ )
    {
        NODE::OPT_OBSTACLE obs = aNode->NearestObstacle( &l );

        if( obs )
        {
            l.RemoveVia();
            int p = l.Line().Split( obs->m_ipFirst );
            l.Line().Remove( p + 1, -1 );
        } else
            break;
    }

    if( i == IterationLimit )
        l.Line().Clear();

    return l;
}


void LINE::ShowLinks() const
{
    if( !IsLinked() )
    {
        wxLogTrace( "PNS", "line %p: no links", this );
        return;
    }

    wxLogTrace( "PNS", "line %p: %d linked segs", this, (int) m_segmentRefs.size() );

    for( int i = 0; i < (int) m_segmentRefs.size(); i++ )
        wxLogTrace( "PNS", "seg %d: %p\n", i, m_segmentRefs[i] );
}

SHAPE_LINE_CHAIN dragCornerInternal( const SHAPE_LINE_CHAIN& aOrigin, const VECTOR2I& aP )
{
    OPT<SHAPE_LINE_CHAIN> picked;
    int i;
    int d = 2;

    if( aOrigin.SegmentCount() == 1)
    {
        DIRECTION_45 dir( aOrigin.CPoint( 0 ) - aOrigin.CPoint( 1 ) );

        return DIRECTION_45().BuildInitialTrace( aOrigin.CPoint( 0 ), aP, dir.IsDiagonal() );
    }

    if( aOrigin.CSegment( -1 ).Length() > 100000 * 30 ) // fixme: constant/parameter?
        d = 1;

    for( i = aOrigin.SegmentCount() - d; i >= 0; i-- )
    {
        DIRECTION_45 d_start( aOrigin.CSegment( i ) );
        VECTOR2I p_start = aOrigin.CPoint( i );
        SHAPE_LINE_CHAIN paths[2];
        DIRECTION_45 dirs[2];
        DIRECTION_45 d_prev = ( i > 0 ? DIRECTION_45( aOrigin.CSegment( i-1 ) ) : DIRECTION_45() );

        for( int j = 0; j < 2; j++ )
        {
            paths[j] = d_start.BuildInitialTrace( p_start, aP, j );
            dirs[j] = DIRECTION_45( paths[j].CSegment( 0 ) );
        }

        for( int j = 0; j < 2; j++ )
        {
            if( dirs[j] == d_start )
            {
                picked = paths[j];
                break;
            }
        }

        if( picked )
            break;

        for( int j = 0; j < 2; j++ )
        {
            if( dirs[j].IsObtuse( d_prev ) )
            {
                picked = paths[j];
                break;
            }
        }

        if( picked )
            break;
    }

    if( picked )
    {
        SHAPE_LINE_CHAIN path = aOrigin.Slice( 0, i );
        path.Append( *picked );

        return path;
    }

    DIRECTION_45 dir( aOrigin.CPoint( -1 ) - aOrigin.CPoint( -2 ) );

    return DIRECTION_45().BuildInitialTrace( aOrigin.CPoint( 0 ), aP, dir.IsDiagonal() );
}


void LINE::dragCorner45( const VECTOR2I& aP, int aIndex, int aSnappingThreshold )
{
    SHAPE_LINE_CHAIN path;

    VECTOR2I snapped = snapDraggedCorner( m_line, aP, aIndex, aSnappingThreshold );

    if( aIndex == 0 )
        path = dragCornerInternal( m_line.Reverse(), snapped ).Reverse();
    else if( aIndex == m_line.SegmentCount() )
        path = dragCornerInternal( m_line, snapped );
    else
    {
        // fixme: awkward behaviour for "outwards" drags
        path = dragCornerInternal( m_line.Slice( 0, aIndex ), snapped );
        SHAPE_LINE_CHAIN path_rev = dragCornerInternal( m_line.Slice( aIndex, -1 ).Reverse(),
                                                        snapped ).Reverse();
        path.Append( path_rev );
    }

    path.Simplify();
    m_line = path;
}


void LINE::dragCornerFree( const VECTOR2I& aP, int aIndex, int aSnappingThreshold )
{
    m_line.Point( aIndex ) = aP;
    m_line.Simplify();
}

void LINE::DragCorner( const VECTOR2I& aP, int aIndex, int aSnappingThreshold, bool aFreeAngle )
{
    if( aFreeAngle )
    {
        dragCornerFree ( aP, aIndex, aSnappingThreshold );
    }
    else
    {
        dragCorner45 ( aP, aIndex, aSnappingThreshold );
    }
}

void LINE::DragSegment( const VECTOR2I& aP, int aIndex, int aSnappingThreshold, bool aFreeAngle )
{
    if( aFreeAngle )
    {
        assert( false );
    }
    else
    {
        dragSegment45 ( aP, aIndex, aSnappingThreshold );
    }
}


VECTOR2I LINE::snapDraggedCorner( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I& aP,
                                      int aIndex, int aThreshold ) const
{
    int s_start = std::max( aIndex - 2, 0 );
    int s_end = std::min( aIndex + 2, aPath.SegmentCount() - 1 );

    int i, j;
    int best_dist = INT_MAX;
    VECTOR2I best_snap = aP;

    if( aThreshold <= 0 )
        return aP;

    for( i = s_start; i <= s_end; i++ )
    {
        const SEG& a = aPath.CSegment( i );

        for( j = s_start; j < i; j++ )
        {
            const SEG& b = aPath.CSegment( j );

            if( !( DIRECTION_45( a ).IsObtuse(DIRECTION_45( b ) ) ) )
                continue;

            OPT_VECTOR2I ip = a.IntersectLines(b);

            if( ip )
            {
                int dist = ( *ip - aP ).EuclideanNorm();

                if( dist < aThreshold && dist < best_dist )
                {
                    best_dist = dist;
                    best_snap = *ip;
                }
            }
        }
    }

    return best_snap;
}

VECTOR2I LINE::snapToNeighbourSegments( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I &aP,
                                            int aIndex, int aThreshold ) const
{
    VECTOR2I snap_p[2];
    DIRECTION_45 dragDir( aPath.CSegment( aIndex ) );
    int snap_d[2] = { -1, -1 };

    if( aThreshold == 0 )
        return aP;

    if( aIndex >= 2 )
    {
        SEG s = aPath.CSegment( aIndex - 2 );

        if( DIRECTION_45( s ) == dragDir )
            snap_d[0] = s.LineDistance( aP );

        snap_p[0] = s.A;
    }

    if( aIndex < aPath.SegmentCount() - 2 )
    {
        SEG s = aPath.CSegment( aIndex + 2 );

        if( DIRECTION_45( s ) == dragDir )
            snap_d[1] = s.LineDistance(aP);

        snap_p[1] = s.A;
    }

    VECTOR2I best = aP;
    int minDist = INT_MAX;

    for( int i = 0; i < 2; i++ )
    {
        if( snap_d[i] >= 0 && snap_d[i] < minDist && snap_d[i] <= aThreshold )
        {
            minDist = snap_d[i];
            best = snap_p[i];
        }
    }

    return best;
}


void LINE::dragSegment45( const VECTOR2I& aP, int aIndex, int aSnappingThreshold )
{
    SHAPE_LINE_CHAIN path( m_line );
    VECTOR2I target( aP );

    SEG guideA[2], guideB[2];
    int index = aIndex;

    target = snapToNeighbourSegments( path, aP, aIndex, aSnappingThreshold );

    if( index == 0 )
    {
        path.Insert( 0, path.CPoint( 0 ) );
        index++;
    }

    if( index == path.SegmentCount() - 1 )
    {
        path.Insert( path.PointCount() - 1, path.CPoint( -1 ) );
    }

    SEG dragged = path.CSegment( index );
    DIRECTION_45 drag_dir( dragged );

    SEG s_prev = path.CSegment( index - 1 );
    SEG s_next = path.CSegment( index + 1 );

    DIRECTION_45 dir_prev( s_prev );
    DIRECTION_45 dir_next( s_next );

    if( dir_prev == drag_dir )
    {
        dir_prev = dir_prev.Left();
        path.Insert( index, path.CPoint( index ) );
        index++;
    }

    if( dir_next == drag_dir )
    {
        dir_next = dir_next.Right();
        path.Insert( index + 1, path.CPoint( index + 1 ) );
    }

    s_prev = path.CSegment( index - 1 );
    s_next = path.CSegment( index + 1 );
    dragged = path.CSegment( index );

    const bool lockEndpointA = true;
    const bool lockEndpointB = true;

    if( aIndex == 0 )
    {
        if( !lockEndpointA )
        {
            guideA[0] = guideA[1] = SEG( dragged.A,
                                         dragged.A + drag_dir.Right().Right().ToVector() );
        }
        else
        {
            guideA[0] = SEG( dragged.A, dragged.A + drag_dir.Right().ToVector() );
            guideA[1] = SEG( dragged.A, dragged.A + drag_dir.Left().ToVector() );
        }
    }
    else
    {
        if( dir_prev.IsObtuse(drag_dir ) )
        {
            guideA[0] = SEG( s_prev.A, s_prev.A + drag_dir.Left().ToVector() );
            guideA[1] = SEG( s_prev.A, s_prev.A + drag_dir.Right().ToVector() );
        }
        else
            guideA[0] = guideA[1] = SEG( dragged.A, dragged.A + dir_prev.ToVector() );
    }

    if( aIndex == m_line.SegmentCount() - 1 )
    {
        if( !lockEndpointB )
        {
            guideB[0] = guideB[1] = SEG( dragged.B,
                                         dragged.B + drag_dir.Right().Right().ToVector() );
        }
        else
        {
            guideB[0] = SEG( dragged.B, dragged.B + drag_dir.Right().ToVector() );
            guideB[1] = SEG( dragged.B, dragged.B + drag_dir.Left().ToVector() );
        }
    }
    else
    {
        if( dir_next.IsObtuse( drag_dir ) )
        {
            guideB[0] = SEG( s_next.B, s_next.B + drag_dir.Left().ToVector() );
            guideB[1] = SEG( s_next.B, s_next.B + drag_dir.Right().ToVector() );
        }
        else
            guideB[0] = guideB[1] =    SEG( dragged.B, dragged.B + dir_next.ToVector() );
    }

    SEG s_current( target, target + drag_dir.ToVector() );

    int best_len = INT_MAX;
    SHAPE_LINE_CHAIN best;

    for( int i = 0; i < 2; i++ )
    {
        for( int j = 0; j < 2; j++ )
        {
            OPT_VECTOR2I ip1 = s_current.IntersectLines( guideA[i] );
            OPT_VECTOR2I ip2 = s_current.IntersectLines( guideB[j] );

            SHAPE_LINE_CHAIN np;

            if( !ip1 || !ip2 )
                continue;

            SEG s1( s_prev.A, *ip1 );
            SEG s2( *ip1, *ip2 );
            SEG s3( *ip2, s_next.B );

            OPT_VECTOR2I ip;

            if( (ip = s1.Intersect( s_next )) )
            {
                np.Append( s1.A );
                np.Append( *ip );
                np.Append( s_next.B );
            }
            else if( (ip = s3.Intersect( s_prev )) )
            {
                np.Append( s_prev.A );
                np.Append( *ip );
                np.Append( s3.B );
            }
            else if( (ip = s1.Intersect( s3 )) )
            {
                np.Append( s_prev.A );
                np.Append( *ip );
                np.Append( s_next.B );
            }
            else
            {
                np.Append( s_prev.A );
                np.Append( *ip1 );
                np.Append( *ip2 );
                np.Append( s_next.B );
            }

            if( np.Length() < best_len )
            {
                best_len = np.Length();
                best = np;
            }
        }
    }

    if( !lockEndpointA && aIndex == 0 )
        best.Remove( 0, 0 );
    if( !lockEndpointB && aIndex == m_line.SegmentCount() - 1 )
        best.Remove( -1, -1 );

    if( m_line.PointCount() == 1 )
        m_line = best;
    else if( aIndex == 0 )
        m_line.Replace( 0, 1, best );
    else if( aIndex == m_line.SegmentCount() - 1 )
        m_line.Replace( -2, -1, best );
    else
        m_line.Replace( aIndex, aIndex + 1, best );

    m_line.Simplify();
}


bool LINE::CompareGeometry( const LINE& aOther )
{
    return m_line.CompareGeometry( aOther.m_line );
}


void LINE::Reverse()
{
    m_line = m_line.Reverse();

    std::reverse( m_segmentRefs.begin(), m_segmentRefs.end() );
}


void LINE::AppendVia( const VIA& aVia )
{
    if( m_line.PointCount() > 1 && aVia.Pos() == m_line.CPoint( 0 ) )
    {
        Reverse();
    }

    m_hasVia = true;
    m_via = aVia;
    m_via.SetNet( m_net );
}


void LINE::SetRank( int aRank )
{
    m_rank = aRank;

    for( SEGMENT* s : m_segmentRefs )
        s->SetRank( aRank );

}


int LINE::Rank() const
{
    int min_rank = INT_MAX;

    if( IsLinked() ) {
        for( SEGMENT *s : m_segmentRefs )
        {
            min_rank = std::min( min_rank, s->Rank() );
        }
    } else {
        min_rank = m_rank;
    }

    int rank = ( min_rank == INT_MAX ) ? -1 : min_rank;

    return rank;
}


void LINE::ClipVertexRange( int aStart, int aEnd )
{
    m_line = m_line.Slice( aStart, aEnd );

    if( IsLinked() ) {
        assert( m_segmentRefs.size() < INT_MAX );
        assert( (int) m_segmentRefs.size() >= (aEnd - aStart) );

        // Note: The range includes aEnd, but we have n-1 segments.
        std::rotate(
            m_segmentRefs.begin(),
            m_segmentRefs.begin() + aStart,
            m_segmentRefs.begin() + aEnd
        );

        m_segmentRefs.resize( aEnd - aStart );
    }
}


bool LINE::HasLoops() const
{
    for( int i = 0; i < PointCount(); i++ )
    {
        for( int j = i + 2; j < PointCount(); j++ )
        {
            if( CPoint( i ) == CPoint( j ) )
                return true;
        }
    }

    return false;
}


void LINE::ClearSegmentLinks()
{
    m_segmentRefs.clear();
}


static void extendBox( BOX2I& aBox, bool& aDefined, const VECTOR2I& aP )
{
    if( aDefined )
    {
        aBox.Merge( aP );
    }
    else
    {
        aBox = BOX2I( aP, VECTOR2I( 0, 0 ) );
        aDefined = true;
    }
}


OPT_BOX2I LINE::ChangedArea( const LINE* aOther ) const
{
    BOX2I area;
    bool areaDefined = false;

    int i_start = -1;
    int i_end_self = -1, i_end_other = -1;

    SHAPE_LINE_CHAIN self( m_line );
    self.Simplify();
    SHAPE_LINE_CHAIN other( aOther->m_line );
    other.Simplify();

    int np_self = self.PointCount();
    int np_other = other.PointCount();

    int n = std::min( np_self, np_other );

    for( int i = 0; i < n; i++ )
    {
        const VECTOR2I p1 = self.CPoint( i );
        const VECTOR2I p2 = other.CPoint( i );

        if( p1 != p2 )
        {
            if( i != n - 1 )
            {
                SEG s = self.CSegment( i );

                if( !s.Contains( p2 ) )
                {
                    i_start = i;
                    break;
                }
            }
            else
            {
                i_start = i;
                break;
            }
        }
    }

    for( int i = 0; i < n; i++ )
    {
        const VECTOR2I p1 = self.CPoint( np_self - 1 - i );
        const VECTOR2I p2 = other.CPoint( np_other - 1 - i );

        if( p1 != p2 )
        {
            i_end_self = np_self - 1 - i;
            i_end_other = np_other - 1 - i;
            break;
        }
    }

    if( i_start < 0 )
        i_start = n;

    if( i_end_self < 0 )
        i_end_self = np_self - 1;

    if( i_end_other < 0 )
        i_end_other = np_other - 1;

    for( int i = i_start; i <= i_end_self; i++ )
        extendBox( area, areaDefined, self.CPoint( i ) );

    for( int i = i_start; i <= i_end_other; i++ )
        extendBox( area, areaDefined, other.CPoint( i ) );

    if( areaDefined )
    {
        area.Inflate( std::max( Width(), aOther->Width() ) );
        return area;
    }

    return OPT_BOX2I();
}


bool LINE::HasLockedSegments() const
{
    for( const SEGMENT* seg : m_segmentRefs )
    {
        if( seg->Marker() & MK_LOCKED )
            return true;
    }
    return false;
}

}

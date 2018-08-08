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

#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_topology.h"
#include "pns_debug_decorator.h"

//#include <class_board_item.h>

namespace PNS {

LINE_PLACER::LINE_PLACER( ROUTER* aRouter ) :
    PLACEMENT_ALGO( aRouter )
{
    m_initial_direction = DIRECTION_45::N;
    m_world = NULL;
    m_shove = NULL;
    m_currentNode = NULL;
    m_idle = true;

    // Init temporary variables (do not leave uninitialized members)
    m_lastNode = NULL;
    m_placingVia = false;
    m_currentNet = 0;
    m_currentLayer = 0;
    m_currentMode = RM_MarkObstacles;
    m_startItem = NULL;
    m_chainedPlacement = false;
    m_orthoMode = false;
}


LINE_PLACER::~LINE_PLACER()
{
}


void LINE_PLACER::setWorld( NODE* aWorld )
{
    m_world = aWorld;
}


const VIA LINE_PLACER::makeVia( const VECTOR2I& aP )
{
    const LAYER_RANGE layers( m_sizes.GetLayerTop(), m_sizes.GetLayerBottom() );

    return VIA( aP, layers, m_sizes.ViaDiameter(), m_sizes.ViaDrill(), -1, m_sizes.ViaType() );
}


bool LINE_PLACER::ToggleVia( bool aEnabled )
{
    m_placingVia = aEnabled;

    if( !aEnabled )
        m_head.RemoveVia();

    return true;
}


void LINE_PLACER::setInitialDirection( const DIRECTION_45& aDirection )
{
    m_initial_direction = aDirection;

    if( m_tail.SegmentCount() == 0 )
            m_direction = aDirection;
}


bool LINE_PLACER::handleSelfIntersections()
{
    SHAPE_LINE_CHAIN::INTERSECTIONS ips;
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    // if there is no tail, there is nothing to intersect with
    if( tail.PointCount() < 2 )
        return false;

    tail.Intersect( head, ips );

    // no intesection points - nothing to reduce
    if( ips.empty() )
        return false;

    int n = INT_MAX;
    VECTOR2I ipoint;

    // if there is more than one intersection, find the one that is
    // closest to the beginning of the tail.
    for( SHAPE_LINE_CHAIN::INTERSECTION i : ips )
    {
        if( i.our.Index() < n )
        {
            n = i.our.Index();
            ipoint = i.p;
        }
    }

    // ignore the point where head and tail meet
    if( ipoint == head.CPoint( 0 ) || ipoint == tail.CPoint( -1 ) )
        return false;

    // Intersection point is on the first or the second segment: just start routing
    // from the beginning
    if( n < 2 )
    {
        m_p_start = tail.Point( 0 );
        m_direction = m_initial_direction;
        tail.Clear();
        head.Clear();

        return true;
    }
    else
    {
        // Clip till the last tail segment before intersection.
        // Set the direction to the one of this segment.
        const SEG last = tail.CSegment( n - 1 );
        m_p_start = last.A;
        m_direction = DIRECTION_45( last );
        tail.Remove( n, -1 );
        return true;
    }

    return false;
}


bool LINE_PLACER::handlePullback()
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    if( head.PointCount() < 2 )
        return false;

    int n = tail.PointCount();

    if( n == 0 )
        return false;
    else if( n == 1 )
    {
        m_p_start = tail.CPoint( 0 );
        tail.Clear();
        return true;
    }

    DIRECTION_45 first_head( head.CSegment( 0 ) );
    DIRECTION_45 last_tail( tail.CSegment( -1 ) );
    DIRECTION_45::AngleType angle = first_head.Angle( last_tail );

    // case 1: we have a defined routing direction, and the currently computed
    // head goes in different one.
    bool pullback_1 = false;    // (m_direction != DIRECTION_45::UNDEFINED && m_direction != first_head);

    // case 2: regardless of the current routing direction, if the tail/head
    // extremities form an acute or right angle, reduce the tail by one segment
    // (and hope that further iterations) will result with a cleaner trace
    bool pullback_2 = ( angle == DIRECTION_45::ANG_RIGHT || angle == DIRECTION_45::ANG_ACUTE );

    if( pullback_1 || pullback_2 )
    {
        const SEG last = tail.CSegment( -1 );
        m_direction = DIRECTION_45( last );
        m_p_start = last.A;

        wxLogTrace( "PNS", "Placer: pullback triggered [%d] [%s %s]",
                n, last_tail.Format().c_str(), first_head.Format().c_str() );

        // erase the last point in the tail, hoping that the next iteration will
        // result with a head trace that starts with a segment following our
        // current direction.
        if( n < 2 )
            tail.Clear(); // don't leave a single-point tail
        else
            tail.Remove( -1, -1 );

        if( !tail.SegmentCount() )
            m_direction = m_initial_direction;

        return true;
    }

    return false;
}


bool LINE_PLACER::reduceTail( const VECTOR2I& aEnd )
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    int n = tail.SegmentCount();

    if( head.SegmentCount() < 1 )
        return false;

    // Don't attempt this for too short tails
    if( n < 2 )
        return false;

    // Start from the segment farthest from the end of the tail
    // int start_index = std::max(n - 1 - ReductionDepth, 0);

    DIRECTION_45 new_direction;
    VECTOR2I new_start;
    int reduce_index = -1;

    for( int i = tail.SegmentCount() - 1; i >= 0; i-- )
    {
        const SEG s = tail.CSegment( i );
        DIRECTION_45 dir( s );

        // calculate a replacement route and check if it matches
        // the direction of the segment to be replaced
        SHAPE_LINE_CHAIN replacement = dir.BuildInitialTrace( s.A, aEnd );

        LINE tmp( m_tail, replacement );

        if( m_currentNode->CheckColliding( &tmp, ITEM::ANY_T ) )
            break;

        if( DIRECTION_45( replacement.CSegment( 0 ) ) == dir )
        {
            new_start = s.A;
            new_direction = dir;
            reduce_index = i;
        }
    }

    if( reduce_index >= 0 )
    {
        wxLogTrace( "PNS", "Placer: reducing tail: %d", reduce_index );
        SHAPE_LINE_CHAIN reducedLine = new_direction.BuildInitialTrace( new_start, aEnd );

        m_p_start = new_start;
        m_direction = new_direction;
        tail.Remove( reduce_index + 1, -1 );
        head.Clear();
        return true;
    }

    if( !tail.SegmentCount() )
        m_direction = m_initial_direction;

    return false;
}


bool LINE_PLACER::checkObtusity( const SEG& aA, const SEG& aB ) const
{
    const DIRECTION_45 dir_a( aA );
    const DIRECTION_45 dir_b( aB );

    return dir_a.IsObtuse( dir_b ) || dir_a == dir_b;
}


bool LINE_PLACER::mergeHead()
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    const int ForbiddenAngles = DIRECTION_45::ANG_ACUTE |
                                DIRECTION_45::ANG_HALF_FULL |
                                DIRECTION_45::ANG_UNDEFINED;

    head.Simplify();
    tail.Simplify();

    int n_head  = head.SegmentCount();
    int n_tail  = tail.SegmentCount();

    if( n_head < 3 )
    {
        wxLogTrace( "PNS", "Merge failed: not enough head segs." );
        return false;
    }

    if( n_tail && head.CPoint( 0 ) != tail.CPoint( -1 ) )
    {
        wxLogTrace( "PNS", "Merge failed: head and tail discontinuous." );
        return false;
    }

    if( m_head.CountCorners( ForbiddenAngles ) != 0 )
        return false;

    DIRECTION_45 dir_tail, dir_head;

    dir_head = DIRECTION_45( head.CSegment( 0 ) );

    if( n_tail )
    {
        dir_tail = DIRECTION_45( tail.CSegment( -1 ) );

        if( dir_head.Angle( dir_tail ) & ForbiddenAngles )
            return false;
    }

    if( !n_tail )
        tail.Append( head.CSegment( 0 ).A );

    for( int i = 0; i < n_head - 2; i++ )
    {
        tail.Append( head.CSegment( i ).B );
    }

    tail.Simplify();

    SEG last = tail.CSegment( -1 );

    m_p_start = last.B;
    m_direction = DIRECTION_45( last ).Right();

    head.Remove( 0, n_head - 2 );

    wxLogTrace( "PNS", "Placer: merge %d, new direction: %s", n_head, m_direction.Format().c_str() );

    head.Simplify();
    tail.Simplify();

    return true;
}


bool LINE_PLACER::rhWalkOnly( const VECTOR2I& aP, LINE& aNewHead )
{
    LINE initTrack( m_head );
    LINE walkFull;
    int effort = 0;
    bool rv = true, viaOk;

    viaOk = buildInitialLine( aP, initTrack );

    WALKAROUND walkaround( m_currentNode, Router() );

    walkaround.SetSolidsOnly( false );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );

    WALKAROUND::WALKAROUND_STATUS wf = walkaround.Route( initTrack, walkFull, false );

    switch( Settings().OptimizerEffort() )
    {
    case OE_LOW:
        effort = 0;
        break;

    case OE_MEDIUM:
    case OE_FULL:
        effort = OPTIMIZER::MERGE_SEGMENTS;
        break;
    }

    if( Settings().SmartPads() )
        effort |= OPTIMIZER::SMART_PADS;

    if( wf == WALKAROUND::STUCK )
    {
        walkFull = walkFull.ClipToNearestObstacle( m_currentNode );
        rv = true;
    }
    else if( m_placingVia && viaOk )
    {
        walkFull.AppendVia( makeVia( walkFull.CPoint( -1 ) ) );
    }

    OPTIMIZER::Optimize( &walkFull, effort, m_currentNode );

    if( m_currentNode->CheckColliding( &walkFull ) )
    {
        aNewHead = m_head;
        return false;
    }

    m_head = walkFull;
    aNewHead = walkFull;

    return rv;
}


bool LINE_PLACER::rhMarkObstacles( const VECTOR2I& aP, LINE& aNewHead )
{
    buildInitialLine( aP, m_head );

    auto obs = m_currentNode->NearestObstacle( &m_head );

    if( obs )
    {
        int cl = m_currentNode->GetClearance( obs->m_item, &m_head );
        auto hull = obs->m_item->Hull( cl, m_head.Width() );

        auto nearest = hull.NearestPoint( aP );
        Dbg()->AddLine( hull, 2, 10000 );

        if( ( nearest - aP ).EuclideanNorm() < m_head.Width() )
        {
            buildInitialLine( nearest, m_head );
        }
    }

    aNewHead = m_head;

    return static_cast<bool>( m_currentNode->CheckColliding( &m_head ) );
}


const LINE LINE_PLACER::reduceToNearestObstacle( const LINE& aOriginalLine )
{
    const auto& l0 = aOriginalLine.CLine();

    if ( !l0.PointCount() )
        return aOriginalLine;

    int l = l0.Length();
    int step = l / 2;
    VECTOR2I target;

    LINE l_test( aOriginalLine );

    while( step > 0 )
    {
        target = l0.PointAlong( l );
        SHAPE_LINE_CHAIN l_cur( l0 );

        int index = l_cur.Split( target );

        l_test.SetShape( l_cur.Slice( 0, index ) );

        if ( m_currentNode->CheckColliding( &l_test ) )
            l -= step;
        else
            l += step;

        step /= 2;
    }

    l = l_test.CLine().Length();

    while( m_currentNode->CheckColliding( &l_test ) && l > 0 )
    {
        l--;
        target = l0.PointAlong( l );
        SHAPE_LINE_CHAIN l_cur( l0 );

        int index = l_cur.Split( target );

        l_test.SetShape( l_cur.Slice( 0, index ) );
    }

    return l_test;
}


bool LINE_PLACER::rhStopAtNearestObstacle( const VECTOR2I& aP, LINE& aNewHead )
{
    LINE l0;
    l0 = m_head;

    buildInitialLine( aP, l0 );

    LINE l_cur = reduceToNearestObstacle( l0 );

    const auto l_shape = l_cur.CLine();

    if( l_shape.SegmentCount() == 0 )
    {
        return false;
    }

    if( l_shape.SegmentCount() == 1 )
    {
        auto s = l_shape.CSegment( 0 );

        VECTOR2I dL( DIRECTION_45( s ).Left().ToVector() );
        VECTOR2I dR( DIRECTION_45( s ).Right().ToVector() );

        SEG leadL( s.B, s.B + dL );
        SEG leadR( s.B, s.B + dR );

        SEG segL( s.B, leadL.LineProject( aP ) );
        SEG segR( s.B, leadR.LineProject( aP ) );

        LINE finishL( l0, SHAPE_LINE_CHAIN( segL.A, segL.B ) );
        LINE finishR( l0, SHAPE_LINE_CHAIN( segR.A, segR.B ) );

        LINE reducedL = reduceToNearestObstacle( finishL );
        LINE reducedR = reduceToNearestObstacle( finishR );

        int lL = reducedL.CLine().Length();
        int lR = reducedR.CLine().Length();

        if( lL > lR )
            l_cur.Line().Append( reducedL.CLine() );
        else
            l_cur.Line().Append( reducedR.CLine() );

        l_cur.Line().Simplify();
    }

    m_head = l_cur;
    aNewHead = m_head;
    return true;
}


bool LINE_PLACER::rhShoveOnly( const VECTOR2I& aP, LINE& aNewHead )
{
    LINE initTrack( m_head );
    LINE walkSolids, l2;

    bool viaOk = buildInitialLine( aP, initTrack );

    m_currentNode = m_shove->CurrentNode();
    OPTIMIZER optimizer( m_currentNode );

    WALKAROUND walkaround( m_currentNode, Router() );

    walkaround.SetSolidsOnly( true );
    walkaround.SetIterationLimit( 10 );
    WALKAROUND::WALKAROUND_STATUS stat_solids = walkaround.Route( initTrack, walkSolids );

    optimizer.SetEffortLevel( OPTIMIZER::MERGE_SEGMENTS );
    optimizer.SetCollisionMask( ITEM::SOLID_T );
    optimizer.Optimize( &walkSolids );

    if( stat_solids == WALKAROUND::DONE )
        l2 = walkSolids;
    else
        l2 = initTrack.ClipToNearestObstacle( m_shove->CurrentNode() );

    LINE l( m_tail );
    l.Line().Append( l2.CLine() );
    l.Line().Simplify();

    if( l.PointCount() == 0 || l2.PointCount() == 0 )
    {
        aNewHead = m_head;
        return false;
    }

    if( m_placingVia && viaOk )
    {
        VIA v1( makeVia( l.CPoint( -1 ) ) );
        VIA v2( makeVia( l2.CPoint( -1 ) ) );

        l.AppendVia( v1 );
        l2.AppendVia( v2 );
    }

    l.Line().Simplify();

    // in certain, uncommon cases there may be loops in the head+tail, In such case, we don't shove to avoid
    // screwing up the database.
    if( l.HasLoops() )
    {
        aNewHead = m_head;
        return false;
    }

    SHOVE::SHOVE_STATUS status = m_shove->ShoveLines( l );

    m_currentNode = m_shove->CurrentNode();

    if( status == SHOVE::SH_OK  || status == SHOVE::SH_HEAD_MODIFIED )
    {
        if( status == SHOVE::SH_HEAD_MODIFIED )
        {
            l2 = m_shove->NewHead();
        }

        optimizer.SetWorld( m_currentNode );
        optimizer.SetEffortLevel( OPTIMIZER::MERGE_OBTUSE | OPTIMIZER::SMART_PADS );
        optimizer.SetCollisionMask( ITEM::ANY_T );
        optimizer.Optimize( &l2 );

        aNewHead = l2;

        return true;
    }
    else
    {
        walkaround.SetWorld( m_currentNode );
        walkaround.SetSolidsOnly( false );
        walkaround.SetIterationLimit( 10 );
        walkaround.SetApproachCursor( true, aP );
        walkaround.Route( initTrack, l2 );
        aNewHead = l2.ClipToNearestObstacle( m_shove->CurrentNode() );

        return false;
    }

    return false;
}


bool LINE_PLACER::routeHead( const VECTOR2I& aP, LINE& aNewHead )
{
    switch( m_currentMode )
    {
    case RM_MarkObstacles:
        return rhMarkObstacles( aP, aNewHead );
    case RM_Walkaround:
        return rhWalkOnly( aP, aNewHead );
    case RM_Shove:
        return rhShoveOnly( aP, aNewHead );
    default:
        break;
    }

    return false;
}


bool LINE_PLACER::optimizeTailHeadTransition()
{
    LINE linetmp = Trace();

    if( OPTIMIZER::Optimize( &linetmp, OPTIMIZER::FANOUT_CLEANUP, m_currentNode ) )
    {
        if( linetmp.SegmentCount() < 1 )
            return false;

        m_head = linetmp;
        m_p_start = linetmp.CLine().CPoint( 0 );
        m_direction = DIRECTION_45( linetmp.CSegment( 0 ) );
        m_tail.Line().Clear();

        return true;
    }

    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    int tailLookbackSegments = 3;

    //if(m_currentMode() == RM_Walkaround)
    //    tailLookbackSegments = 10000;

    int threshold = std::min( tail.PointCount(), tailLookbackSegments + 1 );

    if( tail.SegmentCount() < 3 )
        return false;

    // assemble TailLookbackSegments tail segments with the current head
    SHAPE_LINE_CHAIN opt_line = tail.Slice( -threshold, -1 );

    int end = std::min(2, head.PointCount() - 1 );

    opt_line.Append( head.Slice( 0, end ) );

    LINE new_head( m_tail, opt_line );

    // and see if it could be made simpler by merging obtuse/collnear segments.
    // If so, replace the (threshold) last tail points and the head with
    // the optimized line

    if( OPTIMIZER::Optimize( &new_head, OPTIMIZER::MERGE_OBTUSE, m_currentNode ) )
    {
        LINE tmp( m_tail, opt_line );

        wxLogTrace( "PNS", "Placer: optimize tail-head [%d]", threshold );

        head.Clear();
        tail.Replace( -threshold, -1, new_head.CLine() );
        tail.Simplify();

        m_p_start = new_head.CLine().CPoint( -1 );
        m_direction = DIRECTION_45( new_head.CSegment( -1 ) );

        return true;
    }

    return false;
}


void LINE_PLACER::routeStep( const VECTOR2I& aP )
{
    bool fail = false;
    bool go_back = false;

    int i, n_iter = 1;

    LINE new_head;

    wxLogTrace( "PNS", "INIT-DIR: %s head: %d, tail: %d segs",
            m_initial_direction.Format().c_str(), m_head.SegmentCount(), m_tail.SegmentCount() );

    for( i = 0; i < n_iter; i++ )
    {
        if( !go_back && Settings().FollowMouse() )
            reduceTail( aP );

        go_back = false;

        if( !routeHead( aP, new_head ) )
            fail = true;

        if( !new_head.Is45Degree() )
            fail = true;

        if( !Settings().FollowMouse() )
            return;

        m_head = new_head;

        if( handleSelfIntersections() )
        {
            n_iter++;
            go_back = true;
        }

        if( !go_back && handlePullback() )
        {
            n_iter++;
            go_back = true;
        }
    }

    if( !fail )
    {
       if( optimizeTailHeadTransition() )
          return;

        mergeHead();
    }
}


bool LINE_PLACER::route( const VECTOR2I& aP )
{
    routeStep( aP );
    return CurrentEnd() == aP;
}


const LINE LINE_PLACER::Trace() const
{
    LINE tmp( m_head );

    tmp.SetShape( m_tail.CLine() );
    tmp.Line().Append( m_head.CLine() );
    tmp.Line().Simplify();
    return tmp;
}


const ITEM_SET LINE_PLACER::Traces()
{
    m_currentTrace = Trace();
    return ITEM_SET( &m_currentTrace );
}


void LINE_PLACER::FlipPosture()
{
    m_initial_direction = m_initial_direction.Right();
    m_direction = m_direction.Right();
}


NODE* LINE_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( aLoopsRemoved && m_lastNode )
        return m_lastNode;

    return m_currentNode;
}


bool LINE_PLACER::SplitAdjacentSegments( NODE* aNode, ITEM* aSeg, const VECTOR2I& aP )
{
    if( !aSeg )
        return false;

    if( !aSeg->OfKind( ITEM::SEGMENT_T ) )
        return false;

    JOINT* jt = aNode->FindJoint( aP, aSeg );

    if( jt && jt->LinkCount() >= 1 )
        return false;

    SEGMENT* s_old = static_cast<SEGMENT*>( aSeg );

    std::unique_ptr< SEGMENT > s_new[2] = {
        Clone( *s_old ),
        Clone( *s_old )
    };

    s_new[0]->SetEnds( s_old->Seg().A, aP );
    s_new[1]->SetEnds( aP, s_old->Seg().B );

    aNode->Remove( s_old );
    aNode->Add( std::move( s_new[0] ), true );
    aNode->Add( std::move( s_new[1] ), true );

    return true;
}


bool LINE_PLACER::SetLayer( int aLayer )
{
    if( m_idle )
    {
        m_currentLayer = aLayer;
        return true;
    }
    else if( m_chainedPlacement )
    {
        return false;
    }
    else if( !m_startItem || ( m_startItem->OfKind( ITEM::VIA_T ) && m_startItem->Layers().Overlaps( aLayer ) ) )
    {
        m_currentLayer = aLayer;
        initPlacement();
        Move( m_currentEnd, NULL );
        return true;
    }

    return false;
}


bool LINE_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    m_currentStart = VECTOR2I( aP );
    m_currentEnd = VECTOR2I( aP );
    m_currentNet = std::max( 0, aStartItem ? aStartItem->Net() : 0 );
    m_startItem = aStartItem;
    m_placingVia = false;
    m_chainedPlacement = false;

    setInitialDirection( Settings().InitialDirection() );

    initPlacement();
    return true;
}


void LINE_PLACER::initPlacement()
{
    m_idle = false;

    m_head.Line().Clear();
    m_tail.Line().Clear();
    m_head.SetNet( m_currentNet );
    m_tail.SetNet( m_currentNet );
    m_head.SetLayer( m_currentLayer );
    m_tail.SetLayer( m_currentLayer );
    m_head.SetWidth( m_sizes.TrackWidth() );
    m_tail.SetWidth( m_sizes.TrackWidth() );
    m_head.RemoveVia();
    m_tail.RemoveVia();

    m_p_start = m_currentStart;
    m_direction = m_initial_direction;

    NODE* world = Router()->GetWorld();

    world->KillChildren();
    NODE* rootNode = world->Branch();

    SplitAdjacentSegments( rootNode, m_startItem, m_currentStart );

    setWorld( rootNode );

    wxLogTrace( "PNS", "world %p, intitial-direction %s layer %d",
            m_world, m_direction.Format().c_str(), m_currentLayer );

    m_lastNode = NULL;
    m_currentNode = m_world;
    m_currentMode = Settings().Mode();

    m_shove.reset();

    if( m_currentMode == RM_Shove || m_currentMode == RM_Smart )
    {
        m_shove.reset( new SHOVE( m_world->Branch(), Router() ) );
    }
}


bool LINE_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    LINE current;
    VECTOR2I p = aP;
    int eiDepth = -1;

    if( aEndItem && aEndItem->Owner() )
        eiDepth = static_cast<NODE*>( aEndItem->Owner() )->Depth();

    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = NULL;
    }

    route( p );

    current = Trace();

    if( !current.PointCount() )
        m_currentEnd = m_p_start;
    else
        m_currentEnd = current.CLine().CPoint( -1 );

    NODE* latestNode = m_currentNode;
    m_lastNode = latestNode->Branch();

    if( eiDepth >= 0 && aEndItem && latestNode->Depth() > eiDepth && current.SegmentCount() )
    {
        SplitAdjacentSegments( m_lastNode, aEndItem, current.CPoint( -1 ) );

        if( Settings().RemoveLoops() )
            removeLoops( m_lastNode, current );
    }

    updateLeadingRatLine();
    return true;
}


bool LINE_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    bool realEnd = false;
    int lastV;

    LINE pl = Trace();

    if( m_currentMode == RM_MarkObstacles )
    {
        // Mark Obstacles is sort of a half-manual, half-automated mode in which the
        // user has more responsibility and authority.

        if( aEndItem )
        {
            // The user has indicated a connection should be made.  If either the
            // trace or endItem is netless, then allow the connection by adopting the net of the other.
            if( m_currentNet <= 0 )
            {
                m_currentNet = aEndItem->Net();
                pl.SetNet( m_currentNet );
            }
            else if (aEndItem->Net() <= 0 )
                aEndItem->SetNet( m_currentNet );
        }

        // Collisions still prevent fixing unless "Allow DRC violations" is checked
        if( !Settings().CanViolateDRC() && m_world->CheckColliding( &pl ) )
            return false;
    }

    const SHAPE_LINE_CHAIN& l = pl.CLine();

    if( !l.SegmentCount() )
    {
        if( pl.EndsWithVia() )
        {
            m_lastNode->Add( Clone( pl.Via() ) );
            Router()->CommitRouting( m_lastNode );

            m_lastNode = NULL;
            m_currentNode = NULL;

            m_idle = true;
        }

        return true;
    }

    VECTOR2I p_pre_last = l.CPoint( -1 );
    const VECTOR2I p_last = l.CPoint( -1 );
    DIRECTION_45 d_last( l.CSegment( -1 ) );

    if( l.PointCount() > 2 )
        p_pre_last = l.CPoint( -2 );

    if( aEndItem && m_currentNet >= 0 && m_currentNet == aEndItem->Net() )
        realEnd = true;

    if( aForceFinish )
        realEnd = true;

    if( realEnd || m_placingVia )
        lastV = l.SegmentCount();
    else
        lastV = std::max( 1, l.SegmentCount() - 1 );

    SEGMENT* lastSeg = nullptr;

    for( int i = 0; i < lastV; i++ )
    {
        const SEG& s = pl.CSegment( i );
        lastSeg = new SEGMENT( s, m_currentNet );
        std::unique_ptr< SEGMENT > seg( lastSeg );
        seg->SetWidth( pl.Width() );
        seg->SetLayer( m_currentLayer );
        if( ! m_lastNode->Add( std::move( seg ) ) )
        {
            lastSeg = nullptr;
        }
    }

    if( pl.EndsWithVia() )
        m_lastNode->Add( Clone( pl.Via() ) );

    if( realEnd && lastSeg )
        simplifyNewLine( m_lastNode, lastSeg );

    Router()->CommitRouting( m_lastNode );

    m_lastNode = NULL;
    m_currentNode = NULL;

    if( !realEnd )
    {
        setInitialDirection( d_last );
        m_currentStart = m_placingVia ? p_last : p_pre_last;
        m_startItem = NULL;
        m_placingVia = false;
        m_chainedPlacement = !pl.EndsWithVia();
        initPlacement();
    }
    else
    {
        m_idle = true;
    }

    return realEnd;
}


void LINE_PLACER::removeLoops( NODE* aNode, LINE& aLatest )
{
    if( !aLatest.SegmentCount() )
        return;

    if( aLatest.CLine().CPoint( 0 ) == aLatest.CLine().CPoint( -1 ) )
        return;

    std::set<SEGMENT *> toErase;
    aNode->Add( aLatest, true );

    for( int s = 0; s < aLatest.LinkCount(); s++ )
    {
        SEGMENT* seg = aLatest.GetLink(s);
        LINE ourLine = aNode->AssembleLine( seg );
        JOINT a, b;
        std::vector<LINE> lines;

        aNode->FindLineEnds( ourLine, a, b );

        if( a == b )
        {
            aNode->FindLineEnds( aLatest, a, b );
        }

        aNode->FindLinesBetweenJoints( a, b, lines );

        int removedCount = 0;
        int total = 0;

        for( LINE& line : lines )
        {
            total++;

            if( !( line.ContainsSegment( seg ) ) && line.SegmentCount() )
            {
                for( SEGMENT *ss : line.LinkedSegments() )
                    toErase.insert( ss );

                removedCount++;
            }
        }

        wxLogTrace( "PNS", "total segs removed: %d/%d", removedCount, total );
    }

    for( SEGMENT *s : toErase )
        aNode->Remove( s );

    aNode->Remove( aLatest );
}


void LINE_PLACER::simplifyNewLine( NODE* aNode, SEGMENT* aLatest )
{
    LINE l = aNode->AssembleLine( aLatest );
    SHAPE_LINE_CHAIN simplified( l.CLine() );

    simplified.Simplify();

    if( simplified.PointCount() != l.PointCount() )
    {
        aNode->Remove( l );
        l.SetShape( simplified );
        aNode->Add( l );
    }
}


void LINE_PLACER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    if( !m_idle )
    {
        initPlacement();
    }
}


void LINE_PLACER::updateLeadingRatLine()
{
    LINE current = Trace();
    SHAPE_LINE_CHAIN ratLine;
    TOPOLOGY topo( m_lastNode );

    if( topo.LeadingRatLine( &current, ratLine ) )
        Dbg()->AddLine( ratLine, 5, 10000 );
}


void LINE_PLACER::SetOrthoMode( bool aOrthoMode )
{
    m_orthoMode = aOrthoMode;
}


bool LINE_PLACER::buildInitialLine( const VECTOR2I& aP, LINE& aHead, bool aInvertPosture )
{
    SHAPE_LINE_CHAIN l;

    if( m_p_start == aP )
    {
        l.Clear();
    }
    else
    {
        if( Settings().GetFreeAngleMode() && Settings().Mode() == RM_MarkObstacles )
        {
            l = SHAPE_LINE_CHAIN( m_p_start, aP );
        }
        else
        {
            if ( aInvertPosture )
                l = m_direction.Right().BuildInitialTrace( m_p_start, aP );
            else
                l = m_direction.BuildInitialTrace( m_p_start, aP );
        }

        if( l.SegmentCount() > 1 && m_orthoMode )
        {
            VECTOR2I newLast = l.CSegment( 0 ).LineProject( l.CPoint( -1 ) );

            l.Remove( -1, -1 );
            l.Point( 1 ) = newLast;
        }
    }

    aHead.SetShape( l );

    if( !m_placingVia )
        return true;

    VIA v( makeVia( aP ) );
    v.SetNet( aHead.Net() );

    if( m_currentMode == RM_MarkObstacles )
    {
        aHead.AppendVia( v );
        return true;
    }

    VECTOR2I force;
    VECTOR2I lead = aP - m_p_start;

    bool solidsOnly = ( m_currentMode != RM_Walkaround );

    if( v.PushoutForce( m_currentNode, lead, force, solidsOnly, 40 ) )
    {
        SHAPE_LINE_CHAIN line = m_direction.BuildInitialTrace( m_p_start, aP + force );
        aHead = LINE( aHead, line );

        v.SetPos( v.Pos() + force );
        return true;
    }

    return false; // via placement unsuccessful
}


void LINE_PLACER::GetModifiedNets( std::vector<int>& aNets ) const
{
    aNets.push_back( m_currentNet );
}

LOGGER* LINE_PLACER::Logger()
{
    if( m_shove )
        return m_shove->Logger();

    return NULL;
}

}

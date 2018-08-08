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

#define PNS_DEBUG

#include <deque>
#include <cassert>

#include "range.h"

#include "pns_line.h"
#include "pns_node.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_solid.h"
#include "pns_optimizer.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_utils.h"
#include "pns_topology.h"

#include "time_limit.h"


namespace PNS {

void SHOVE::replaceItems( ITEM* aOld, std::unique_ptr< ITEM > aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew.get() );

    if( changed_area )
    {
        m_affectedAreaSum = m_affectedAreaSum ? m_affectedAreaSum->Merge( *changed_area ) : *changed_area;
    }

    m_currentNode->Replace( aOld, std::move( aNew ) );
}

void SHOVE::replaceLine( LINE& aOld, LINE& aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew );

    if( changed_area )
    {
        m_affectedAreaSum = m_affectedAreaSum ? m_affectedAreaSum->Merge( *changed_area ) : *changed_area;
    }

    m_currentNode->Replace( aOld, aNew );
}

int SHOVE::getClearance( const ITEM* aA, const ITEM* aB ) const
{
    if( m_forceClearance >= 0 )
        return m_forceClearance;

    return m_currentNode->GetClearance( aA, aB );
}


void SHOVE::sanityCheck( LINE* aOld, LINE* aNew )
{
    assert( aOld->CPoint( 0 ) == aNew->CPoint( 0 ) );
    assert( aOld->CPoint( -1 ) == aNew->CPoint( -1 ) );
}


SHOVE::SHOVE( NODE* aWorld, ROUTER* aRouter ) :
    ALGO_BASE( aRouter )
{
    m_forceClearance = -1;
    m_root = aWorld;
    m_currentNode = aWorld;

    // Initialize other temporary variables:
    m_draggedVia = NULL;
    m_iter = 0;
    m_multiLineMode = false;
}


SHOVE::~SHOVE()
{
}


LINE SHOVE::assembleLine( const SEGMENT* aSeg, int* aIndex )
{
    return m_currentNode->AssembleLine( const_cast<SEGMENT*>( aSeg ), aIndex, true );
}

// A dumb function that checks if the shoved line is shoved the right way, e.g.
// visually "outwards" of the line/via applying pressure on it. Unfortunately there's no
// mathematical concept of orientation of an open curve, so we use some primitive heuristics:
// if the shoved line wraps around the start of the "pusher", it's likely shoved in wrong direction.
bool SHOVE::checkBumpDirection( const LINE& aCurrent, const LINE& aShoved ) const
{
    const SEG& ss = aCurrent.CSegment( 0 );

    int dist = getClearance( &aCurrent, &aShoved ) + PNS_HULL_MARGIN;

    dist += aCurrent.Width() / 2;
    dist += aShoved.Width() / 2;

    const VECTOR2I ps = ss.A - ( ss.B - ss.A ).Resize( dist );

    return !aShoved.CLine().PointOnEdge( ps );
}


SHOVE::SHOVE_STATUS SHOVE::walkaroundLoneVia( LINE& aCurrent, LINE& aObstacle,
                                                      LINE& aShoved )
{
    int clearance = getClearance( &aCurrent, &aObstacle );
    const SHAPE_LINE_CHAIN hull = aCurrent.Via().Hull( clearance, aObstacle.Width() );
    SHAPE_LINE_CHAIN path_cw, path_ccw;

    if( ! aObstacle.Walkaround( hull, path_cw, true ) )
        return SH_INCOMPLETE;

    if( ! aObstacle.Walkaround( hull, path_ccw, false ) )
        return SH_INCOMPLETE;

    const SHAPE_LINE_CHAIN& shortest = path_ccw.Length() < path_cw.Length() ? path_ccw : path_cw;

    if( shortest.PointCount() < 2 )
        return SH_INCOMPLETE;

    if( aObstacle.CPoint( -1 ) != shortest.CPoint( -1 ) )
        return SH_INCOMPLETE;

    if( aObstacle.CPoint( 0 ) != shortest.CPoint( 0 ) )
        return SH_INCOMPLETE;

    aShoved.SetShape( shortest );

    if( m_currentNode->CheckColliding( &aShoved, &aCurrent ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


SHOVE::SHOVE_STATUS SHOVE::processHullSet( LINE& aCurrent, LINE& aObstacle,
                                                   LINE& aShoved, const HULL_SET& aHulls )
{
    const SHAPE_LINE_CHAIN& obs = aObstacle.CLine();

    int attempt;

    for( attempt = 0; attempt < 4; attempt++ )
    {
        bool invertTraversal = ( attempt >= 2 );
        bool clockwise = attempt % 2;
        int vFirst = -1, vLast = -1;

        SHAPE_LINE_CHAIN path;
        LINE l( aObstacle );

        for( int i = 0; i < (int) aHulls.size(); i++ )
        {
            const SHAPE_LINE_CHAIN& hull = aHulls[invertTraversal ? aHulls.size() - 1 - i : i];

            if( ! l.Walkaround( hull, path, clockwise ) )
                return SH_INCOMPLETE;

            path.Simplify();
            l.SetShape( path );
        }

        for( int i = 0; i < std::min( path.PointCount(), obs.PointCount() ); i++ )
        {
            if( path.CPoint( i ) != obs.CPoint( i ) )
            {
                vFirst = i;
                break;
            }
        }

        int k = obs.PointCount() - 1;
        for( int i = path.PointCount() - 1; i >= 0 && k >= 0; i--, k-- )
        {
            if( path.CPoint( i ) != obs.CPoint( k ) )
            {
                vLast = i;
                break;
            }
        }

        if( ( vFirst < 0 || vLast < 0 ) && !path.CompareGeometry( aObstacle.CLine() ) )
        {
            wxLogTrace( "PNS", "attempt %d fail vfirst-last", attempt );
            continue;
        }

        if( path.CPoint( -1 ) != obs.CPoint( -1 ) || path.CPoint( 0 ) != obs.CPoint( 0 ) )
        {
            wxLogTrace( "PNS", "attempt %d fail vend-start\n", attempt );
            continue;
        }

        if( !checkBumpDirection( aCurrent, l ) )
        {
            wxLogTrace( "PNS", "attempt %d fail direction-check", attempt );
            aShoved.SetShape( l.CLine() );

            continue;
        }

        if( path.SelfIntersecting() )
        {
            wxLogTrace( "PNS", "attempt %d fail self-intersect", attempt );
            continue;
        }

        bool colliding = m_currentNode->CheckColliding( &l, &aCurrent, ITEM::ANY_T, m_forceClearance );

        if( ( aCurrent.Marker() & MK_HEAD ) && !colliding )
        {
            JOINT* jtStart = m_currentNode->FindJoint( aCurrent.CPoint( 0 ), &aCurrent );

            for( ITEM* item : jtStart->LinkList() )
            {
                if( m_currentNode->CheckColliding( item, &l ) )
                    colliding = true;
            }
        }

        if( colliding )
        {
            wxLogTrace( "PNS", "attempt %d fail coll-check", attempt );
            continue;
        }

        aShoved.SetShape( l.CLine() );

        return SH_OK;
    }

    return SH_INCOMPLETE;
}


SHOVE::SHOVE_STATUS SHOVE::ProcessSingleLine( LINE& aCurrent, LINE& aObstacle,
                                                      LINE& aShoved )
{
    aShoved.ClearSegmentLinks();

    bool obstacleIsHead = false;

    for( SEGMENT* s : aObstacle.LinkedSegments() )
    {
        if( s->Marker() & MK_HEAD )
        {
            obstacleIsHead = true;
            break;
        }
    }

    SHOVE_STATUS rv;

    bool viaOnEnd = aCurrent.EndsWithVia();

    if( viaOnEnd && ( !aCurrent.LayersOverlap( &aObstacle ) || aCurrent.SegmentCount() == 0 ) )
    {
        rv = walkaroundLoneVia( aCurrent, aObstacle, aShoved );
    }
    else
    {
        int w = aObstacle.Width();
        int n_segs = aCurrent.SegmentCount();

        int clearance = getClearance( &aCurrent, &aObstacle ) + 1;

        HULL_SET hulls;

        hulls.reserve( n_segs + 1 );

        for( int i = 0; i < n_segs; i++ )
        {
            SEGMENT seg( aCurrent, aCurrent.CSegment( i ) );
            SHAPE_LINE_CHAIN hull = seg.Hull( clearance, w );

            hulls.push_back( hull );
        }

        if( viaOnEnd )
            hulls.push_back( aCurrent.Via().Hull( clearance, w ) );

        rv = processHullSet( aCurrent, aObstacle, aShoved, hulls );
    }

    if( obstacleIsHead )
        aShoved.Mark( aShoved.Marker() | MK_HEAD );

    return rv;
}


SHOVE::SHOVE_STATUS SHOVE::onCollidingSegment( LINE& aCurrent, SEGMENT* aObstacleSeg )
{
    int segIndex;
    LINE obstacleLine = assembleLine( aObstacleSeg, &segIndex );
    LINE shovedLine( obstacleLine );
    SEGMENT tmp( *aObstacleSeg );

    if( obstacleLine.HasLockedSegments() )
        return SH_TRY_WALK;

    SHOVE_STATUS rv = ProcessSingleLine( aCurrent, obstacleLine, shovedLine );

    const double extensionWalkThreshold = 1.0;

    double obsLen = obstacleLine.CLine().Length();
    double shovedLen = shovedLine.CLine().Length();
    double extensionFactor = 0.0;

    if( obsLen != 0.0f )
        extensionFactor = shovedLen / obsLen - 1.0;

    if( extensionFactor > extensionWalkThreshold )
        return SH_TRY_WALK;

    assert( obstacleLine.LayersOverlap( &shovedLine ) );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-segment", m_iter );
    m_logger.Log( &tmp, 0, "obstacle-segment" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &obstacleLine, 2, "obstacle-line" );
    m_logger.Log( &shovedLine, 3, "shoved-line" );
#endif

    if( rv == SH_OK )
    {
        if( shovedLine.Marker() & MK_HEAD )
        {
            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = shovedLine;
        }

        int rank = aCurrent.Rank();
        shovedLine.SetRank( rank - 1 );

        sanityCheck( &obstacleLine, &shovedLine );
        replaceLine( obstacleLine, shovedLine );

        if( !pushLine( shovedLine ) )
            rv = SH_INCOMPLETE;
    }

    return rv;
}


SHOVE::SHOVE_STATUS SHOVE::onCollidingLine( LINE& aCurrent, LINE& aObstacle )
{
    LINE shovedLine( aObstacle );

    SHOVE_STATUS rv = ProcessSingleLine( aCurrent, aObstacle, shovedLine );

    #ifdef DEBUG
        m_logger.NewGroup( "on-colliding-line", m_iter );
        m_logger.Log( &aObstacle, 0, "obstacle-line" );
        m_logger.Log( &aCurrent, 1, "current-line" );
        m_logger.Log( &shovedLine, 3, "shoved-line" );
    #endif

    if( rv == SH_OK )
    {
        if( shovedLine.Marker() & MK_HEAD )
        {
            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = shovedLine;
        }

        sanityCheck( &aObstacle, &shovedLine );
        replaceLine( aObstacle, shovedLine );

        int rank = aObstacle.Rank();
        shovedLine.SetRank( rank - 1 );


        if( !pushLine( shovedLine ) )
        {
            rv = SH_INCOMPLETE;
        }
    }

    return rv;
}

SHOVE::SHOVE_STATUS SHOVE::onCollidingSolid( LINE& aCurrent, ITEM* aObstacle )
{
    WALKAROUND walkaround( m_currentNode, Router() );
    LINE walkaroundLine( aCurrent );

    if( aCurrent.EndsWithVia() )
    {
        VIA vh = aCurrent.Via();
        VIA* via = NULL;
        JOINT* jtStart = m_currentNode->FindJoint( vh.Pos(), &aCurrent );

        if( !jtStart )
            return SH_INCOMPLETE;

        for( ITEM* item : jtStart->LinkList() )
        {
            if( item->OfKind( ITEM::VIA_T ) )
            {
                via = (VIA*) item;
                break;
            }
        }

        if( via && m_currentNode->CheckColliding( via, aObstacle ) )
            return onCollidingVia( aObstacle, via );
    }

    TOPOLOGY topo( m_currentNode );

    std::set<ITEM*> cluster = topo.AssembleCluster( aObstacle, aCurrent.Layers().Start() );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid-cluster", m_iter );
    for( ITEM* item : cluster )
    {
        m_logger.Log( item, 0, "cluster-entry" );
    }
#endif

    walkaround.SetSolidsOnly( false );
    walkaround.RestrictToSet( true, cluster );
    walkaround.SetIterationLimit( 16 ); // fixme: make configurable

    int currentRank = aCurrent.Rank();
    int nextRank;

    bool success = false;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        if( attempt == 1 || Settings().JumpOverObstacles() )
        {

            nextRank = currentRank - 1;
            walkaround.SetSingleDirection( true );
        }
        else
        {
            nextRank = currentRank + 10000;
            walkaround.SetSingleDirection( false );
        }


    	WALKAROUND::WALKAROUND_STATUS status = walkaround.Route( aCurrent, walkaroundLine, false );

        if( status != WALKAROUND::DONE )
            continue;

        walkaroundLine.ClearSegmentLinks();
        walkaroundLine.Unmark();
    	walkaroundLine.Line().Simplify();

    	if( walkaroundLine.HasLoops() )
            continue;

    	if( aCurrent.Marker() & MK_HEAD )
    	{
            walkaroundLine.Mark( MK_HEAD );

            if( m_multiLineMode )
                continue;

            m_newHead = walkaroundLine;
        }

    	sanityCheck( &aCurrent, &walkaroundLine );

        if( !m_lineStack.empty() )
        {
            LINE lastLine = m_lineStack.front();

            if( m_currentNode->CheckColliding( &lastLine, &walkaroundLine ) )
            {
                LINE dummy( lastLine );

                if( ProcessSingleLine( walkaroundLine, lastLine, dummy ) == SH_OK )
                {
                    success = true;
                    break;
                }
            } else {
                success = true;
                break;
            }
        }
    }

    if(!success)
        return SH_INCOMPLETE;

    replaceLine( aCurrent, walkaroundLine );
    walkaroundLine.SetRank( nextRank );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid", m_iter );
    m_logger.Log( aObstacle, 0, "obstacle-solid" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &walkaroundLine, 3, "walk-line" );
#endif

    popLine();

    if( !pushLine( walkaroundLine ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


bool SHOVE::reduceSpringback( const ITEM_SET& aHeadSet )
{
    bool rv = false;

    while( !m_nodeStack.empty() )
    {
        SPRINGBACK_TAG spTag = m_nodeStack.back();

        if( !spTag.m_node->CheckColliding( aHeadSet ) )
        {
            rv = true;

            delete spTag.m_node;
            m_nodeStack.pop_back();
        }
        else
           break;
    }

    return rv;
}


bool SHOVE::pushSpringback( NODE* aNode, const ITEM_SET& aHeadItems,
                                const COST_ESTIMATOR& aCost, const OPT_BOX2I& aAffectedArea )
{
    SPRINGBACK_TAG st;
    OPT_BOX2I prev_area;

    if( !m_nodeStack.empty() )
        prev_area = m_nodeStack.back().m_affectedArea;

    st.m_node = aNode;
    st.m_cost = aCost;
    st.m_headItems = aHeadItems;

    if( aAffectedArea )
    {
        if( prev_area )
            st.m_affectedArea = prev_area->Merge( *aAffectedArea );
        else
            st.m_affectedArea = aAffectedArea;
    } else
        st.m_affectedArea = prev_area;

    m_nodeStack.push_back( st );

    return true;
}


SHOVE::SHOVE_STATUS SHOVE::pushVia( VIA* aVia, const VECTOR2I& aForce, int aCurrentRank, bool aDryRun )
{
    LINE_PAIR_VEC draggedLines;
    VECTOR2I p0( aVia->Pos() );
    JOINT* jt = m_currentNode->FindJoint( p0, aVia );
    VECTOR2I p0_pushed( p0 + aForce );

    if( !jt )
    {
        wxLogTrace( "PNS", "weird, can't find the center-of-via joint\n" );
        return SH_INCOMPLETE;
    }

    if( aVia->IsLocked() )
        return SH_TRY_WALK;

    if( jt->IsLocked() )
        return SH_INCOMPLETE;

    // nothing to push...
    if ( aForce.x == 0 && aForce.y == 0 )
        return SH_OK;

    while( aForce.x != 0 || aForce.y != 0 )
    {
        JOINT* jt_next = m_currentNode->FindJoint( p0_pushed, aVia );

        if( !jt_next )
            break;

        p0_pushed += aForce.Resize( 2 ); // make sure pushed via does not overlap with any existing joint
    }

    std::unique_ptr< VIA > pushedVia = Clone( *aVia );
    pushedVia->SetPos( p0_pushed );
    pushedVia->Mark( aVia->Marker() );

    for( ITEM* item : jt->LinkList() )
    {
        if( SEGMENT* seg = dynamic_cast<SEGMENT*>( item ) )
        {
            LINE_PAIR lp;
            int segIndex;

            lp.first = assembleLine( seg, &segIndex );

            if( lp.first.HasLockedSegments() )
                return SH_TRY_WALK;

            assert( segIndex == 0 || ( segIndex == ( lp.first.SegmentCount() - 1 ) ) );

            if( segIndex == 0 )
                lp.first.Reverse();

            lp.second = lp.first;
            lp.second.ClearSegmentLinks();
            lp.second.DragCorner( p0_pushed, lp.second.CLine().Find( p0 ) );
            lp.second.AppendVia( *pushedVia );
            draggedLines.push_back( lp );

            if( aVia->Marker() & MK_HEAD )
                m_draggedViaHeadSet.Add( lp.second );
        }
    }

    m_draggedViaHeadSet.Add( pushedVia.get() );

    if( aDryRun )
        return SH_OK;

#ifdef DEBUG
    m_logger.Log( aVia, 0, "obstacle-via" );
#endif

    pushedVia->SetRank( aCurrentRank - 1 );

#ifdef DEBUG
    m_logger.Log( pushedVia.get(), 1, "pushed-via" );
#endif

    if( aVia->Marker() & MK_HEAD )
    {
        m_draggedVia = pushedVia.get();
        m_draggedViaHeadSet.Clear();
    }

    replaceItems( aVia, std::move( pushedVia ) );

    for( LINE_PAIR lp : draggedLines )
    {
        if( lp.first.Marker() & MK_HEAD )
        {
            lp.second.Mark( MK_HEAD );

            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = lp.second;
        }

        unwindStack( &lp.first );

        if( lp.second.SegmentCount() )
        {
            replaceLine( lp.first, lp.second );
            lp.second.SetRank( aCurrentRank - 1 );

            if( !pushLine( lp.second, true ) )
                return SH_INCOMPLETE;
        }
        else
        {
            m_currentNode->Remove( lp.first );
        }

#ifdef DEBUG
        m_logger.Log( &lp.first, 2, "fan-pre" );
        m_logger.Log( &lp.second, 3, "fan-post" );
#endif
    }

    return SH_OK;
}


SHOVE::SHOVE_STATUS SHOVE::onCollidingVia( ITEM* aCurrent, VIA* aObstacleVia )
{
    int clearance = getClearance( aCurrent, aObstacleVia ) ;
    LINE_PAIR_VEC draggedLines;
    bool colLine = false, colVia = false;
    LINE* currentLine = NULL;
    VECTOR2I mtvLine, mtvVia, mtv, mtvSolid;
    int rank = -1;

    if( aCurrent->OfKind( ITEM::LINE_T ) )
    {
#ifdef DEBUG
         m_logger.NewGroup( "push-via-by-line", m_iter );
         m_logger.Log( aCurrent, 4, "current" );
#endif

        currentLine = (LINE*) aCurrent;
        colLine = CollideShapes( aObstacleVia->Shape(), currentLine->Shape(),
                                 clearance + currentLine->Width() / 2 + PNS_HULL_MARGIN,
                                 true, mtvLine );

        if( currentLine->EndsWithVia() )
             colVia = CollideShapes( currentLine->Via().Shape(), aObstacleVia->Shape(),
                                     clearance + PNS_HULL_MARGIN, true, mtvVia );

        if( !colLine && !colVia )
             return SH_OK;

        if( colLine && colVia )
            mtv = mtvVia.EuclideanNorm() > mtvLine.EuclideanNorm() ? mtvVia : mtvLine;
        else if( colLine )
            mtv = mtvLine;
        else
            mtv = mtvVia;

        rank = currentLine->Rank();
    }
    else if( aCurrent->OfKind( ITEM::SOLID_T ) )
    {
        CollideShapes( aObstacleVia->Shape(), aCurrent->Shape(),
                       clearance + PNS_HULL_MARGIN, true, mtvSolid );
        mtv = -mtvSolid;
        rank = aCurrent->Rank() + 10000;
    }

    return pushVia( aObstacleVia, mtv, rank );
}


SHOVE::SHOVE_STATUS SHOVE::onReverseCollidingVia( LINE& aCurrent, VIA* aObstacleVia )
{
    int n = 0;
    LINE cur( aCurrent );
    cur.ClearSegmentLinks();

    JOINT* jt = m_currentNode->FindJoint( aObstacleVia->Pos(), aObstacleVia );
    LINE shoved( aCurrent );
    shoved.ClearSegmentLinks();

    cur.RemoveVia();
    unwindStack( &aCurrent );

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T ) && item->LayersOverlap( &aCurrent ) )
        {
            SEGMENT* seg = (SEGMENT*) item;
            LINE head = assembleLine( seg );

            head.AppendVia( *aObstacleVia );

            SHOVE_STATUS st = ProcessSingleLine( head, cur, shoved );

            if( st != SH_OK )
            {
#ifdef DEBUG
                m_logger.NewGroup( "on-reverse-via-fail-shove", m_iter );
                m_logger.Log( aObstacleVia, 0, "the-via" );
                m_logger.Log( &aCurrent, 1, "current-line" );
                m_logger.Log( &shoved, 3, "shoved-line" );
#endif

                return st;
            }

            cur.SetShape( shoved.CLine() );
            n++;
        }
    }

    if( !n )
    {
#ifdef DEBUG
        m_logger.NewGroup( "on-reverse-via-fail-lonevia", m_iter );
        m_logger.Log( aObstacleVia, 0, "the-via" );
        m_logger.Log( &aCurrent, 1, "current-line" );
#endif

        LINE head( aCurrent );
        head.Line().Clear();
        head.AppendVia( *aObstacleVia );
        head.ClearSegmentLinks();

        SHOVE_STATUS st = ProcessSingleLine( head, aCurrent, shoved );

        if( st != SH_OK )
            return st;

        cur.SetShape( shoved.CLine() );
    }

    if( aCurrent.EndsWithVia() )
        shoved.AppendVia( aCurrent.Via() );

#ifdef DEBUG
    m_logger.NewGroup( "on-reverse-via", m_iter );
    m_logger.Log( aObstacleVia, 0, "the-via" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &shoved, 3, "shoved-line" );
#endif
    int currentRank = aCurrent.Rank();
    replaceLine( aCurrent, shoved );

    if( !pushLine( shoved ) )
        return SH_INCOMPLETE;

    shoved.SetRank( currentRank );

    return SH_OK;
}


void SHOVE::unwindStack( SEGMENT* aSeg )
{
    for( std::vector<LINE>::iterator i = m_lineStack.begin(); i != m_lineStack.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_lineStack.erase( i );
        else
            i++;
    }

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_optimizerQueue.erase( i );
        else
            i++;
    }
}


void SHOVE::unwindStack( ITEM* aItem )
{
    if( aItem->OfKind( ITEM::SEGMENT_T ) )
        unwindStack( static_cast<SEGMENT*>( aItem ) );
    else if( aItem->OfKind( ITEM::LINE_T ) )
    {
        LINE* l = static_cast<LINE*>( aItem );

        for( SEGMENT* seg : l->LinkedSegments() )
            unwindStack( seg );
    }
}


bool SHOVE::pushLine( const LINE& aL, bool aKeepCurrentOnTop )
{
    if( !aL.IsLinkedChecked() && aL.SegmentCount() != 0 )
        return false;

    if( aKeepCurrentOnTop && m_lineStack.size() > 0)
    {
        m_lineStack.insert( m_lineStack.begin() + m_lineStack.size() - 1, aL );
    }
    else
    {
        m_lineStack.push_back( aL );
    }

    m_optimizerQueue.push_back( aL );

    return true;
}

void SHOVE::popLine( )
{
    LINE& l = m_lineStack.back();

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        bool found = false;

        for( SEGMENT *s : l.LinkedSegments() )
        {
            if( i->ContainsSegment( s ) )
            {
                i = m_optimizerQueue.erase( i );
                found = true;
                break;
            }
        }

        if( !found )
            i++;
    }

    m_lineStack.pop_back();
}


SHOVE::SHOVE_STATUS SHOVE::shoveIteration( int aIter )
{
    LINE currentLine = m_lineStack.back();
    NODE::OPT_OBSTACLE nearest;
    SHOVE_STATUS st = SH_NULL;

    ITEM::PnsKind search_order[] = { ITEM::SOLID_T, ITEM::VIA_T, ITEM::SEGMENT_T };

    for( int i = 0; i < 3; i++ )
    {
         nearest = m_currentNode->NearestObstacle( &currentLine, search_order[i] );

         if( nearest )
            break;
    }

    if( !nearest )
    {
        m_lineStack.pop_back();
        return SH_OK;
    }

    ITEM* ni = nearest->m_item;

    unwindStack( ni );

    if( !ni->OfKind( ITEM::SOLID_T ) && ni->Rank() >= 0 && ni->Rank() > currentLine.Rank() )
    {
        switch( ni->Kind() )
        {
        case ITEM::VIA_T:
        {
            VIA* revVia = (VIA*) ni;
            wxLogTrace( "PNS", "iter %d: reverse-collide-via", aIter );

            if( currentLine.EndsWithVia() && m_currentNode->CheckColliding( &currentLine.Via(), revVia ) )
            {
                st = SH_INCOMPLETE;
            }
            else
            {
                st = onReverseCollidingVia( currentLine, revVia );
            }

            break;
        }

        case ITEM::SEGMENT_T:
        {
            SEGMENT* seg = (SEGMENT*) ni;
            wxLogTrace( "PNS", "iter %d: reverse-collide-segment ", aIter );
            LINE revLine = assembleLine( seg );

            popLine();
            st = onCollidingLine( revLine, currentLine );
            if( !pushLine( revLine ) )
                return SH_INCOMPLETE;

            break;
        }

        default:
            assert( false );
        }
    }
    else
    { // "forward" collisions
        switch( ni->Kind() )
        {
        case ITEM::SEGMENT_T:
            wxLogTrace( "PNS", "iter %d: collide-segment ", aIter );

            st = onCollidingSegment( currentLine, (SEGMENT*) ni );

            if( st == SH_TRY_WALK )
            {
                st = onCollidingSolid( currentLine, ni );
            }
            break;

        case ITEM::VIA_T:
            wxLogTrace( "PNS", "iter %d: shove-via ", aIter );
            st = onCollidingVia( &currentLine, (VIA*) ni );

            if( st == SH_TRY_WALK )
            {
                st = onCollidingSolid( currentLine, ni );
            }
            break;

        case ITEM::SOLID_T:
            wxLogTrace( "PNS", "iter %d: walk-solid ", aIter );
            st = onCollidingSolid( currentLine, (SOLID*) ni );
            break;

        default:
            break;
        }
    }

    return st;
}


SHOVE::SHOVE_STATUS SHOVE::shoveMainLoop()
{
    SHOVE_STATUS st = SH_OK;

    m_affectedAreaSum = OPT_BOX2I();

    wxLogTrace( "PNS", "ShoveStart [root: %d jts, current: %d jts]", m_root->JointCount(),
           m_currentNode->JointCount() );

    int iterLimit = Settings().ShoveIterationLimit();
    TIME_LIMIT timeLimit = Settings().ShoveTimeLimit();

    m_iter = 0;

    timeLimit.Restart();

    while( !m_lineStack.empty() )
    {
        st = shoveIteration( m_iter );

        m_iter++;

        if( st == SH_INCOMPLETE || timeLimit.Expired() || m_iter >= iterLimit )
        {
            st = SH_INCOMPLETE;
            break;
        }
    }

    return st;
}


OPT_BOX2I SHOVE::totalAffectedArea() const
{
    OPT_BOX2I area;
    if( !m_nodeStack.empty() )
        area = m_nodeStack.back().m_affectedArea;

    if( area )
    {
        if( m_affectedAreaSum )
            area->Merge( *m_affectedAreaSum );
    } else
        area = m_affectedAreaSum;

    return area;
}


SHOVE::SHOVE_STATUS SHOVE::ShoveLines( const LINE& aCurrentHead )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = false;

    // empty head? nothing to shove...

    if( !aCurrentHead.SegmentCount() && !aCurrentHead.EndsWithVia() )
        return SH_INCOMPLETE;

    LINE head( aCurrentHead );
    head.ClearSegmentLinks();

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_logger.Clear();

    ITEM_SET headSet;
    headSet.Add( aCurrentHead );

    reduceSpringback( headSet );

    NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    m_currentNode->Add( head );

    m_currentNode->LockJoint( head.CPoint(0), &head, true );

    if( !head.EndsWithVia() )
        m_currentNode->LockJoint( head.CPoint( -1 ), &head, true );

    head.Mark( MK_HEAD );
    head.SetRank( 100000 );

    m_logger.NewGroup( "initial", 0 );
    m_logger.Log( &head, 0, "head" );

    if( head.EndsWithVia() )
    {
        std::unique_ptr< VIA >headVia = Clone( head.Via() );
        headVia->Mark( MK_HEAD );
        headVia->SetRank( 100000 );
        m_logger.Log( headVia.get(), 0, "head-via" );
        m_currentNode->Add( std::move( headVia ) );
    }

    if( !pushLine( head ) )
    {
        delete m_currentNode;
        m_currentNode = parent;

        return SH_INCOMPLETE;
    }

    st = shoveMainLoop();

    if( st == SH_OK )
    {
        runOptimizer( m_currentNode );

        if( m_newHead )
            st = m_currentNode->CheckColliding( &( *m_newHead ) ) ? SH_INCOMPLETE : SH_HEAD_MODIFIED;
        else
            st = m_currentNode->CheckColliding( &head ) ? SH_INCOMPLETE : SH_OK;
    }

    m_currentNode->RemoveByMarker( MK_HEAD );

    wxLogTrace( "PNS", "Shove status : %s after %d iterations",
           ( ( st == SH_OK || st == SH_HEAD_MODIFIED ) ? "OK" : "FAILURE"), m_iter );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, headSet, COST_ESTIMATOR(), m_affectedAreaSum );
    }
    else
    {
        delete m_currentNode;

        m_currentNode = parent;
        m_newHead = OPT_LINE();
    }

    if(m_newHead)
        m_newHead->Unmark();

    if( m_newHead && head.EndsWithVia() )
    {
        VIA v = head.Via();
        v.SetPos( m_newHead->CPoint( -1 ) );
        m_newHead->AppendVia(v);
    }

    return st;
}


SHOVE::SHOVE_STATUS SHOVE::ShoveMultiLines( const ITEM_SET& aHeadSet )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = true;

    ITEM_SET headSet;

    for( const ITEM* item : aHeadSet.CItems() )
    {
        const LINE* headOrig = static_cast<const LINE*>( item );

        // empty head? nothing to shove...
        if( !headOrig->SegmentCount() )
            return SH_INCOMPLETE;

        headSet.Add( *headOrig );
    }

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_logger.Clear();

    reduceSpringback( headSet );

    NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    int n = 0;

    for( const ITEM* item : aHeadSet.CItems() )
    {
        const LINE* headOrig = static_cast<const LINE*>( item );
        LINE head( *headOrig );
        head.ClearSegmentLinks();

        m_currentNode->Add( head );

        head.Mark( MK_HEAD );
        head.SetRank( 100000 );
        n++;

        if( !pushLine( head ) )
            return SH_INCOMPLETE;

        if( head.EndsWithVia() )
        {
            std::unique_ptr< VIA > headVia = Clone( head.Via() );
            headVia->Mark( MK_HEAD );
            headVia->SetRank( 100000 );
            m_logger.Log( headVia.get(), 0, "head-via" );
            m_currentNode->Add( std::move( headVia ) );
        }
    }

    m_logger.NewGroup( "initial", 0 );
    //m_logger.Log( head, 0, "head" );

    st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    m_currentNode->RemoveByMarker( MK_HEAD );

    wxLogTrace( "PNS", "Shove status : %s after %d iterations",
           ( st == SH_OK ? "OK" : "FAILURE"), m_iter );

    if( st == SH_OK )
    {
        pushSpringback( m_currentNode, ITEM_SET(), COST_ESTIMATOR(), m_affectedAreaSum );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}


SHOVE::SHOVE_STATUS SHOVE::ShoveDraggingVia( VIA* aVia, const VECTOR2I& aWhere,
                                                     VIA** aNewVia )
{
    SHOVE_STATUS st = SH_OK;

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_draggedVia = NULL;
    m_draggedViaHeadSet.Clear();

    NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent;

    parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();

    aVia->Mark( MK_HEAD );

    st = pushVia( aVia, ( aWhere - aVia->Pos() ), 0 );
    st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        if( aNewVia )
        {
            wxLogTrace( "PNS","setNewV %p", m_draggedVia );
            *aNewVia = m_draggedVia;
        }

        pushSpringback( m_currentNode, m_draggedViaHeadSet, COST_ESTIMATOR(), m_affectedAreaSum );
    }
    else
    {
        if( aNewVia )
        {
            *aNewVia = nullptr;
        }

        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}


void SHOVE::runOptimizer( NODE* aNode )
{
    OPTIMIZER optimizer( aNode );
    int optFlags = 0, n_passes = 0;

    PNS_OPTIMIZATION_EFFORT effort = Settings().OptimizerEffort();

    OPT_BOX2I area = totalAffectedArea();

    int maxWidth = 0;

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin();
             i != m_optimizerQueue.end(); ++i )
    {
        maxWidth = std::max( i->Width(), maxWidth );
    }

    if( area )
    {
        area->Inflate( 10 * maxWidth );
    }

    switch( effort )
    {
    case OE_LOW:
        optFlags = OPTIMIZER::MERGE_OBTUSE;
        n_passes = 1;
        break;

    case OE_MEDIUM:
        optFlags = OPTIMIZER::MERGE_SEGMENTS;

        if( area )
            optimizer.SetRestrictArea( *area );

        n_passes = 2;
        break;

    case OE_FULL:
        optFlags = OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    default:
        break;
    }

    if( Settings().SmartPads() )
        optFlags |= OPTIMIZER::SMART_PADS;

    optimizer.SetEffortLevel( optFlags );
    optimizer.SetCollisionMask( ITEM::ANY_T );

    for( int pass = 0; pass < n_passes; pass++ )
    {
        std::reverse( m_optimizerQueue.begin(), m_optimizerQueue.end() );

        for( std::vector<LINE>::iterator i = m_optimizerQueue.begin();
             i != m_optimizerQueue.end(); ++i )
        {
            LINE& line = *i;

            if( !( line.Marker() & MK_HEAD ) )
            {
                LINE optimized;

                if( optimizer.Optimize( &line, &optimized ) )
                {
                    aNode->Remove( line );
                    line.SetShape( optimized.CLine() );
                    aNode->Add( line );
                }
            }
        }
    }
}


NODE* SHOVE::CurrentNode()
{
    return m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
}


const LINE SHOVE::NewHead() const
{
    assert( m_newHead );

    return *m_newHead;
}


void SHOVE::SetInitialLine( LINE& aInitial )
{
    m_root = m_root->Branch();
    m_root->Remove( aInitial );
}

}

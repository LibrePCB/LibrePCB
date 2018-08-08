/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_node.h"
#include "pns_joint.h"
#include "pns_solid.h"
#include "pns_router.h"
#include "pns_utils.h"

#include "pns_diff_pair.h"
#include "pns_topology.h"

//#include <class_board.h>

namespace PNS {

bool TOPOLOGY::SimplifyLine( LINE* aLine )
{
    if( !aLine->IsLinked() || !aLine->SegmentCount() )
        return false;

    SEGMENT* root = aLine->GetLink(0);
    LINE l = m_world->AssembleLine( root );
    SHAPE_LINE_CHAIN simplified( l.CLine() );

    simplified.Simplify();

    if( simplified.PointCount() != l.PointCount() )
    {
        m_world->Remove( l );
        LINE lnew( l );
        lnew.SetShape( simplified );
        m_world->Add( lnew );
        return true;
    }

    return false;
}


const TOPOLOGY::JOINT_SET TOPOLOGY::ConnectedJoints( JOINT* aStart )
{
    std::deque<JOINT*> searchQueue;
    JOINT_SET processed;

    searchQueue.push_back( aStart );
    processed.insert( aStart );

    while( !searchQueue.empty() )
    {
        JOINT* current = searchQueue.front();
        searchQueue.pop_front();

        for( ITEM* item : current->LinkList() )
        {
            if( item->OfKind( ITEM::SEGMENT_T ) )
            {
                SEGMENT* seg = static_cast<SEGMENT*>( item );
                JOINT* a = m_world->FindJoint( seg->Seg().A, seg );
                JOINT* b = m_world->FindJoint( seg->Seg().B, seg );
                JOINT* next = ( *a == *current ) ? b : a;

                if( processed.find( next ) == processed.end() )
                {
                    processed.insert( next );
                    searchQueue.push_back( next );
                }
            }
        }
    }

    return processed;
}


bool TOPOLOGY::LeadingRatLine( const LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine )
{
    LINE track( *aTrack );
    VECTOR2I end;

    if( !track.PointCount() )
        return false;

    std::unique_ptr<NODE> tmpNode( m_world->Branch() );
    tmpNode->Add( track );

    JOINT* jt = tmpNode->FindJoint( track.CPoint( -1 ), &track );

    if( !jt )
       return false;

    if( ( !track.EndsWithVia() && jt->LinkCount() >= 2 ) || ( track.EndsWithVia() && jt->LinkCount() >= 3 ) ) // we got something connected
    {
        end = jt->Pos();
    }
    else
    {
        int anchor;

        TOPOLOGY topo( tmpNode.get() );
        ITEM* it = topo.NearestUnconnectedItem( jt, &anchor );

        if( !it )
            return false;

        end = it->Anchor( anchor );
    }

    aRatLine.Clear();
    aRatLine.Append( track.CPoint( -1 ) );
    aRatLine.Append( end );
    return true;
}


ITEM* TOPOLOGY::NearestUnconnectedItem( JOINT* aStart, int* aAnchor, int aKindMask )
{
    std::set<ITEM*> disconnected;

    m_world->AllItemsInNet( aStart->Net(), disconnected );

    for( const JOINT* jt : ConnectedJoints( aStart ) )
    {
        for( ITEM* link : jt->LinkList() )
        {
            if( disconnected.find( link ) != disconnected.end() )
                disconnected.erase( link );
        }
    }

    int best_dist = INT_MAX;
    ITEM* best = NULL;

    for( ITEM* item : disconnected )
    {
        if( item->OfKind( aKindMask ) )
        {
            for( int i = 0; i < item->AnchorCount(); i++ )
            {
                VECTOR2I p = item->Anchor( i );
                int d = ( p - aStart->Pos() ).EuclideanNorm();

                if( d < best_dist )
                {
                    best_dist = d;
                    best = item;

                    if( aAnchor )
                        *aAnchor = i;
                }
            }
        }
    }

    return best;
}


bool TOPOLOGY::followTrivialPath( LINE* aLine, bool aLeft, ITEM_SET& aSet, std::set<ITEM*>& aVisited )
{
    assert( aLine->IsLinked() );

    VECTOR2I anchor = aLeft ? aLine->CPoint( 0 )              : aLine->CPoint( -1 );
    SEGMENT* last   = aLeft ? aLine->LinkedSegments().front() : aLine->LinkedSegments().back();
    JOINT* jt = m_world->FindJoint( anchor, aLine );

    assert( jt != NULL );

    aVisited.insert( last );

    if( jt->IsNonFanoutVia() || jt->IsTraceWidthChange() )
    {
        ITEM* via = NULL;
        SEGMENT* next_seg = NULL;

        for( ITEM* link : jt->Links().Items() )
        {
            if( link->OfKind( ITEM::VIA_T ) )
                via = link;
            else if( aVisited.find( link ) == aVisited.end() )
                next_seg = static_cast<SEGMENT*>( link );
        }

        if( !next_seg )
            return false;

        LINE l = m_world->AssembleLine( next_seg );

        VECTOR2I nextAnchor = ( aLeft ? l.CLine().CPoint( -1 ) : l.CLine().CPoint( 0 ) );

        if( nextAnchor != anchor )
        {
            l.Reverse();
        }

        if( aLeft )
        {
            if( via )
                aSet.Prepend( via );

            aSet.Prepend( l );
        }
        else
        {
            if( via )
                aSet.Add( via );

            aSet.Add( l );
        }

        return followTrivialPath( &l, aLeft, aSet, aVisited );
    }

    return false;
}


const ITEM_SET TOPOLOGY::AssembleTrivialPath( ITEM* aStart )
{
    ITEM_SET path;
    std::set<ITEM*> visited;
    SEGMENT* seg;
    VIA* via;

    seg = dynamic_cast<SEGMENT*> (aStart);

    if(!seg && (via = dynamic_cast<VIA*>( aStart ) ) )
    {
        JOINT *jt = m_world->FindJoint( via->Pos(), via );

        if( !jt->IsNonFanoutVia() )
            return ITEM_SET();

        for( auto entry : jt->Links().Items() )
            if( ( seg = dynamic_cast<SEGMENT*>( entry.item ) ) )
                break;
    }

    if( !seg )
        return ITEM_SET();

    LINE l = m_world->AssembleLine( seg );

    path.Add( l );

    followTrivialPath( &l, false, path, visited );
    followTrivialPath( &l, true, path, visited );

    return path;
}


const ITEM_SET TOPOLOGY::ConnectedItems( JOINT* aStart, int aKindMask )
{
    return ITEM_SET();
}


const ITEM_SET TOPOLOGY::ConnectedItems( ITEM* aStart, int aKindMask )
{
    return ITEM_SET();
}


bool commonParallelProjection( SEG n, SEG p, SEG &pClip, SEG& nClip );


bool TOPOLOGY::AssembleDiffPair( ITEM* aStart, DIFF_PAIR& aPair )
{
    int refNet = aStart->Net();
    int coupledNet = m_world->GetRuleResolver()->DpCoupledNet( refNet );

    if( coupledNet < 0 )
        return false;

    std::set<ITEM*> coupledItems;

    m_world->AllItemsInNet( coupledNet, coupledItems );

    SEGMENT* coupledSeg = NULL, *refSeg;
    int minDist = std::numeric_limits<int>::max();

    if( ( refSeg = dynamic_cast<SEGMENT*>( aStart ) ) != NULL )
    {
        for( ITEM* item : coupledItems )
        {
            if( SEGMENT* s = dynamic_cast<SEGMENT*>( item ) )
            {
                if( s->Layers().Start() == refSeg->Layers().Start() && s->Width() == refSeg->Width() )
                {
                    int dist = s->Seg().Distance( refSeg->Seg() );
                    bool isParallel = refSeg->Seg().ApproxParallel( s->Seg() );
                    SEG p_clip, n_clip;

                    bool isCoupled = commonParallelProjection( refSeg->Seg(), s->Seg(), p_clip, n_clip );

                    if( isParallel && isCoupled && dist < minDist )
                    {
                        minDist = dist;
                        coupledSeg = s;
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }

    if( !coupledSeg )
        return false;

    LINE lp = m_world->AssembleLine( refSeg );
    LINE ln = m_world->AssembleLine( coupledSeg );

    if( m_world->GetRuleResolver()->DpNetPolarity( refNet ) < 0 )
    {
        std::swap( lp, ln );
    }

    int gap = -1;

    if( refSeg->Seg().ApproxParallel( coupledSeg->Seg() ) )
    {
        // Segments are parallel -> compute pair gap
        const VECTOR2I refDir       = refSeg->Anchor( 1 ) - refSeg->Anchor( 0 );
        const VECTOR2I displacement = refSeg->Anchor( 1 ) - coupledSeg->Anchor( 1 );
        gap = (int) std::abs( refDir.Cross( displacement ) / refDir.EuclideanNorm() ) - lp.Width();
    }

    aPair = DIFF_PAIR( lp, ln );
    aPair.SetWidth( lp.Width() );
    aPair.SetLayers( lp.Layers() );
    aPair.SetGap( gap );

    return true;
}

const std::set<ITEM*> TOPOLOGY::AssembleCluster( ITEM* aStart, int aLayer )
{
    std::set<ITEM*> visited;
    std::deque<ITEM*> pending;

    pending.push_back( aStart );

    while( !pending.empty() )
    {
        NODE::OBSTACLES obstacles;
        ITEM* top = pending.front();

        pending.pop_front();

        visited.insert( top );

        m_world->QueryColliding( top, obstacles, ITEM::ANY_T, -1, false );

        for( OBSTACLE& obs : obstacles )
        {
            if( visited.find( obs.m_item ) == visited.end() && obs.m_item->Layers().Overlaps( aLayer ) && !( obs.m_item->Marker() & MK_HEAD ) )
            {
                visited.insert( obs.m_item );
                pending.push_back( obs.m_item );
            }
        }
    }

    return visited;
}

}

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

/*#include <class_board.h>
#include <class_board_item.h>
#include <class_netinfo.h>*/

#include "pns_node.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_diff_pair_placer.h"
#include "pns_solid.h"
#include "pns_topology.h"
#include "pns_debug_decorator.h"

namespace PNS {

DIFF_PAIR_PLACER::DIFF_PAIR_PLACER( ROUTER* aRouter ) :
    PLACEMENT_ALGO( aRouter )
{
    m_state = RT_START;
    m_chainedPlacement = false;
    m_initialDiagonal = false;
    m_startDiagonal = false;
    m_fitOk = false;
    m_netP = 0;
    m_netN = 0;
    m_iteration = 0;
    m_world = NULL;
    m_shove = NULL;
    m_currentNode = NULL;
    m_lastNode = NULL;
    m_placingVia = false;
    m_viaDiameter = 0;
    m_viaDrill = 0;
    m_currentWidth = 0;
    m_currentNet = 0;
    m_currentLayer = 0;
    m_startsOnVia = false;
    m_orthoMode = false;
    m_snapOnTarget = false;
    m_currentEndItem = NULL;
    m_currentMode = RM_MarkObstacles;
    m_idle = true;
}

DIFF_PAIR_PLACER::~DIFF_PAIR_PLACER()
{
    if( m_shove )
        delete m_shove;
}


void DIFF_PAIR_PLACER::setWorld( NODE* aWorld )
{
    m_world = aWorld;
}

const VIA DIFF_PAIR_PLACER::makeVia( const VECTOR2I& aP, int aNet )
{
    const LAYER_RANGE layers( m_sizes.GetLayerTop(), m_sizes.GetLayerBottom() );

    VIA v( aP, layers, m_sizes.ViaDiameter(), m_sizes.ViaDrill(), -1, m_sizes.ViaType() );
    v.SetNet( aNet );

    return v;
}


void DIFF_PAIR_PLACER::SetOrthoMode ( bool aOrthoMode )
{
    m_orthoMode = aOrthoMode;

    if( !m_idle )
        Move( m_currentEnd, NULL );
}


bool DIFF_PAIR_PLACER::ToggleVia( bool aEnabled )
{
    m_placingVia = aEnabled;

    if( !m_idle )
        Move( m_currentEnd, NULL );

    return true;
}


bool DIFF_PAIR_PLACER::rhMarkObstacles( const VECTOR2I& aP )
{
    if( !routeHead( aP ) )
        return false;

    bool collP = static_cast<bool>( m_currentNode->CheckColliding( &m_currentTrace.PLine() ) );
    bool collN = static_cast<bool>( m_currentNode->CheckColliding( &m_currentTrace.NLine() ) );

    m_fitOk = !( collP || collN ) ;

    return m_fitOk;
}


bool DIFF_PAIR_PLACER::propagateDpHeadForces ( const VECTOR2I& aP, VECTOR2I& aNewP )
{
    VIA virtHead = makeVia( aP, -1 );

    if( m_placingVia )
        virtHead.SetDiameter( viaGap() + 2 * virtHead.Diameter() );
    else
    {
        virtHead.SetLayer( m_currentLayer );
        virtHead.SetDiameter( m_sizes.DiffPairGap() + 2 * m_sizes.TrackWidth() );
    }

    VECTOR2I lead( 0, 0 );// = aP - m_currentStart ;
    VECTOR2I force;
    bool solidsOnly = true;

    if( m_currentMode == RM_MarkObstacles )
    {
        aNewP = aP;
        return true;
    }
    else if( m_currentMode == RM_Walkaround )
    {
        solidsOnly = false;
    }

    // fixme: I'm too lazy to do it well. Circular approximaton will do for the moment.
    if( virtHead.PushoutForce( m_currentNode, lead, force, solidsOnly, 40 ) )
    {
        aNewP = aP + force;
        return true;
    }

    return false;
}


bool DIFF_PAIR_PLACER::attemptWalk( NODE* aNode, DIFF_PAIR* aCurrent,
        DIFF_PAIR& aWalk, bool aPFirst, bool aWindCw, bool aSolidsOnly )
{
    WALKAROUND walkaround( aNode, Router() );
    WALKAROUND::WALKAROUND_STATUS wf1;

    walkaround.SetSolidsOnly( aSolidsOnly );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );

    SHOVE shove( aNode, Router() );
    LINE walkP, walkN;

    aWalk = *aCurrent;

    int iter = 0;

    DIFF_PAIR cur( *aCurrent );

    bool currentIsP = aPFirst;

    int mask = aSolidsOnly ? ITEM::SOLID_T : ITEM::ANY_T;

    do
    {
        LINE preWalk = ( currentIsP ? cur.PLine() : cur.NLine() );
        LINE preShove = ( currentIsP ? cur.NLine() : cur.PLine() );
        LINE postWalk;

        if( !aNode->CheckColliding ( &preWalk, mask ) )
        {
            currentIsP = !currentIsP;

            if( !aNode->CheckColliding( &preShove, mask ) )
                break;
            else
                continue;
        }

        wf1 = walkaround.Route( preWalk, postWalk, false );

        if( wf1 != WALKAROUND::DONE )
            return false;

        LINE postShove( preShove );

        shove.ForceClearance( true, cur.Gap() - 2 * PNS_HULL_MARGIN );

        SHOVE::SHOVE_STATUS sh1;

        sh1 = shove.ProcessSingleLine( postWalk, preShove, postShove );

        if( sh1 != SHOVE::SH_OK )
            return false;

        postWalk.Line().Simplify();
        postShove.Line().Simplify();

        cur.SetShape( postWalk.CLine(), postShove.CLine(), !currentIsP );

        currentIsP = !currentIsP;

        if( !aNode->CheckColliding( &postShove, mask ) )
            break;

        iter++;
    }
    while( iter < 3 );

    if( iter == 3 )
        return false;

    aWalk.SetShape( cur.CP(), cur.CN() );

    return true;
}


bool DIFF_PAIR_PLACER::tryWalkDp( NODE* aNode, DIFF_PAIR &aPair, bool aSolidsOnly )
{
    DIFF_PAIR best;
    double bestScore = 100000000000000.0;

    for( int attempt = 0; attempt <= 3; attempt++ )
    {
        DIFF_PAIR p;
        NODE *tmp = m_currentNode->Branch();

        bool pfirst = ( attempt & 1 ) ? true : false;
        bool wind_cw = ( attempt & 2 ) ? true : false;

        if( attemptWalk( tmp, &aPair, p, pfirst, wind_cw, aSolidsOnly ) )
        {
        //    double len = p.TotalLength();
            double cl = p.CoupledLength();
            double skew = p.Skew();

            double score = cl + fabs( skew ) * 3.0;

            if( score < bestScore )
            {
                bestScore = score;
                best = p;
            }
        }

        delete tmp;
    }

    if( bestScore > 0.0 )
    {
        OPTIMIZER optimizer( m_currentNode );

        aPair.SetShape( best );
        optimizer.Optimize( &aPair );

        return true;
    }

    return false;
}


bool DIFF_PAIR_PLACER::rhWalkOnly( const VECTOR2I& aP )
{
    if( !routeHead ( aP ) )
        return false;

    m_fitOk = tryWalkDp( m_currentNode, m_currentTrace, false );

    return m_fitOk;
}


bool DIFF_PAIR_PLACER::route( const VECTOR2I& aP )
{
    switch( m_currentMode )
    {
    case RM_MarkObstacles:
        return rhMarkObstacles( aP );
    case RM_Walkaround:
        return rhWalkOnly( aP );
    case RM_Shove:
        return rhShoveOnly( aP );
    default:
        break;
    }

    return false;
}


bool DIFF_PAIR_PLACER::rhShoveOnly( const VECTOR2I& aP )
{
    m_currentNode = m_shove->CurrentNode();

    bool ok = routeHead( aP );

    m_fitOk  = false;

    if( !ok )
        return false;

    if( !tryWalkDp( m_currentNode, m_currentTrace, true ) )
        return false;

    LINE pLine( m_currentTrace.PLine() );
    LINE nLine( m_currentTrace.NLine() );
    ITEM_SET head;

    head.Add( &pLine );
    head.Add( &nLine );

    SHOVE::SHOVE_STATUS status = m_shove->ShoveMultiLines( head );

    m_currentNode = m_shove->CurrentNode();

    if( status == SHOVE::SH_OK )
    {
        m_currentNode = m_shove->CurrentNode();

        if( !m_currentNode->CheckColliding( &m_currentTrace.PLine() ) &&
            !m_currentNode->CheckColliding( &m_currentTrace.NLine() ) )
        {
            m_fitOk = true;
        }
    }

    return m_fitOk;
}


const ITEM_SET DIFF_PAIR_PLACER::Traces()
{
      ITEM_SET t;

      t.Add( const_cast<LINE*>( &m_currentTrace.PLine() ) );
      t.Add( const_cast<LINE*>( &m_currentTrace.NLine() ) );

      return t;
}


void DIFF_PAIR_PLACER::FlipPosture()
{
    m_startDiagonal = !m_startDiagonal;

    if( !m_idle )
        Move( m_currentEnd, NULL );
}


NODE* DIFF_PAIR_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( m_lastNode )
        return m_lastNode;

    return m_currentNode;
}


bool DIFF_PAIR_PLACER::SetLayer( int aLayer )
{
    if( m_idle )
    {
        m_currentLayer = aLayer;
        return true;
    }
    else if( m_chainedPlacement || !m_prevPair )
    {
        return false;
    }
    else if( !m_prevPair->PrimP() || ( m_prevPair->PrimP()->OfKind( ITEM::VIA_T ) &&
                m_prevPair->PrimP()->Layers().Overlaps( aLayer ) ) )
    {
        m_currentLayer = aLayer;
        m_start = *m_prevPair;
        initPlacement();
        Move( m_currentEnd, NULL );
        return true;
    }

    return false;
}


OPT_VECTOR2I DIFF_PAIR_PLACER::getDanglingAnchor( NODE* aNode, ITEM* aItem )
{
    switch( aItem->Kind() )
    {
    case ITEM::VIA_T:
    case ITEM::SOLID_T:
        return aItem->Anchor( 0 );

    case ITEM::SEGMENT_T:
    {
        SEGMENT* s =static_cast<SEGMENT*>( aItem );

        JOINT* jA = aNode->FindJoint( s->Seg().A, s );
        JOINT* jB = aNode->FindJoint( s->Seg().B, s );

        if( jA->LinkCount() == 1 )
            return s->Seg().A;
        else if( jB->LinkCount() == 1 )
            return s->Seg().B;
        else
            return OPT_VECTOR2I();
    }

    default:
        return OPT_VECTOR2I();
        break;
    }
}



bool DIFF_PAIR_PLACER::findDpPrimitivePair( const VECTOR2I& aP, ITEM* aItem,
                                            DP_PRIMITIVE_PAIR& aPair, std::string* aErrorMsg )
{
    int netP, netN;

    wxLogTrace( "PNS", "world %p", m_world );

    bool result = m_world->GetRuleResolver()->DpNetPair( aItem, netP, netN );

    if( !result )
        return false;

    int refNet = aItem->Net();
    int coupledNet = ( refNet == netP ) ? netN : netP;

    wxLogTrace( "PNS", "result %d", !!result );

    OPT_VECTOR2I refAnchor = getDanglingAnchor( m_currentNode, aItem );
    ITEM* primRef = aItem;

    wxLogTrace( "PNS", "refAnchor %p", aItem );

    if( !refAnchor )
        return false;

    std::set<ITEM*> coupledItems;

    m_currentNode->AllItemsInNet( coupledNet, coupledItems );
    double bestDist = std::numeric_limits<double>::max();
    bool found = false;

    for( ITEM* item : coupledItems )
    {
        if( item->Kind() == aItem->Kind() )
        {
            OPT_VECTOR2I anchor = getDanglingAnchor( m_currentNode, item );
            if( !anchor )
                continue;

            double dist = ( *anchor - *refAnchor ).EuclideanNorm();

            bool shapeMatches = true;

            if( item->OfKind( ITEM::SOLID_T ) && item->Layers() != aItem->Layers() )
            {
                shapeMatches = false;
            }

            if( dist < bestDist && shapeMatches )
            {
                found = true;
                bestDist = dist;

                if( refNet == netP )
                {
                    aPair = DP_PRIMITIVE_PAIR ( item, primRef );
                    aPair.SetAnchors( *anchor, *refAnchor );
                }
                else
                {
                    aPair = DP_PRIMITIVE_PAIR( primRef, item );
                    aPair.SetAnchors( *refAnchor, *anchor );
                }
            }
        }
    }

    return found;
}


int DIFF_PAIR_PLACER::viaGap() const
{
    return m_sizes.DiffPairViaGap();
}


int DIFF_PAIR_PLACER::gap() const
{
    return m_sizes.DiffPairGap() + m_sizes.DiffPairWidth();
}


bool DIFF_PAIR_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    VECTOR2I p( aP );

    if( !aStartItem )
    {
        Router()->SetFailureReason( ( "Can't start a differential pair "
                                       " in the middle of nowhere." ) );
        return false;
    }

    setWorld( Router()->GetWorld() );
    m_currentNode = m_world;

    if( !findDpPrimitivePair( aP, aStartItem, m_start ) )
    {
        Router()->SetFailureReason( ( "Unable to find complementary differential pair "
                                       "net. Make sure the names of the nets belonging "
                                       "to a differential pair end with either _N/_P or +/-." ) );
        return false;
    }

    m_netP = m_start.PrimP()->Net();
    m_netN = m_start.PrimN()->Net();

    #if 0
    // FIXME: this also needs to be factored out but not so important right now
    // Check if the current track/via gap & track width settings are violated
    BOARD* brd = NULL; // FIXME Router()->GetBoard();
    NETCLASSPTR netclassP = brd->FindNet( m_netP )->GetNetClass();
    NETCLASSPTR netclassN = brd->FindNet( m_netN )->GetNetClass();
    int clearance = std::min( m_sizes.DiffPairGap(), m_sizes.DiffPairViaGap() );

    if( clearance < netclassP->GetClearance() || clearance < netclassN->GetClearance() )
    {
        Router()->SetFailureReason( _( "Current track/via gap setting violates "
                                       "design rules for this net." ) );
        return false;
    }

    if( m_sizes.DiffPairWidth() < brd->GetDesignSettings().m_TrackMinWidth )
    {
        Router()->SetFailureReason( _( "Current track width setting violates design rules." ) );
        return false;
    }
    #endif

    m_currentStart = p;
    m_currentEnd = p;
    m_placingVia = false;
    m_chainedPlacement = false;

    initPlacement();

    return true;
}


void DIFF_PAIR_PLACER::initPlacement()
{
    m_idle = false;
    m_orthoMode = false;
    m_currentEndItem = NULL;
    m_startDiagonal = m_initialDiagonal;

    NODE* world = Router()->GetWorld();

    world->KillChildren();
    NODE* rootNode = world->Branch();

    setWorld( rootNode );

    m_lastNode = NULL;
    m_currentNode = rootNode;
    m_currentMode = Settings().Mode();

    if( m_shove )
        delete m_shove;

    m_shove = NULL;

    if( m_currentMode == RM_Shove || m_currentMode == RM_Smart )
    {
        m_shove = new SHOVE( m_currentNode, Router() );
    }
}


bool DIFF_PAIR_PLACER::routeHead( const VECTOR2I& aP )
{
    m_fitOk = false;

    DP_GATEWAYS gwsEntry( gap() );
    DP_GATEWAYS gwsTarget( gap() );

    if( !m_prevPair )
        m_prevPair = m_start;

    gwsEntry.BuildFromPrimitivePair( *m_prevPair, m_startDiagonal );

    DP_PRIMITIVE_PAIR target;

    if( findDpPrimitivePair( aP, m_currentEndItem, target ) )
    {
        gwsTarget.BuildFromPrimitivePair( target, m_startDiagonal );
        m_snapOnTarget = true;
    }
    else
    {
        VECTOR2I fp;

        if( !propagateDpHeadForces( aP, fp ) )
            return false;

        VECTOR2I midp, dirV;
        m_prevPair->CursorOrientation( fp, midp, dirV );

        VECTOR2I fpProj = SEG( midp, midp + dirV ).LineProject( fp );
        int lead_dist = ( fpProj - fp ).EuclideanNorm();

        gwsTarget.SetFitVias( m_placingVia, m_sizes.ViaDiameter(), viaGap() );

        if( lead_dist > m_sizes.DiffPairGap() + m_sizes.DiffPairWidth() )
        {
            gwsTarget.BuildForCursor( fp );
        }
        else
        {
            gwsTarget.BuildForCursor( fpProj );
            gwsTarget.FilterByOrientation( DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_HALF_FULL, DIRECTION_45( dirV ) );
        }

        m_snapOnTarget = false;
    }

    m_currentTrace = DIFF_PAIR();
    m_currentTrace.SetGap( gap() );
    m_currentTrace.SetLayer( m_currentLayer );

    bool result = gwsEntry.FitGateways( gwsEntry, gwsTarget, m_startDiagonal, m_currentTrace );

    if( result )
    {
        m_currentTrace.SetNets( m_netP, m_netN );
        m_currentTrace.SetWidth( m_sizes.DiffPairWidth() );
        m_currentTrace.SetGap( m_sizes.DiffPairGap() );

        if( m_placingVia )
        {
            m_currentTrace.AppendVias ( makeVia( m_currentTrace.CP().CPoint( -1 ), m_netP ),
                                        makeVia( m_currentTrace.CN().CPoint( -1 ), m_netN ) );
        }

        return true;
    }

    return false;
}


bool DIFF_PAIR_PLACER::Move( const VECTOR2I& aP , ITEM* aEndItem )
{
    m_currentEndItem = aEndItem;
    m_fitOk = false;

    delete m_lastNode;
    m_lastNode = NULL;

    if( !route( aP ) )
        return false;

    NODE* latestNode = m_currentNode;
    m_lastNode = latestNode->Branch();

    assert( m_lastNode != NULL );
    m_currentEnd = aP;

    updateLeadingRatLine();

    return true;
}


void DIFF_PAIR_PLACER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    if( !m_idle )
    {
        initPlacement();
        Move( m_currentEnd, NULL );
    }
}


bool DIFF_PAIR_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    if( !m_fitOk )
        return false;

    if( m_currentTrace.CP().SegmentCount() < 1 ||
            m_currentTrace.CN().SegmentCount() < 1 )
        return false;

    if( m_currentTrace.CP().SegmentCount() > 1 )
        m_initialDiagonal = !DIRECTION_45( m_currentTrace.CP().CSegment( -2 ) ).IsDiagonal();

    TOPOLOGY topo( m_lastNode );

    if( !m_snapOnTarget && !m_currentTrace.EndsWithVias() && !aForceFinish )
    {
        SHAPE_LINE_CHAIN newP( m_currentTrace.CP() );
        SHAPE_LINE_CHAIN newN( m_currentTrace.CN() );

        if( newP.SegmentCount() > 1 && newN.SegmentCount() > 1 )
        {
            newP.Remove( -1, -1 );
            newN.Remove( -1, -1 );
        }

        m_currentTrace.SetShape( newP, newN );
    }

    if( m_currentTrace.EndsWithVias() )
    {
        m_lastNode->Add( Clone( m_currentTrace.PLine().Via() ) );
        m_lastNode->Add( Clone( m_currentTrace.NLine().Via() ) );
        m_chainedPlacement = false;
    }
    else
    {
        m_chainedPlacement = !m_snapOnTarget && !aForceFinish;
    }

    LINE lineP( m_currentTrace.PLine() );
    LINE lineN( m_currentTrace.NLine() );

    m_lastNode->Add( lineP );
    m_lastNode->Add( lineN );

    topo.SimplifyLine( &lineP );
    topo.SimplifyLine( &lineN );

    m_prevPair = m_currentTrace.EndingPrimitives();

    Router()->CommitRouting( m_lastNode );

    m_lastNode = NULL;
    m_placingVia = false;

    if( m_snapOnTarget || aForceFinish )
    {
        m_idle = true;
        return true;
    }
    else
    {
        initPlacement();
        return false;
    }
}


void DIFF_PAIR_PLACER::GetModifiedNets( std::vector<int> &aNets ) const
{
    aNets.push_back( m_netP );
    aNets.push_back( m_netN );
}


void DIFF_PAIR_PLACER::updateLeadingRatLine()
{
    SHAPE_LINE_CHAIN ratLineN, ratLineP;
    TOPOLOGY topo( m_lastNode );

    if( topo.LeadingRatLine( &m_currentTrace.PLine(), ratLineP ) )
    {
        Dbg()->AddLine( ratLineP, 1, 10000 );
    }

    if( topo.LeadingRatLine ( &m_currentTrace.NLine(), ratLineN ) )
    {
        Dbg()->AddLine( ratLineN, 3, 10000 );
    }
}


const std::vector<int> DIFF_PAIR_PLACER::CurrentNets() const
{
    std::vector<int> rv;
    rv.push_back( m_netP );
    rv.push_back( m_netN );
    return rv;
}

}

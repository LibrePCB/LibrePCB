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

#include <core/optional.h>

//#include <base_units.h> // God forgive me doing this...

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_dp_meander_placer.h"
#include "pns_diff_pair.h"
#include "pns_router.h"
#include "pns_utils.h"
//#include "util/util.hpp"

namespace PNS {

DP_MEANDER_PLACER::DP_MEANDER_PLACER( ROUTER* aRouter ) :
    MEANDER_PLACER_BASE( aRouter )
{
    m_world = NULL;
    m_currentNode = NULL;

    // Init temporary variables (do not leave uninitialized members)
    m_initialSegment = NULL;
    m_lastLength = 0;
    m_lastStatus = TOO_SHORT;
}


DP_MEANDER_PLACER::~DP_MEANDER_PLACER()
{
}


const LINE DP_MEANDER_PLACER::Trace() const
{
    return m_currentTraceP;
}


NODE* DP_MEANDER_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( !m_currentNode )
        return m_world;

    return m_currentNode;
}


bool DP_MEANDER_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    VECTOR2I p;

    if( !aStartItem || !aStartItem->OfKind( ITEM::SEGMENT_T ) )
    {
        Router()->SetFailureReason( ( "Please select a track whose length you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<SEGMENT*>( aStartItem );

    p = m_initialSegment->Seg().NearestPoint( aP );

    m_currentNode=NULL;
    m_currentStart = p;

    m_world = Router()->GetWorld()->Branch();

    TOPOLOGY topo( m_world );

    if( !topo.AssembleDiffPair( m_initialSegment, m_originPair ) )
    {
        Router()->SetFailureReason( ( "Unable to find complementary differential pair "
                                       "net for length tuning. Make sure the names of the nets belonging "
                                       "to a differential pair end with either _N/_P or +/-." ) );
        return false;
    }

    if( m_originPair.Gap() < 0 )
        m_originPair.SetGap( Router()->Sizes().DiffPairGap() );

    if( !m_originPair.PLine().SegmentCount() ||
        !m_originPair.NLine().SegmentCount() )
        return false;

    m_tunedPathP = topo.AssembleTrivialPath( m_originPair.PLine().GetLink( 0 ) );
    m_tunedPathN = topo.AssembleTrivialPath( m_originPair.NLine().GetLink( 0 ) );

    m_world->Remove( m_originPair.PLine() );
    m_world->Remove( m_originPair.NLine() );

    m_currentWidth = m_originPair.Width();

    return true;
}


void DP_MEANDER_PLACER::release()
{
}


int DP_MEANDER_PLACER::origPathLength() const
{
    int totalP = 0;
    int totalN = 0;

    for( const ITEM* item : m_tunedPathP.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            totalP += l->CLine().Length();

    }

    for( const ITEM* item : m_tunedPathN.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            totalN += l->CLine().Length();
    }

    return std::max( totalP, totalN );
}


const SEG DP_MEANDER_PLACER::baselineSegment( const DIFF_PAIR::COUPLED_SEGMENTS& aCoupledSegs )
{
    const VECTOR2I a( ( aCoupledSegs.coupledP.A + aCoupledSegs.coupledN.A ) / 2 );
    const VECTOR2I b( ( aCoupledSegs.coupledP.B + aCoupledSegs.coupledN.B ) / 2 );

    return SEG( a, b );
}


static bool pairOrientation( const DIFF_PAIR::COUPLED_SEGMENTS& aPair )
{
    VECTOR2I midp = ( aPair.coupledP.A + aPair.coupledN.A ) / 2;

    //DrawDebugPoint(midp, 6);

    return aPair.coupledP.Side( midp ) > 0;
}


bool DP_MEANDER_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
//    return false;

    DIFF_PAIR::COUPLED_SEGMENTS_VEC coupledSegments;

    if( m_currentNode )
        delete m_currentNode;

    m_currentNode = m_world->Branch();

    SHAPE_LINE_CHAIN preP, tunedP, postP;
    SHAPE_LINE_CHAIN preN, tunedN, postN;

    cutTunedLine( m_originPair.CP(), m_currentStart, aP, preP, tunedP, postP );
    cutTunedLine( m_originPair.CN(), m_currentStart, aP, preN, tunedN, postN );

    DIFF_PAIR tuned( m_originPair );

    tuned.SetShape( tunedP, tunedN );

    tuned.CoupledSegmentPairs( coupledSegments );

    if( coupledSegments.size() == 0 )
        return false;

    //Router()->DisplayDebugLine( tuned.CP(), 5, 20000 );
    //Router()->DisplayDebugLine( tuned.CN(), 4, 20000 );

    //Router()->DisplayDebugLine( m_originPair.CP(), 5, 20000 );
    //Router()->DisplayDebugLine( m_originPair.CN(), 4, 20000 );

    m_result = MEANDERED_LINE( this, true );
    m_result.SetWidth( tuned.Width() );

    int offset = ( tuned.Gap() + tuned.Width() ) / 2;

    if( !pairOrientation( coupledSegments[0] ) )
        offset *= -1;

    m_result.SetBaselineOffset( offset );

    for( const ITEM* item : m_tunedPathP.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            Dbg()->AddLine( l->CLine(), 5, 10000 );
    }

    for( const ITEM* item : m_tunedPathN.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            Dbg()->AddLine( l->CLine(), 5, 10000 );
    }

    int curIndexP = 0, curIndexN = 0;

    for( const DIFF_PAIR::COUPLED_SEGMENTS& sp : coupledSegments )
    {
        SEG base = baselineSegment( sp );

        Dbg()->AddSegment( base, 3 );

        while( sp.indexP >= curIndexP )
        {
            m_result.AddCorner( tunedP.CPoint( curIndexP ), tunedN.CPoint( curIndexN ) );
            curIndexP++;
        }

        while( sp.indexN >= curIndexN )
        {
            m_result.AddCorner( tunedP.CPoint( sp.indexP ), tunedN.CPoint( curIndexN ) );
            curIndexN++;
        }

        m_result.MeanderSegment( base );
    }

    while( curIndexP < tunedP.PointCount() )
        m_result.AddCorner( tunedP.CPoint( curIndexP++ ), tunedN.CPoint( curIndexN ) );

    while( curIndexN < tunedN.PointCount() )
        m_result.AddCorner( tunedP.CPoint( -1 ), tunedN.CPoint( curIndexN++ ) );

    int dpLen = origPathLength();

    m_lastStatus = TUNED;

    if( dpLen - m_settings.m_targetLength > m_settings.m_lengthTolerance )
    {
        m_lastStatus = TOO_LONG;
        m_lastLength = dpLen;
    }
    else
    {
        m_lastLength = dpLen - std::max( tunedP.Length(), tunedN.Length() );
        tuneLineLength( m_result, m_settings.m_targetLength - dpLen );
    }

    if( m_lastStatus != TOO_LONG )
    {
        tunedP.Clear();
        tunedN.Clear();

        for( MEANDER_SHAPE* m : m_result.Meanders() )
        {
            if( m->Type() != MT_EMPTY )
            {
                tunedP.Append( m->CLine( 0 ) );
                tunedN.Append( m->CLine( 1 ) );
            }
        }

        m_lastLength += std::max( tunedP.Length(), tunedN.Length() );

        int comp = compareWithTolerance( m_lastLength - m_settings.m_targetLength, 0, m_settings.m_lengthTolerance );

        if( comp > 0 )
            m_lastStatus = TOO_LONG;
        else if( comp < 0 )
            m_lastStatus = TOO_SHORT;
        else
            m_lastStatus = TUNED;
    }

    m_finalShapeP.Clear();
    m_finalShapeP.Append( preP );
    m_finalShapeP.Append( tunedP );
    m_finalShapeP.Append( postP );
    m_finalShapeP.Simplify();

    m_finalShapeN.Clear();
    m_finalShapeN.Append( preN );
    m_finalShapeN.Append( tunedN );
    m_finalShapeN.Append( postN );
    m_finalShapeN.Simplify();

    return true;
}


bool DP_MEANDER_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    LINE lP( m_originPair.PLine(), m_finalShapeP );
    LINE lN( m_originPair.NLine(), m_finalShapeN );

    m_currentNode->Add( lP );
    m_currentNode->Add( lN );

    Router()->CommitRouting( m_currentNode );

    return true;
}


bool DP_MEANDER_PLACER::CheckFit( MEANDER_SHAPE* aShape )
{
    LINE l1( m_originPair.PLine(), aShape->CLine( 0 ) );
    LINE l2( m_originPair.NLine(), aShape->CLine( 1 ) );

    if( m_currentNode->CheckColliding( &l1 ) )
        return false;

    if( m_currentNode->CheckColliding( &l2 ) )
        return false;

    int w = aShape->Width();
    int clearance = w + m_settings.m_spacing;

    return m_result.CheckSelfIntersections( aShape, clearance );
}


const ITEM_SET DP_MEANDER_PLACER::Traces()
{
    m_currentTraceP = LINE( m_originPair.PLine(), m_finalShapeP );
    m_currentTraceN = LINE( m_originPair.NLine(), m_finalShapeN );

    ITEM_SET traces;

    traces.Add( &m_currentTraceP );
    traces.Add( &m_currentTraceN );

    return traces;
}


const VECTOR2I& DP_MEANDER_PLACER::CurrentEnd() const
{
    return m_currentEnd;
}


int DP_MEANDER_PLACER::CurrentLayer() const
{
    return m_initialSegment->Layers().Start();
}


const std::string DP_MEANDER_PLACER::TuningInfo() const
{
    std::string status;

    switch( m_lastStatus )
    {
    case TOO_LONG:
        status = ( "Too long: " );
        break;
    case TOO_SHORT:
        status = ("Too short: " );
        break;
    case TUNED:
        status = ( "Tuned: " );
        break;
    default:
        return ( "?" );
    }

    //status += horizon::dim_to_string(m_lastLength, false);
    //status += "/";
    //status += horizon::dim_to_string(m_settings.m_targetLength, false);
    //status += " (gap: ";
    //status += horizon::dim_to_string(m_originPair.Gap(), false);
    //status += ")";

    return status;
}


DP_MEANDER_PLACER::TUNING_STATUS DP_MEANDER_PLACER::TuningStatus() const
{
    return m_lastStatus;
}

const std::vector<int> DP_MEANDER_PLACER::CurrentNets() const
{
    std::vector<int> rv;
    rv.push_back( m_originPair.NetP() );
    rv.push_back( m_originPair.NetN() );
    return rv;
}

}

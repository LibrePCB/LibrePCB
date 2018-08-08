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

//#include <base_units.h> // God forgive me doing this...

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_meander_placer.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
//#include "util/util.hpp"

namespace PNS {

MEANDER_PLACER::MEANDER_PLACER( ROUTER* aRouter ) :
    MEANDER_PLACER_BASE( aRouter )
{
    m_world = NULL;
    m_currentNode = NULL;

    // Init temporary variables (do not leave uninitialized members)
    m_initialSegment = NULL;
    m_lastLength = 0;
    m_lastStatus = TOO_SHORT;
}


MEANDER_PLACER::~MEANDER_PLACER()
{
}


NODE* MEANDER_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( !m_currentNode )
        return m_world;

    return m_currentNode;
}


bool MEANDER_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    VECTOR2I p;

    if( !aStartItem || !aStartItem->OfKind( ITEM::SEGMENT_T ) )
    {
        Router()->SetFailureReason( ( "Please select a track whose length you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<SEGMENT*>( aStartItem );

    p = m_initialSegment->Seg().NearestPoint( aP );

    m_currentNode = NULL;
    m_currentStart = p;

    m_world = Router()->GetWorld()->Branch();
    m_originLine = m_world->AssembleLine( m_initialSegment );

    TOPOLOGY topo( m_world );
    m_tunedPath = topo.AssembleTrivialPath( m_initialSegment );

    m_world->Remove( m_originLine );

    m_currentWidth = m_originLine.Width();
    m_currentEnd = VECTOR2I( 0, 0 );

    return true;
}


int MEANDER_PLACER::origPathLength() const
{
    int total = 0;
    for( const ITEM* item : m_tunedPath.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
        {
            total += l->CLine().Length();
        }
    }

    return total;
}


bool MEANDER_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    return doMove( aP, aEndItem, m_settings.m_targetLength );
}


bool MEANDER_PLACER::doMove( const VECTOR2I& aP, ITEM* aEndItem, int aTargetLength )
{
    SHAPE_LINE_CHAIN pre, tuned, post;

    if( m_currentNode )
        delete m_currentNode;

    m_currentNode = m_world->Branch();

    cutTunedLine( m_originLine.CLine(), m_currentStart, aP, pre, tuned, post );

    m_result = MEANDERED_LINE( this, false );
    m_result.SetWidth( m_originLine.Width() );
    m_result.SetBaselineOffset( 0 );

    for( int i = 0; i < tuned.SegmentCount(); i++ )
    {
        const SEG s = tuned.CSegment( i );
        m_result.AddCorner( s.A );
        m_result.MeanderSegment( s );
        m_result.AddCorner( s.B );
    }

    int lineLen = origPathLength();

    m_lastLength = lineLen;
    m_lastStatus = TUNED;

    if( compareWithTolerance( lineLen, aTargetLength, m_settings.m_lengthTolerance ) > 0 )
    {
        m_lastStatus = TOO_LONG;
    } else {
        m_lastLength = lineLen - tuned.Length();
        tuneLineLength( m_result, aTargetLength - lineLen );
    }

    for( const ITEM* item : m_tunedPath.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
        {
            Dbg()->AddLine( l->CLine(), 5, 30000 );
        }
    }

    if( m_lastStatus != TOO_LONG )
    {
        tuned.Clear();

        for( MEANDER_SHAPE* m : m_result.Meanders() )
        {
            if( m->Type() != MT_EMPTY )
            {
                tuned.Append ( m->CLine( 0 ) );
            }
        }

        m_lastLength += tuned.Length();

        int comp = compareWithTolerance( m_lastLength - aTargetLength, 0, m_settings.m_lengthTolerance );

        if( comp > 0 )
            m_lastStatus = TOO_LONG;
        else if( comp < 0 )
            m_lastStatus = TOO_SHORT;
        else
            m_lastStatus = TUNED;
    }

    m_finalShape.Clear();
    m_finalShape.Append( pre );
    m_finalShape.Append( tuned );
    m_finalShape.Append( post );
    m_finalShape.Simplify();

    return true;
}


bool MEANDER_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    if( !m_currentNode )
        return false;

    m_currentTrace = LINE( m_originLine, m_finalShape );
    m_currentNode->Add( m_currentTrace );

    Router()->CommitRouting( m_currentNode );
    return true;
}


bool MEANDER_PLACER::CheckFit( MEANDER_SHAPE* aShape )
{
    LINE l( m_originLine, aShape->CLine( 0 ) );

    if( m_currentNode->CheckColliding( &l ) )
        return false;

    int w = aShape->Width();
    int clearance = w + m_settings.m_spacing;

    return m_result.CheckSelfIntersections( aShape, clearance );
}


const ITEM_SET MEANDER_PLACER::Traces()
{
    m_currentTrace = LINE( m_originLine, m_finalShape );
    return ITEM_SET( &m_currentTrace );
}


const VECTOR2I& MEANDER_PLACER::CurrentEnd() const
{
    return m_currentEnd;
}

int MEANDER_PLACER::CurrentLayer() const
{
    return m_initialSegment->Layers().Start();
}


const std::string MEANDER_PLACER::TuningInfo() const
{
    std::string status;

    switch ( m_lastStatus )
    {
    case TOO_LONG:
        status = ( "Too long: " );
        break;
    case TOO_SHORT:
        status = ( "Too short: " );
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

    return status;
}


MEANDER_PLACER::TUNING_STATUS MEANDER_PLACER::TuningStatus() const
{
    return m_lastStatus;
}

}

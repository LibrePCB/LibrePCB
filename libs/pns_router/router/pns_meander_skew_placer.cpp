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
#include "pns_meander_skew_placer.h"

#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "util/util.hpp"

namespace PNS {

MEANDER_SKEW_PLACER::MEANDER_SKEW_PLACER ( ROUTER* aRouter ) :
    MEANDER_PLACER ( aRouter )
{
    // Init temporary variables (do not leave uninitialized members)
    m_coupledLength = 0;
}


MEANDER_SKEW_PLACER::~MEANDER_SKEW_PLACER( )
{
}


bool MEANDER_SKEW_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    VECTOR2I p;

    if( !aStartItem || !aStartItem->OfKind( ITEM::SEGMENT_T ) )
    {
        Router()->SetFailureReason( ( "Please select a differential pair trace you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<SEGMENT*>( aStartItem );

    p = m_initialSegment->Seg().NearestPoint( aP );

    m_currentNode = NULL;
    m_currentStart = p;

    m_world = Router()->GetWorld( )->Branch();
    m_originLine = m_world->AssembleLine( m_initialSegment );

    TOPOLOGY topo( m_world );
    m_tunedPath = topo.AssembleTrivialPath( m_initialSegment );

    if( !topo.AssembleDiffPair ( m_initialSegment, m_originPair ) )
    {
        Router()->SetFailureReason( ( "Unable to find complementary differential pair "
                                       "net for skew tuning. Make sure the names of the nets belonging "
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

    m_world->Remove( m_originLine );

    m_currentWidth = m_originLine.Width();
    m_currentEnd = VECTOR2I( 0, 0 );

    if ( m_originPair.PLine().Net() == m_originLine.Net() )
        m_coupledLength = itemsetLength( m_tunedPathN );
    else
        m_coupledLength = itemsetLength( m_tunedPathP );

    return true;
}


int MEANDER_SKEW_PLACER::origPathLength( ) const
{
    return itemsetLength ( m_tunedPath );
}


int MEANDER_SKEW_PLACER::itemsetLength( const ITEM_SET& aSet ) const
{
    int total = 0;
    for( const ITEM* item : aSet.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
        {
            total += l->CLine().Length();
        }
    }

    return total;
}


int MEANDER_SKEW_PLACER::currentSkew() const
{
    return m_lastLength - m_coupledLength;
}


bool MEANDER_SKEW_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    for( const ITEM* item : m_tunedPathP.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            Dbg()->AddLine( l->CLine(), 5, 10000 );
    }

    for( const ITEM* item : m_tunedPathN.CItems() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
            Dbg()->AddLine( l->CLine(), 4, 10000 );
    }

    return doMove( aP, aEndItem, m_coupledLength + m_settings.m_targetSkew );
}


const std::string MEANDER_SKEW_PLACER::TuningInfo() const
{
    std::string status;

    switch( m_lastStatus )
    {
    case TOO_LONG:
        status = ( "Too long: skew " );
        break;
    case TOO_SHORT:
        status = ( "Too short: skew " );
        break;
    case TUNED:
        status = ( "Tuned: skew " );
        break;
    default:
        return ( "?" );
    }

    status += horizon::dim_to_string(m_lastLength - m_coupledLength, false);
    status += "/";
    status += horizon::dim_to_string(m_settings.m_targetSkew, false);

    return status;
}

}

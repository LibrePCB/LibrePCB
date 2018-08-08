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

#include "pns_dragger.h"
#include "pns_shove.h"
#include "pns_router.h"

namespace PNS {

DRAGGER::DRAGGER( ROUTER* aRouter ) :
    ALGO_BASE( aRouter )
{
    m_world = NULL;
    m_lastNode = NULL;
    m_mode = DM_SEGMENT;
    m_draggedVia = NULL;
    m_shove = NULL;
    m_draggedSegmentIndex = 0;
    m_dragStatus = false;
    m_currentMode = RM_MarkObstacles;
    m_initialVia = NULL;
    m_freeAngleMode = false;
}


DRAGGER::~DRAGGER()
{
    if( m_shove )
        delete m_shove;
}


void DRAGGER::SetWorld( NODE* aWorld )
{
    m_world = aWorld;
}


bool DRAGGER::startDragSegment( const VECTOR2D& aP, SEGMENT* aSeg )
{
    int w2 = aSeg->Width() / 2;

    m_draggedLine = m_world->AssembleLine( aSeg, &m_draggedSegmentIndex );
    m_shove->SetInitialLine( m_draggedLine );
    m_lastValidDraggedLine = m_draggedLine;
    m_lastValidDraggedLine.ClearSegmentLinks();

    auto distA = ( aP - aSeg->Seg().A ).EuclideanNorm();
    auto distB = ( aP - aSeg->Seg().B ).EuclideanNorm();


    if( distA <= w2 )
    {
        m_mode = DM_CORNER;
    }
    else if( distB <= w2 )
    {
        m_draggedSegmentIndex++;
        m_mode = DM_CORNER;
    }
    else if ( m_freeAngleMode )
    {
        if( distB < distA )
        {
            m_draggedSegmentIndex++;
        }
        m_mode = DM_CORNER;
    }
    else
    {
        m_mode = DM_SEGMENT;
    }

    return true;
}


bool DRAGGER::startDragVia( const VECTOR2D& aP, VIA* aVia )
{
    m_draggedVia = aVia;
    m_initialVia = aVia;
    m_mode = DM_VIA;

    VECTOR2I p0( aVia->Pos() );
    JOINT* jt = m_world->FindJoint( p0, aVia->Layers().Start(), aVia->Net() );

    if( !jt )
        return false;

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T ) )
        {
            int segIndex;
            SEGMENT* seg = ( SEGMENT*) item;
            LINE l = m_world->AssembleLine( seg, &segIndex );

            if( segIndex != 0 )
                l.Reverse();

            m_origViaConnections.Add( l );
        }
    }

    return true;
}


bool DRAGGER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    m_shove = new SHOVE( m_world, Router() );
    m_lastNode = NULL;
    m_draggedItems.Clear();
    m_currentMode = Settings().Mode();
    m_freeAngleMode = (m_mode & DM_FREE_ANGLE);

    aStartItem->Unmark( MK_LOCKED );

    wxLogTrace( "PNS", "StartDragging: item %p [kind %d]", aStartItem, (int) aStartItem->Kind() );

    switch( aStartItem->Kind() )
    {
    case ITEM::SEGMENT_T:
        return startDragSegment( aP, static_cast<SEGMENT*>( aStartItem ) );

    case ITEM::VIA_T:
        return startDragVia( aP, static_cast<VIA*>( aStartItem ) );

    default:
        return false;
    }
}


void DRAGGER::SetMode( int aMode )
{
    m_mode = aMode;
}


bool DRAGGER::dragMarkObstacles( const VECTOR2I& aP )
{
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = NULL;
    }

    switch( m_mode )
    {
    case DM_SEGMENT:
    case DM_CORNER:
    {
        int thresh = Settings().SmoothDraggedSegments() ? m_draggedLine.Width() / 4 : 0;
        LINE dragged( m_draggedLine );

        if( m_mode == DM_SEGMENT )
            dragged.DragSegment( aP, m_draggedSegmentIndex, thresh );
        else
            dragged.DragCorner( aP, m_draggedSegmentIndex, thresh, m_freeAngleMode );

        m_lastNode = m_shove->CurrentNode()->Branch();

        m_lastValidDraggedLine = dragged;
        m_lastValidDraggedLine.ClearSegmentLinks();
        m_lastValidDraggedLine.Unmark();

        m_lastNode->Add( m_lastValidDraggedLine );
        m_draggedItems.Clear();
        m_draggedItems.Add( m_lastValidDraggedLine );

        break;
    }

    case DM_VIA: // fixme...
    {
        m_lastNode = m_shove->CurrentNode()->Branch();
        dumbDragVia( m_initialVia, m_lastNode, aP );

        break;
    }
    }

    if( Settings().CanViolateDRC() )
        m_dragStatus = true;
    else
        m_dragStatus = !m_world->CheckColliding( m_draggedItems );

    return true;
}


void DRAGGER::dumbDragVia( VIA* aVia, NODE* aNode, const VECTOR2I& aP )
{
    m_draggedItems.Clear();

    // fixme: this is awful.
    auto via_clone = Clone( *aVia );

    m_draggedVia = via_clone.get();
    m_draggedVia->SetPos( aP );

    m_draggedItems.Add( m_draggedVia );

    m_lastNode->Remove( aVia );
    m_lastNode->Add( std::move( via_clone ) );

    for( ITEM* item : m_origViaConnections.Items() )
    {
        if( const LINE* l = dynamic_cast<const LINE*>( item ) )
        {
            LINE origLine( *l );
            LINE draggedLine( *l );

            draggedLine.DragCorner( aP, origLine.CLine().Find( aVia->Pos() ), 0, m_freeAngleMode );
            draggedLine.ClearSegmentLinks();

            m_draggedItems.Add( draggedLine );

            m_lastNode->Remove( origLine );
            m_lastNode->Add( draggedLine );
        }
    }
}


bool DRAGGER::dragShove( const VECTOR2I& aP )
{
    bool ok = false;

    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = NULL;
    }

    switch( m_mode )
    {
    case DM_SEGMENT:
    case DM_CORNER:
    {
        int thresh = Settings().SmoothDraggedSegments() ? m_draggedLine.Width() / 4 : 0;
        LINE dragged( m_draggedLine );

        if( m_mode == DM_SEGMENT )
            dragged.DragSegment( aP, m_draggedSegmentIndex, thresh );
        else
            dragged.DragCorner( aP, m_draggedSegmentIndex, thresh );

        SHOVE::SHOVE_STATUS st = m_shove->ShoveLines( dragged );

        if( st == SHOVE::SH_OK )
            ok = true;
        else if( st == SHOVE::SH_HEAD_MODIFIED )
        {
            dragged = m_shove->NewHead();
            ok = true;
        }

        m_lastNode = m_shove->CurrentNode()->Branch();

        if( ok )
            m_lastValidDraggedLine = dragged;

        m_lastValidDraggedLine.ClearSegmentLinks();
        m_lastValidDraggedLine.Unmark();
        m_lastNode->Add( m_lastValidDraggedLine );
        m_draggedItems.Clear();
        m_draggedItems.Add( m_lastValidDraggedLine );

        break;
    }

    case DM_VIA:
    {
        VIA* newVia;
        SHOVE::SHOVE_STATUS st = m_shove->ShoveDraggingVia( m_draggedVia, aP, &newVia );

        if( st == SHOVE::SH_OK || st == SHOVE::SH_HEAD_MODIFIED )
            ok = true;

        m_lastNode = m_shove->CurrentNode()->Branch();

        if( ok )
        {
            if( newVia )
                m_draggedVia = newVia;

            m_draggedItems.Clear();
        }

        break;
    }
    }

    m_dragStatus = ok;

    return ok;
}


bool DRAGGER::FixRoute()
{
    NODE* node = CurrentNode();

    if( node )
    {
        Router()->CommitRouting( node );
        return true;
    }

    return false;
}


bool DRAGGER::Drag( const VECTOR2I& aP )
{
    if( m_freeAngleMode )
        return dragMarkObstacles( aP );

    switch( m_currentMode )
    {
    case RM_MarkObstacles:
        return dragMarkObstacles( aP );

    case RM_Shove:
    case RM_Walkaround:
    case RM_Smart:
        return dragShove( aP );

    default:
        return false;
    }
}


NODE* DRAGGER::CurrentNode() const
{
   return m_lastNode;
}


const ITEM_SET DRAGGER::Traces()
{
    return m_draggedItems;
}


LOGGER* DRAGGER::Logger()
{
    if( m_shove )
        return m_shove->Logger();

    return NULL;
}

}

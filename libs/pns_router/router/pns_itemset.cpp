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

#include "pns_itemset.h"
#include "pns_line.h"

namespace PNS {

ITEM_SET::~ITEM_SET()
{
}


void ITEM_SET::Add( const LINE& aLine )
{
    LINE* copy = aLine.Clone();
    m_items.push_back( ENTRY( copy, true ) );
}


void ITEM_SET::Prepend( const LINE& aLine )
{
    LINE* copy = aLine.Clone();
    m_items.insert( m_items.begin(), ENTRY( copy, true ) );
}


ITEM_SET& ITEM_SET::FilterLayers( int aStart, int aEnd, bool aInvert )
{
    ENTRIES newItems;
    LAYER_RANGE l;

    if( aEnd < 0 )
        l = LAYER_RANGE( aStart );
    else
        l = LAYER_RANGE( aStart, aEnd );

    for( const ENTRY& ent : m_items )
    {
        if( ent.item->Layers().Overlaps( l ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


ITEM_SET& ITEM_SET::FilterKinds( int aKindMask, bool aInvert )
{
    ENTRIES newItems;

    for( const ENTRY& ent : m_items )
    {
        if( ent.item->OfKind( aKindMask ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


ITEM_SET& ITEM_SET::FilterMarker( int aMarker, bool aInvert )
{
    ENTRIES newItems;

    for( const ENTRY& ent : m_items )
    {
        if( ent.item->Marker() & aMarker )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


ITEM_SET& ITEM_SET::FilterNet( int aNet, bool aInvert )
{
    ENTRIES newItems;

    for( const ENTRY& ent : m_items )
    {
        if( ( ent.item->Net() == aNet ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


ITEM_SET& ITEM_SET::ExcludeItem( const ITEM* aItem )
{
    ENTRIES newItems;

    for( const ENTRY& ent : m_items )
    {
        if( ent.item != aItem )

        newItems.push_back( ent );
    }

    m_items = newItems;

    return *this;
}

}

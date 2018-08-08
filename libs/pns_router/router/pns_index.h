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

#ifndef __PNS_INDEX_H
#define __PNS_INDEX_H

#include <layers_id_colors_and_visibility.h>
#include <map>
#include <unordered_set>

#include <boost/range/adaptor/map.hpp>

#include <list>
#include <geometry/shape_index.h>

#include "pns_item.h"

namespace PNS {


/**
 * Class INDEX
 *
 * Custom spatial index, holding our board items and allowing for very fast searches. Items
 * are assigned to separate R-Tree subindices depending on their type and spanned layers, reducing
 * overlap and improving search time.
 **/
class INDEX
{
public:
    typedef std::list<ITEM*>            NET_ITEMS_LIST;
    typedef SHAPE_INDEX<ITEM*>          ITEM_SHAPE_INDEX;
    typedef std::unordered_set<ITEM*>   ITEM_SET;

    INDEX();
    ~INDEX();

    /**
     * Function Add()
     *
     * Adds item to the spatial index.
     */
    void Add( ITEM* aItem );

    /**
     * Function Remove()
     *
     * Removes an item from the spatial index.
     */
    void Remove( ITEM* aItem );

    /**
     * Function Add()
     *
     * Replaces one item with another.
     */
    void Replace( ITEM* aOldItem, ITEM* aNewItem );

    /**
     * Function Query()
     *
     * Searches items in the index that are in proximity of aItem.
     * For each item, function object aVisitor is called. Only items on
     * overlapping layers are considered.
     *
     * @param aItem item to search against
     * @param aMinDistance proximity distance (wrs to the item's shape)
     * @param aVisitor function object called on each found item. Return
              false from the visitor to stop searching.
     * @return number of items found.
     */
    template<class Visitor>
    int Query( const ITEM* aItem, int aMinDistance, Visitor& aVisitor );

    /**
     * Function Query()
     *
     * Searches items in the index that are in proximity of aShape.
     * For each item, function object aVisitor is called. Treats all
     * layers as colliding.
     *
     * @param aShape shape to search against
     * @param aMinDistance proximity distance (wrs to the item's shape)
     * @param aVisitor function object called on each found item. Return
              false from the visitor to stop searching.
     * @return number of items found.
     */
    template<class Visitor>
    int Query( const SHAPE* aShape, int aMinDistance, Visitor& aVisitor );

    /**
     * Function Clear()
     *
     * Removes all items from the index.
     */
    void Clear();

    /**
     * Function GetItemsForNet()
     *
     * Returns list of all items in a given net.
     */
    NET_ITEMS_LIST* GetItemsForNet( int aNet );

    /**
     * Function Contains()
     *
     * Returns true if item aItem exists in the index.
     */
    bool Contains( ITEM* aItem ) const
    {
        return m_allItems.find( aItem ) != m_allItems.end();
    }

    /**
     * Function Size()
     *
     * Returns number of items stored in the index.
     */
    int Size() const { return m_allItems.size(); }

    ITEM_SET::iterator begin() { return m_allItems.begin(); }
    ITEM_SET::iterator end() { return m_allItems.end(); }

private:
    static const int    MaxSubIndices   = 128;
    static const int    SI_Multilayer   = 2;
    static const int    SI_SegDiagonal  = 0;
    static const int    SI_SegStraight  = 1;
    static const int    SI_Traces       = 3;
    static const int    SI_PadsTop      = 0;
    static const int    SI_PadsBottom   = 1;

    template <class Visitor>
    int querySingle( int index, const SHAPE* aShape, int aMinDistance, Visitor& aVisitor );

    ITEM_SHAPE_INDEX* getSubindex( const ITEM* aItem );

    ITEM_SHAPE_INDEX* m_subIndices[MaxSubIndices];
    std::map<int, NET_ITEMS_LIST> m_netMap;
    ITEM_SET m_allItems;
};

INDEX::INDEX()
{
    memset( m_subIndices, 0, sizeof( m_subIndices ) );
}

INDEX::ITEM_SHAPE_INDEX* INDEX::getSubindex( const ITEM* aItem )
{
    int idx_n = -1;

    const LAYER_RANGE l = aItem->Layers();

    switch( aItem->Kind() )
    {
    case ITEM::VIA_T:
        idx_n = SI_Multilayer;
        break;

    case ITEM::SOLID_T:
        {
            if( l.IsMultilayer() )
                idx_n = SI_Multilayer;
            else if( l.Start() == B_Cu ) // fixme: use kicad layer codes
                idx_n = SI_PadsTop;
            else if( l.Start() == F_Cu )
                idx_n = SI_PadsBottom;
            else
                idx_n = SI_Traces + 2 * l.Start() + SI_SegStraight;
        }
        break;

    case ITEM::SEGMENT_T:
    case ITEM::LINE_T:
        idx_n = SI_Traces + 2 * l.Start() + SI_SegStraight;
        break;

    default:
        break;
    }

    if( idx_n < 0 || idx_n >= MaxSubIndices )
    {
        assert( idx_n >= 0 );
        assert( idx_n < MaxSubIndices );
        return nullptr;
    }

    if( !m_subIndices[idx_n] )
        m_subIndices[idx_n] = new ITEM_SHAPE_INDEX;

    return m_subIndices[idx_n];
}

void INDEX::Add( ITEM* aItem )
{
    ITEM_SHAPE_INDEX* idx = getSubindex( aItem );

    if( !idx )
        return;

    idx->Add( aItem );
    m_allItems.insert( aItem );
    int net = aItem->Net();

    if( net >= 0 )
    {
        m_netMap[net].push_back( aItem );
    }
}

void INDEX::Remove( ITEM* aItem )
{
    ITEM_SHAPE_INDEX* idx = getSubindex( aItem );

    if( !idx )
        return;

    idx->Remove( aItem );
    m_allItems.erase( aItem );
    int net = aItem->Net();

    if( net >= 0 && m_netMap.find( net ) != m_netMap.end() )
        m_netMap[net].remove( aItem );
}

void INDEX::Replace( ITEM* aOldItem, ITEM* aNewItem )
{
    Remove( aOldItem );
    Add( aNewItem );
}

template<class Visitor>
int INDEX::querySingle( int index, const SHAPE* aShape, int aMinDistance, Visitor& aVisitor )
{
    if( !m_subIndices[index] )
        return 0;

    return m_subIndices[index]->Query( aShape, aMinDistance, aVisitor, false );
}

template<class Visitor>
int INDEX::Query( const ITEM* aItem, int aMinDistance, Visitor& aVisitor )
{
    const SHAPE* shape = aItem->Shape();
    int total = 0;

    total += querySingle( SI_Multilayer, shape, aMinDistance, aVisitor );

    const LAYER_RANGE layers = aItem->Layers();

    if( layers.IsMultilayer() )
    {
        total += querySingle( SI_PadsTop, shape, aMinDistance, aVisitor );
        total += querySingle( SI_PadsBottom, shape, aMinDistance, aVisitor );

        for( int i = layers.Start(); i <= layers.End(); ++i )
            total += querySingle( SI_Traces + 2 * i + SI_SegStraight, shape, aMinDistance, aVisitor );
    }
    else
    {
        int l = layers.Start();

        if( l == B_Cu )
            total += querySingle( SI_PadsTop, shape, aMinDistance, aVisitor );
        else if( l == F_Cu )
            total += querySingle( SI_PadsBottom, shape, aMinDistance, aVisitor );

        total += querySingle(  SI_Traces + 2 * l + SI_SegStraight, shape, aMinDistance, aVisitor );
    }

    return total;
}

template<class Visitor>
int INDEX::Query( const SHAPE* aShape, int aMinDistance, Visitor& aVisitor )
{
    int total = 0;

    for( int i = 0; i < MaxSubIndices; i++ )
        total += querySingle( i, aShape, aMinDistance, aVisitor );

    return total;
}

void INDEX::Clear()
{
    for( int i = 0; i < MaxSubIndices; ++i )
    {
        ITEM_SHAPE_INDEX* idx = m_subIndices[i];

        if( idx )
            delete idx;

        m_subIndices[i] = NULL;
    }
}

INDEX::~INDEX()
{
    Clear();
}

INDEX::NET_ITEMS_LIST* INDEX::GetItemsForNet( int aNet )
{
    if( m_netMap.find( aNet ) == m_netMap.end() )
        return NULL;

    return &m_netMap[aNet];
}

}

#endif

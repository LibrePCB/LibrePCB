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

#ifndef __PNS_JOINT_H
#define __PNS_JOINT_H

#include <vector>

#include <math/vector2d.h>

#include "pns_item.h"
#include "pns_segment.h"
#include "pns_itemset.h"

namespace PNS {

/**
 * Class JOINT
 *
 * Represents a 2D point on a given set of layers and belonging to a certain
 * net, that links together a number of board items.
 * A hash table of joints is used by the router to follow connectivity between
 * the items.
 **/
class JOINT : public ITEM
{
public:
    typedef ITEM_SET::ENTRIES LINKED_ITEMS;

    ///> Joints are hashed by their position, layers and net.
    ///  Linked items are, obviously, not hashed
    struct HASH_TAG
    {
        VECTOR2I pos;
        int net;
    };

    struct JOINT_TAG_HASH
    {
        std::size_t operator()( const JOINT::HASH_TAG& aP ) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            return ( (hash<int>()( aP.pos.x )
                      ^ (hash<int>()( aP.pos.y ) << 1) ) >> 1 )
                   ^ (hash<int>()( aP.net ) << 1);
        }
    };

    JOINT() :
        ITEM( JOINT_T ), m_locked( false ) {}

    JOINT( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet = -1 ) :
        ITEM( JOINT_T )
    {
        m_tag.pos = aPos;
        m_tag.net = aNet;
        m_layers = aLayers;
        m_locked = false;
    }

    JOINT( const JOINT& aB ) :
        ITEM( JOINT_T )
    {
        m_layers = aB.m_layers;
        m_tag.pos = aB.m_tag.pos;
        m_tag.net = aB.m_tag.net;
        m_linkedItems = aB.m_linkedItems;
        m_layers = aB.m_layers;
        m_locked = aB.m_locked;
    }

    ITEM* Clone( ) const override
    {
        assert( false );
        return NULL;
    }

    ///> Returns true if the joint is a trivial line corner, connecting two
    /// segments of the same net, on the same layer.
    bool IsLineCorner() const
    {
        if( m_linkedItems.Size() != 2 || m_linkedItems.Count( SEGMENT_T ) != 2 )
            return false;

        SEGMENT* seg1 = static_cast<SEGMENT*>( m_linkedItems[0] );
        SEGMENT* seg2 = static_cast<SEGMENT*>( m_linkedItems[1] );

        // joints between segments of different widths are not considered trivial.
        return seg1->Width() == seg2->Width();
    }

    bool IsNonFanoutVia() const
    {
        int vias = m_linkedItems.Count( VIA_T );
        int segs = m_linkedItems.Count( SEGMENT_T );

        return ( m_linkedItems.Size() == 3 && vias == 1 && segs == 2 );
    }

    bool IsTraceWidthChange() const
    {
        if( m_linkedItems.Size() != 2 )
            return false;

        if( m_linkedItems.Count( SEGMENT_T ) != 2)
            return false;

        SEGMENT* seg1 = static_cast<SEGMENT*>( m_linkedItems[0] );
        SEGMENT* seg2 = static_cast<SEGMENT*>( m_linkedItems[1] );

        return seg1->Width() != seg2->Width();
    }

    ///> Links the joint to a given board item (when it's added to the NODE)
    void Link( ITEM* aItem )
    {
        if( m_linkedItems.Contains( aItem ) )
            return;

        m_linkedItems.Add( aItem );
    }

    ///> Unlinks a given board item from the joint (upon its removal from a NODE)
    ///> Returns true if the joint became dangling after unlinking.
    bool Unlink( ITEM* aItem )
    {
        m_linkedItems.Erase( aItem );
        return m_linkedItems.Size() == 0;
    }

    ///> For trivial joints, returns the segment adjacent to (aCurrent). For non-trival ones, returns
    ///> NULL, indicating the end of line.
    SEGMENT* NextSegment( SEGMENT* aCurrent ) const
    {
        if( !IsLineCorner() )
            return NULL;

        return static_cast<SEGMENT*>( m_linkedItems[m_linkedItems[0] == aCurrent ? 1 : 0] );
    }

    VIA* Via()
    {
        for( ITEM* item : m_linkedItems.Items() )
        {
            if( item->OfKind( VIA_T ) )
                return static_cast<VIA*>( item );
        }

        return NULL;
    }


    /// trivial accessors
    const HASH_TAG& Tag() const
    {
        return m_tag;
    }

    const VECTOR2I& Pos() const
    {
        return m_tag.pos;
    }

    int Net() const
    {
        return m_tag.net;
    }

    const LINKED_ITEMS& LinkList() const
    {
        return m_linkedItems.CItems();
    }

    const ITEM_SET& CLinks() const
    {
        return m_linkedItems;
    }

    ITEM_SET& Links()
    {
        return m_linkedItems;
    }

    int LinkCount( int aMask = -1 ) const
    {
        return m_linkedItems.Count( aMask );
    }

    void Dump() const;

    bool operator==( const JOINT& rhs )  const
    {
        return m_tag.pos == rhs.m_tag.pos && m_tag.net == rhs.m_tag.net;
    }

    void Merge( const JOINT& aJoint )
    {
        if( !Overlaps( aJoint ) )
            return;

        m_layers.Merge( aJoint.m_layers );

        if( aJoint.IsLocked() )
            m_locked = true;

        for( ITEM* item : aJoint.LinkList() )
        {
            m_linkedItems.Add( item );
        }
    }

    bool Overlaps( const JOINT& rhs ) const
    {
        return m_tag.pos == rhs.m_tag.pos &&
            m_tag.net == rhs.m_tag.net && m_layers.Overlaps( rhs.m_layers );
    }

    void Lock( bool aLock = true )
    {
        m_locked = aLock;
    }

    bool IsLocked() const
    {
        return m_locked;
    }

private:
    ///> hash tag for unordered_multimap
    HASH_TAG m_tag;

    ///> list of items linked to this joint
    ITEM_SET m_linkedItems;

    ///> locked (non-movable) flag
    bool m_locked;
};

inline bool operator==( JOINT::HASH_TAG const& aP1, JOINT::HASH_TAG const& aP2 )
{
    return aP1.pos == aP2.pos && aP1.net == aP2.net;
}

}

#endif    // __PNS_JOINT_H

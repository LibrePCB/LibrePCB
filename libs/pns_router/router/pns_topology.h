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

#ifndef __PNS_TOPOLOGY_H
#define __PNS_TOPOLOGY_H

#include <vector>
#include <set>

#include "pns_itemset.h"

namespace PNS {

class NODE;
class SEGMENT;
class JOINT;
class ITEM;
class SOLID;
class DIFF_PAIR;

class TOPOLOGY
{
public:
    typedef std::set<JOINT*> JOINT_SET;

    TOPOLOGY( NODE* aNode ):
        m_world( aNode ) {};

    ~TOPOLOGY() {};

    bool SimplifyLine( LINE *aLine );
    ITEM* NearestUnconnectedItem( JOINT* aStart, int* aAnchor = NULL, int aKindMask = ITEM::ANY_T );
    bool LeadingRatLine( const LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine );

    const JOINT_SET ConnectedJoints( JOINT* aStart );
    const ITEM_SET ConnectedItems( JOINT* aStart, int aKindMask = ITEM::ANY_T );
    const ITEM_SET ConnectedItems( ITEM* aStart, int aKindMask = ITEM::ANY_T );
    int64_t ShortestConnectionLength( ITEM* aFrom, ITEM* aTo );

    const ITEM_SET AssembleTrivialPath( ITEM* aStart );
    const DIFF_PAIR AssembleDiffPair( SEGMENT* aStart );

    int DpCoupledNet( int aNet );
    int DpNetPolarity( int aNet );
    const LINE DpCoupledLine( LINE* aLine );
    bool AssembleDiffPair( ITEM* aStart, DIFF_PAIR& aPair );

    const std::set<ITEM*> AssembleCluster( ITEM* aStart, int aLayer );

private:
    bool followTrivialPath( LINE* aLine, bool aLeft, ITEM_SET& aSet, std::set<ITEM*>& aVisited );

    NODE *m_world;
};

}

#endif

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

#ifndef __PNS_WALKAROUND_H
#define __PNS_WALKAROUND_H

#include <set>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_logger.h"
#include "pns_algo_base.h"

namespace PNS {

class WALKAROUND : public ALGO_BASE
{
    static const int DefaultIterationLimit = 50;

public:
    WALKAROUND( NODE* aWorld, ROUTER* aRouter ) :
        ALGO_BASE ( aRouter ),
        m_world( aWorld ),
        m_iterationLimit( DefaultIterationLimit )
    {
        m_forceSingleDirection = false;
        m_forceLongerPath = false;
        m_forceWinding = false;
        m_cursorApproachMode = false;
        m_itemMask = ITEM::ANY_T;

        // Initialize other members, to avoid uninitialized variables.
        m_recursiveBlockageCount = 0;
        m_recursiveCollision[0] = m_recursiveCollision[1] = false;
        m_iteration = 0;
        m_forceCw = false;
    }

    ~WALKAROUND() {};

    enum WALKAROUND_STATUS
    {
        IN_PROGRESS = 0,
        DONE,
        STUCK
    };

    void SetWorld( NODE* aNode )
    {
        m_world = aNode;
    }

    void SetIterationLimit( const int aIterLimit )
    {
        m_iterationLimit = aIterLimit;
    }

    void SetSolidsOnly( bool aSolidsOnly )
    {
        if( aSolidsOnly )
            m_itemMask = ITEM::SOLID_T;
        else
            m_itemMask = ITEM::ANY_T;
    }

    void SetItemMask( int aMask )
    {
        m_itemMask = aMask;
    }

    void SetSingleDirection( bool aForceSingleDirection )
    {
        m_forceSingleDirection = aForceSingleDirection;
        m_forceLongerPath = aForceSingleDirection;
    }

    void SetSingleDirection2( bool aForceSingleDirection )
    {
        m_forceSingleDirection = aForceSingleDirection;
    }

    void SetApproachCursor( bool aEnabled, const VECTOR2I& aPos )
    {
        m_cursorPos = aPos;
        m_cursorApproachMode = aEnabled;
    }

    void SetForceWinding ( bool aEnabled, bool aCw )
    {
        m_forceCw = aCw;
        m_forceWinding = aEnabled;
    }

    void RestrictToSet( bool aEnabled, const std::set<ITEM*>& aSet )
    {
        if( aEnabled )
            m_restrictedSet = aSet;
        else
            m_restrictedSet.clear();
    }

    WALKAROUND_STATUS Route( const LINE& aInitialPath, LINE& aWalkPath,
            bool aOptimize = true );

    virtual LOGGER* Logger() override
    {
        return &m_logger;
    }

private:
    void start( const LINE& aInitialPath );

    WALKAROUND_STATUS singleStep( LINE& aPath, bool aWindingDirection );
    NODE::OPT_OBSTACLE nearestObstacle( const LINE& aPath );

    NODE* m_world;

    int m_recursiveBlockageCount;
    int m_iteration;
    int m_iterationLimit;
    int m_itemMask;
    bool m_forceSingleDirection, m_forceLongerPath;
    bool m_cursorApproachMode;
    bool m_forceWinding;
    bool m_forceCw;
    VECTOR2I m_cursorPos;
    NODE::OPT_OBSTACLE m_currentObstacle[2];
    bool m_recursiveCollision[2];
    LOGGER m_logger;
    std::set<ITEM*> m_restrictedSet;
};

}

#endif    // __PNS_WALKAROUND_H

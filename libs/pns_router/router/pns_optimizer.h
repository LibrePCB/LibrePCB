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

#ifndef __PNS_OPTIMIZER_H
#define __PNS_OPTIMIZER_H

#include <unordered_map>
#include <memory>

#include <geometry/shape_index_list.h>
#include <geometry/shape_line_chain.h>

#include "range.h"

namespace PNS {

class NODE;
class ROUTER;
class LINE;
class DIFF_PAIR;

/**
 * Class COST_ESTIMATOR
 *
 * Calculates the cost of a given line, taking corner angles and total length into account.
 **/
class COST_ESTIMATOR
{
public:
    COST_ESTIMATOR() :
        m_lengthCost( 0 ),
        m_cornerCost( 0 )
    {}

    COST_ESTIMATOR( const COST_ESTIMATOR& aB ) :
        m_lengthCost( aB.m_lengthCost ),
        m_cornerCost( aB.m_cornerCost )
    {}

    ~COST_ESTIMATOR() {};

    static int CornerCost( const SEG& aA, const SEG& aB );
    static int CornerCost( const SHAPE_LINE_CHAIN& aLine );
    static int CornerCost( const LINE& aLine );

    void Add( LINE& aLine );
    void Remove( LINE& aLine );
    void Replace( LINE& aOldLine, LINE& aNewLine );

    bool IsBetter( COST_ESTIMATOR& aOther, double aLengthTolerance,
                   double aCornerTollerace ) const;

    double GetLengthCost() const { return m_lengthCost; }
    double GetCornerCost() const { return m_cornerCost; }

private:
    double m_lengthCost;
    int m_cornerCost;
};

/**
 * Class OPTIMIZER
 *
 * Performs various optimizations of the lines being routed, attempting to make the lines shorter
 * and less cornery. There are 3 kinds of optimizations so far:
 * - Merging obtuse segments (MERGE_OBTUSE): tries to join together as many
 *   obtuse segments as possible without causing collisions
 * - Rerouting path between pair of line corners with a 2-segment "\__" line and iteratively repeating
 *   the procedure as long as the total cost of the line keeps decreasing
 * - "Smart Pads" - that is, rerouting pad/via exits to make them look nice (SMART_PADS).
 **/
class OPTIMIZER
{
public:
    enum OptimizationEffort
    {
        MERGE_SEGMENTS  = 0x01,
        SMART_PADS      = 0x02,
        MERGE_OBTUSE    = 0x04,
        FANOUT_CLEANUP    = 0x08
    };

    OPTIMIZER( NODE* aWorld );
    ~OPTIMIZER();

    ///> a quick shortcut to optmize a line without creating and setting up an optimizer
    static bool Optimize( LINE* aLine, int aEffortLevel, NODE* aWorld);

    bool Optimize( LINE* aLine, LINE* aResult = NULL );
    bool Optimize( DIFF_PAIR* aPair );


    void SetWorld( NODE* aNode ) { m_world = aNode; }
    void CacheStaticItem( ITEM* aItem );
    void CacheRemove( ITEM* aItem );
    void ClearCache( bool aStaticOnly = false );

    void SetCollisionMask( int aMask )
    {
        m_collisionKindMask = aMask;
    }

    void SetEffortLevel( int aEffort )
    {
        m_effortLevel = aEffort;
    }


    void SetRestrictArea( const BOX2I& aArea )
    {
        m_restrictArea = aArea;
        m_restrictAreaActive = true;
    }

private:
    static const int MaxCachedItems = 256;

    typedef std::vector<SHAPE_LINE_CHAIN> BREAKOUT_LIST;

    struct CACHE_VISITOR;

    struct CACHED_ITEM
    {
        int m_hits;
        bool m_isStatic;
    };

    bool mergeObtuse( LINE* aLine );
    bool mergeFull( LINE* aLine );
    bool removeUglyCorners( LINE* aLine );
    bool runSmartPads( LINE* aLine );
    bool mergeStep( LINE* aLine, SHAPE_LINE_CHAIN& aCurrentLine, int step );
    bool fanoutCleanup( LINE * aLine );
    bool mergeDpSegments( DIFF_PAIR *aPair );
    bool mergeDpStep( DIFF_PAIR *aPair, bool aTryP, int step );

    bool checkColliding( ITEM* aItem, bool aUpdateCache = true );
    bool checkColliding( LINE* aLine, const SHAPE_LINE_CHAIN& aOptPath );

    void cacheAdd( ITEM* aItem, bool aIsStatic );
    void removeCachedSegments( LINE* aLine, int aStartVertex = 0, int aEndVertex = -1 );

    BREAKOUT_LIST circleBreakouts( int aWidth, const SHAPE* aShape, bool aPermitDiagonal ) const;
    BREAKOUT_LIST rectBreakouts( int aWidth, const SHAPE* aShape, bool aPermitDiagonal ) const;
    BREAKOUT_LIST ovalBreakouts( int aWidth, const SHAPE* aShape, bool aPermitDiagonal ) const;
    BREAKOUT_LIST customBreakouts( int aWidth, const ITEM* aItem, bool aPermitDiagonal ) const;
    BREAKOUT_LIST computeBreakouts( int aWidth, const ITEM* aItem, bool aPermitDiagonal ) const;

    int smartPadsSingle( LINE* aLine, ITEM* aPad, bool aEnd, int aEndVertex );

    ITEM* findPadOrVia( int aLayer, int aNet, const VECTOR2I& aP ) const;

    SHAPE_INDEX_LIST<ITEM*> m_cache;

    typedef std::unordered_map<ITEM*, CACHED_ITEM> CachedItemTags;
    CachedItemTags m_cacheTags;
    NODE* m_world;
    int m_collisionKindMask;
    int m_effortLevel;
    bool m_keepPostures;

    BOX2I m_restrictArea;
    bool m_restrictAreaActive;
};

}

#endif

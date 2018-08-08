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

#ifndef __PNS_DP_MEANDER_PLACER_H
#define __PNS_DP_MEANDER_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"
#include "pns_diff_pair.h"
#include "pns_debug_decorator.h"

namespace PNS {

class ROUTER;

/**
 * Class DP_MEANDER_PLACER
 *
 * Differential Pair length-matching/meandering tool.
 */

class DP_MEANDER_PLACER : public MEANDER_PLACER_BASE
{
public:
    DP_MEANDER_PLACER( ROUTER* aRouter );
    ~DP_MEANDER_PLACER();

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor
     * (unless NULL).
     */
    bool Start( const VECTOR2I& aP, ITEM* aStartItem ) override;

    /**
     * Function Move()
     *
     * Moves the end of the currently routed trace to the point aP, taking
     * aEndItem as anchor (if not NULL).
     * (unless NULL).
     */
    bool Move( const VECTOR2I& aP, ITEM* aEndItem ) override;

    /**
     * Function FixRoute()
     *
     * Commits the currently routed track to the parent node, taking
     * aP as the final end point and aEndItem as the final anchor (if provided).
     * @return true, if route has been commited. May return false if the routing
     * result is violating design rules - in such case, the track is only committed
     * if Settings.CanViolateDRC() is on.
     */
    bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) override;

    const LINE Trace() const;

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state.
     */
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    const ITEM_SET Traces() override;

    const VECTOR2I& CurrentEnd() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNets()
    const std::vector<int> CurrentNets() const override;

    int CurrentLayer() const override;

    int totalLength();

    const std::string TuningInfo() const override;
    TUNING_STATUS TuningStatus() const override;

    bool CheckFit( MEANDER_SHAPE* aShape ) override;


private:
    friend class MEANDER_SHAPE;

    void meanderSegment( const SEG& aBase );

//    void addMeander ( PNS_MEANDER *aM );
//    void addCorner ( const VECTOR2I& aP );

    const SEG baselineSegment( const DIFF_PAIR::COUPLED_SEGMENTS& aCoupledSegs );

    void setWorld( NODE* aWorld );
    void release();

    int origPathLength() const;

    ///> pointer to world to search colliding items
    NODE* m_world;

    ///> current routing start point (end of tail, beginning of head)
    VECTOR2I m_currentStart;

    ///> Current world state
    NODE* m_currentNode;

    DIFF_PAIR m_originPair;
    DIFF_PAIR::COUPLED_SEGMENTS_VEC m_coupledSegments;

    LINE m_currentTraceN, m_currentTraceP;
    ITEM_SET m_tunedPath, m_tunedPathP, m_tunedPathN;

    SHAPE_LINE_CHAIN m_finalShapeP, m_finalShapeN;
    MEANDERED_LINE m_result;
    SEGMENT* m_initialSegment;

    int m_lastLength;
    TUNING_STATUS m_lastStatus;
};

}

#endif    // __PNS_DP_MEANDER_PLACER_H

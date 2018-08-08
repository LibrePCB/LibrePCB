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

#ifndef __PNS_DIFF_PLACER_H
#define __PNS_DIFF_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_sizes_settings.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_algo_base.h"
#include "pns_diff_pair.h"

#include "pns_placement_algo.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;
class VIA;
class SIZES_SETTINGS;


/**
 * Class LINE_PLACER
 *
 * Single track placement algorithm. Interactively routes a track.
 * Applies shove and walkaround algorithms when needed.
 */

class DIFF_PAIR_PLACER : public PLACEMENT_ALGO
{
public:
    DIFF_PAIR_PLACER( ROUTER* aRouter );
    ~DIFF_PAIR_PLACER();

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
    bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish ) override;

    /**
     * Function ToggleVia()
     *
     * Enables/disables a via at the end of currently routed trace.
     */
    bool ToggleVia( bool aEnabled ) override;

    /**
     * Function SetLayer()
     *
     * Sets the current routing layer.
     */
    bool SetLayer( int aLayer ) override;

    /**
     * Function Traces()
     *
     * Returns the complete routed line, as a single-member ITEM_SET.
     */
    const ITEM_SET Traces() override;

    /**
     * Function CurrentEnd()
     *
     * Returns the current end of the line being placed. It may not be equal
     * to the cursor position due to collisions.
     */
    const VECTOR2I& CurrentEnd() const override
    {
        return m_currentEnd;
    }

    /**
     * Function CurrentNets()
     *
     * Returns the net code of currently routed track.
     */
    const std::vector<int> CurrentNets() const override;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently routed track.
     */
    int CurrentLayer() const override
    {
        return m_currentLayer;
    }

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state.
     */
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    /**
     * Function FlipPosture()
     *
     * Toggles the current posture (straight/diagonal) of the trace head.
     */
    void FlipPosture() override;

    /**
     * Function UpdateSizes()
     *
     * Performs on-the-fly update of the width, via diameter & drill size from
     * a settings class. Used to dynamically change these parameters as
     * the track is routed.
     */
    void UpdateSizes( const SIZES_SETTINGS& aSizes ) override;

    bool IsPlacingVia() const override { return m_placingVia; }

    void SetOrthoMode( bool aOrthoMode ) override;

    void GetModifiedNets( std::vector<int>& aNets ) const override;

private:
    int viaGap() const;
    int gap() const;

    /**
     * Function route()
     *
     * Re-routes the current track to point aP. Returns true, when routing has
     * completed successfully (i.e. the trace end has reached point aP), and false
     * if the trace was stuck somewhere on the way. May call routeStep()
     * repetitively due to mouse smoothing.
     * @param aP ending point of current route.
     * @return true, if the routing is complete.
     */
    bool route( const VECTOR2I& aP );

    /**
     * Function updateLeadingRatLine()
     *
     * Draws the "leading" ratsnest line, which connects the end of currently
     * routed track and the nearest yet unrouted item. If the routing for
     * current net is complete, draws nothing.
     */
    void updateLeadingRatLine();

    /**
     * Function setWorld()
     *
     * Sets the board to route.
     */
    void setWorld( NODE* aWorld );

    /**
     * Function startPlacement()
     *
     * Initializes placement of a new line with given parameters.
     */
    void initPlacement( );

    /**
     * Function setInitialDirection()
     *
     * Sets preferred direction of the very first track segment to be laid.
     * Used by posture switching mechanism.
     */
    void setInitialDirection( const DIRECTION_45& aDirection );


    bool routeHead( const VECTOR2I& aP );
    bool tryWalkDp( NODE* aNode, DIFF_PAIR& aPair, bool aSolidsOnly );

    ///> route step, walkaround mode
    bool rhWalkOnly( const VECTOR2I& aP );

    ///> route step, shove mode
    bool rhShoveOnly ( const VECTOR2I& aP );

    ///> route step, mark obstacles mode
    bool rhMarkObstacles( const VECTOR2I& aP );

    const VIA makeVia ( const VECTOR2I& aP, int aNet );

    bool findDpPrimitivePair( const VECTOR2I& aP, ITEM* aItem, DP_PRIMITIVE_PAIR& aPair, std::string* aErrorMsg = nullptr );
    OPT_VECTOR2I getDanglingAnchor( NODE* aNode, ITEM* aItem );
    bool attemptWalk( NODE* aNode, DIFF_PAIR* aCurrent, DIFF_PAIR& aWalk, bool aPFirst, bool aWindCw, bool aSolidsOnly );
    bool propagateDpHeadForces ( const VECTOR2I& aP, VECTOR2I& aNewP );

    enum State {
        RT_START = 0,
        RT_ROUTE = 1,
        RT_FINISH = 2
    };

    State m_state;

    bool m_chainedPlacement;
    bool m_initialDiagonal;
    bool m_startDiagonal;
    bool m_fitOk;

    int m_netP, m_netN;

    DP_PRIMITIVE_PAIR m_start;
    OPT<DP_PRIMITIVE_PAIR> m_prevPair;

    ///> current algorithm iteration
    int m_iteration;

    ///> pointer to world to search colliding items
    NODE* m_world;

    ///> current routing start point (end of tail, beginning of head)
    VECTOR2I m_p_start;

    ///> The shove engine
    SHOVE* m_shove;

    ///> Current world state
    NODE* m_currentNode;

    ///> Postprocessed world state (including marked collisions & removed loops)
    NODE* m_lastNode;

    SIZES_SETTINGS m_sizes;

    ///> Are we placing a via?
    bool m_placingVia;

    ///> current via diameter
    int m_viaDiameter;

    ///> current via drill
    int m_viaDrill;

    ///> current track width
    int m_currentWidth;

    int m_currentNet;
    int m_currentLayer;

    bool m_startsOnVia;
    bool m_orthoMode;
    bool m_snapOnTarget;

    VECTOR2I m_currentEnd, m_currentStart;
    DIFF_PAIR m_currentTrace;

    ITEM* m_currentEndItem;
    PNS_MODE m_currentMode;

    bool m_idle;
};

}

#endif    // __PNS_LINE_PLACER_H

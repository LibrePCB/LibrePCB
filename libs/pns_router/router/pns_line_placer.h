/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
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

#ifndef __PNS_LINE_PLACER_H
#define __PNS_LINE_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_sizes_settings.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
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

class LINE_PLACER : public PLACEMENT_ALGO
{
public:
    LINE_PLACER( ROUTER* aRouter );
    ~LINE_PLACER();

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
     * Function Head()
     *
     * Returns the "head" of the line being placed, that is the volatile part
     * that has not "settled" yet.
     */
    const LINE& Head() const { return m_head; }

    /**
     * Function Tail()
     *
     * Returns the "tail" of the line being placed, the part which has already wrapped around
     * and shoved some obstacles.
     */
    const LINE& Tail() const { return m_tail; }

    /**
     * Function Trace()
     *
     * Returns the complete routed line.
     */
    const LINE Trace() const;

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
     * Function CurrentNet()
     *
     * Returns the net code of currently routed track.
     */
    const std::vector<int> CurrentNets() const override
    {
        return std::vector<int>( 1, m_currentNet );
    }

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

    void SetOrthoMode( bool aOrthoMode ) override;

    bool IsPlacingVia() const override { return m_placingVia; }

    void GetModifiedNets( std::vector<int>& aNets ) const override;

    LOGGER* Logger() override;

    /**
     * Function SplitAdjacentSegments()
     *
     * Checks if point aP lies on segment aSeg. If so, splits the segment in two,
     * forming a joint at aP and stores updated topology in node aNode.
     */
    bool SplitAdjacentSegments( NODE* aNode, ITEM* aSeg, const VECTOR2I& aP );


private:
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
    void initPlacement();

    /**
     * Function setInitialDirection()
     *
     * Sets preferred direction of the very first track segment to be laid.
     * Used by posture switching mechanism.
     */
    void setInitialDirection( const DIRECTION_45& aDirection );

    /**
     * Function removeLoops()
     *
     * Searches aNode for traces concurrent to aLatest and removes them. Updated
     * topology is stored in aNode.
     */
    void removeLoops( NODE* aNode, LINE& aLatest );

    /**
     * Function simplifyNewLine()
     *
     * Assembles a line starting from segment aLatest, removes collinear segments
     * and redundant vertexes. If a simplification bhas been found, replaces the
     * old line with the simplified one in aNode.
     */
    void simplifyNewLine( NODE* aNode, SEGMENT* aLatest );

    /**
     * Function checkObtusity()
     *
     * Helper function, checking if segments a and b form an obtuse angle
     * (in 45-degree regime).
     * @return true, if angle (aA, aB) is obtuse
     */
    bool checkObtusity( const SEG& aA, const SEG& aB ) const;

    /**
     * Function handleSelfIntersections()
     *
     * Checks if the head of the track intersects its tail. If so, cuts the
     * tail up to the intersecting segment and fixes the head direction to match
     * the last segment before the cut.
     * @return true if the line has been changed.
     */
    bool handleSelfIntersections();

    /**
     * Function handlePullback()
     *
     * Deals with pull-back: reduces the tail if head trace is moved backwards
     * wrs to the current tail direction.
     * @return true if the line has been changed.
     */
    bool handlePullback();

    /**
     * Function mergeHead()
     *
     * Moves "estabished" segments from the head to the tail if certain
     * conditions are met.
     * @return true, if the line has been changed.
     */
    bool mergeHead();

    /**
     * Function reduceTail()
     *
     * Attempts to reduce the numer of segments in the tail by trying to replace a
     * certain number of latest tail segments with a direct trace leading to aEnd
     * that does not collide with anything.
     * @param aEnd: current routing destination point.
     * @return true if the line has been changed.
     */
    bool reduceTail( const VECTOR2I& aEnd );

    /**
     * Function optimizeTailHeadTransition()
     *
     * Tries to reduce the corner count of the most recent part of tail/head by
     * merging obtuse/collinear segments.
     * @return true, if the line has been changed.
     */
    bool optimizeTailHeadTransition();

    /**
     * Function routeHead()
     *
     * Computes the head trace between the current start point (m_p_start) and
     * point aP, starting with direction defined in m_direction. The trace walks
     * around all colliding solid or non-movable items. Movable segments are
     * ignored, as they'll be handled later by the shove algorithm.
     */
    bool routeHead( const VECTOR2I& aP, LINE& aNewHead);

    /**
     * Function routeStep()
     *
     * Performs a single routing alorithm step, for the end point aP.
     * @param aP ending point of current route
     * @return true, if the line has been changed.
     */
    void routeStep( const VECTOR2I& aP );

    const LINE reduceToNearestObstacle( const LINE& aOriginalLine );

    bool rhStopAtNearestObstacle( const VECTOR2I& aP, LINE& aNewHead );


    ///> route step, walkaround mode
    bool rhWalkOnly( const VECTOR2I& aP, LINE& aNewHead);

    ///> route step, shove mode
    bool rhShoveOnly( const VECTOR2I& aP, LINE& aNewHead);

    ///> route step, mark obstacles mode
    bool rhMarkObstacles( const VECTOR2I& aP, LINE& aNewHead );

    const VIA makeVia( const VECTOR2I& aP );

    bool buildInitialLine( const VECTOR2I& aP, LINE& aHead, bool aInvertPosture = false );

    ///> current routing direction
    DIRECTION_45 m_direction;

    ///> routing direction for new traces
    DIRECTION_45 m_initial_direction;

    ///> routing "head": volatile part of the track from the previously
    ///  analyzed point to the current routing destination
    LINE m_head;

    ///> routing "tail": part of the track that has been already fixed due to collisions with obstacles
    LINE m_tail;

    ///> pointer to world to search colliding items
    NODE* m_world;

    ///> current routing start point (end of tail, beginning of head)
    VECTOR2I m_p_start;

    ///> The shove engine
    std::unique_ptr< SHOVE > m_shove;

    ///> Current world state
    NODE* m_currentNode;

    ///> Postprocessed world state (including marked collisions & removed loops)
    NODE* m_lastNode;

    SIZES_SETTINGS m_sizes;

    ///> Are we placing a via?
    bool m_placingVia;

    int m_currentNet;
    int m_currentLayer;

    VECTOR2I m_currentEnd, m_currentStart;
    LINE m_currentTrace;

    PNS_MODE m_currentMode;
    ITEM* m_startItem;

    bool m_idle;
    bool m_chainedPlacement;
    bool m_orthoMode;
};

}

#endif    // __PNS_LINE_PLACER_H

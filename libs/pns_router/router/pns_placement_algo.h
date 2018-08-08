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

#ifndef __PNS_PLACEMENT_ALGO_H
#define __PNS_PLACEMENT_ALGO_H

#include <math/vector2d.h>

#include "pns_algo_base.h"
#include "pns_sizes_settings.h"
#include "pns_itemset.h"

namespace PNS {

class ROUTER;
class ITEM;
class NODE;

/**
 * Class PLACEMENT_ALGO
 *
 * Abstract class for a P&S placement/dragging algorithm.
 * All subtools (drag, single/diff pair routing and meandering)
 * are derived from it.
 */

class PLACEMENT_ALGO : public ALGO_BASE
{
public:
    PLACEMENT_ALGO( ROUTER* aRouter ) :
        ALGO_BASE( aRouter ) {};

    virtual ~PLACEMENT_ALGO () {};

    /**
     * Function Start()
     *
     * Starts placement/drag operation at point aP, taking item aStartItem as anchor
     * (unless NULL).
     */
    virtual bool Start( const VECTOR2I& aP, ITEM* aStartItem ) = 0;

    /**
     * Function Move()
     *
     * Moves the end of the currently routed primtive(s) to the point aP, taking
     * aEndItem as the anchor (if not NULL).
     * (unless NULL).
     */
    virtual bool Move( const VECTOR2I& aP, ITEM* aEndItem ) = 0;

    /**
     * Function FixRoute()
     *
     * Commits the currently routed items to the parent node, taking
     * aP as the final end point and aEndItem as the final anchor (if provided).
     * @return true, if route has been commited. May return false if the routing
     * result is violating design rules - in such case, the track is only committed
     * if Settings.CanViolateDRC() is on.
     */
    virtual bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) = 0;

    /**
     * Function ToggleVia()
     *
     * Enables/disables a via at the end of currently routed trace.
     */
    virtual bool ToggleVia( bool aEnabled )
    {
        return false;
    }

    /**
     * Function IsPlacingVia()
     *
     * Returns true if the placer is placing a via (or more vias).
     */
    virtual bool IsPlacingVia() const
    {
        return false;
    }

    /**
     * Function SetLayer()
     *
     * Sets the current routing layer.
     */
    virtual bool SetLayer( int aLayer )
    {
        return false;
    }

    /**
     * Function Traces()
     *
     * Returns all routed/tuned traces.
     */
    virtual const ITEM_SET Traces() = 0;

    /**
     * Function CurrentEnd()
     *
     * Returns the current end of the line(s) being placed/tuned. It may not be equal
     * to the cursor position due to collisions.
     */
    virtual const VECTOR2I& CurrentEnd() const = 0;

    /**
     * Function CurrentNets()
     *
     * Returns the net code(s) of currently routed track(s).
     */
    virtual const std::vector<int> CurrentNets() const = 0;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently routed track.
     */
    virtual int CurrentLayer() const = 0;

    /**
     * Function CurrentNode()
     *
     * Returns the most recent board state.
     */
    virtual NODE* CurrentNode( bool aLoopsRemoved = false ) const = 0;

    /**
     * Function FlipPosture()
     *
     * Toggles the current posture (straight/diagonal) of the trace head.
     */
    virtual void FlipPosture()
    {
    }

    /**
     * Function UpdateSizes()
     *
     * Performs on-the-fly update of the width, via diameter & drill size from
     * a settings class. Used to dynamically change these parameters as
     * the track is routed.
     */
    virtual void UpdateSizes( const SIZES_SETTINGS& aSizes )
    {
    }

    /**
     * Function SetOrthoMode()
     *
     * Forces the router to place a straight 90/45 degree trace (with the end
     * as near to the cursor as possible) instead of a standard 135 degree
     * two-segment bend.
     */
    virtual void SetOrthoMode ( bool aOrthoMode )
    {
    }

    /**
     * Function GetModifiedNets
     *
     * Returns the net codes of all currently routed trace(s)
     */
    virtual void GetModifiedNets( std::vector<int> &aNets ) const
    {
    }
};

}

#endif

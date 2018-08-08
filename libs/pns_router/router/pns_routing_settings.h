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

#ifndef __PNS_ROUTING_SETTINGS
#define __PNS_ROUTING_SETTINGS

#include <cstdio>

#include "time_limit.h"

class DIRECTION_45;
class TOOL_SETTINGS;

namespace PNS {

///> Routing modes
enum PNS_MODE
{
    RM_MarkObstacles = 0,   ///> Ignore collisions, mark obstacles
    RM_Shove,               ///> Only shove
    RM_Walkaround,          ///> Only walkaround
    RM_Smart                ///> Guess what's better, try to make least mess on the PCB
};

///> Optimization effort
enum PNS_OPTIMIZATION_EFFORT
{
    OE_LOW = 0,
    OE_MEDIUM = 1,
    OE_FULL = 2
};

/**
 * Class ROUTING_SETTINGS
 *
 * Contains all persistent settings of the router, such as the mode, optimization effort, etc.
 */

class ROUTING_SETTINGS
{
public:
    ROUTING_SETTINGS();

    //void Load( const TOOL_SETTINGS& where );
    //void Save( TOOL_SETTINGS& where ) const;

    ///> Returns the routing mode.
    PNS_MODE Mode() const { return m_routingMode; }

    ///> Sets the routing mode.
    void SetMode( PNS_MODE aMode ) { m_routingMode = aMode; }

    ///> Returns the optimizer effort. Bigger means cleaner traces, but slower routing.
    PNS_OPTIMIZATION_EFFORT OptimizerEffort() const { return m_optimizerEffort; }

    ///> Sets the optimizer effort. Bigger means cleaner traces, but slower routing.
    void SetOptimizerEffort( PNS_OPTIMIZATION_EFFORT aEffort ) { m_optimizerEffort = aEffort; }

    ///> Returns true if shoving vias is enbled.
    bool ShoveVias() const { return m_shoveVias; }

    ///> Enables/disables shoving vias.
    void SetShoveVias( bool aShoveVias ) { m_shoveVias = aShoveVias; }

    ///> Returns true if loop (redundant track) removal is on.
    bool RemoveLoops() const { return m_removeLoops; }

    ///> Enables/disables loop (redundant track) removal.
    void SetRemoveLoops( bool aRemoveLoops ) { m_removeLoops = aRemoveLoops; }

    ///> Returns true if suggesting the finish of currently placed track is on.
    bool SuggestFinish() { return m_suggestFinish; }

    ///> Enables displaying suggestions for finishing the currently placed track.
    void SetSuggestFinish( bool aSuggestFinish ) { m_suggestFinish = aSuggestFinish; }

    ///> Returns true if Smart Pads (optimized connections) is enabled.
    bool SmartPads() const { return m_smartPads; }

    ///> Enables/disables Smart Pads (optimized connections).
    void SetSmartPads( bool aSmartPads ) { m_smartPads = aSmartPads; }

    ///> Returns true if follow mouse mode is active (permanently on for the moment).
    bool FollowMouse() const
    {
        return m_followMouse && !( Mode() == RM_MarkObstacles );
    }

    ///> Returns true if smoothing segments durign dragging is enabled.
    bool SmoothDraggedSegments() const { return m_smoothDraggedSegments; }

    ///> Enables/disabled smoothing segments during dragging.
    void SetSmoothDraggedSegments( bool aSmooth ) { m_smoothDraggedSegments = aSmooth; }

    ///> Returns true if jumping over unmovable obstacles is on.
    bool JumpOverObstacles() const { return m_jumpOverObstacles; }

    ///> Enables/disables jumping over unmovable obstacles.
    void SetJumpOverObstacles( bool aJumpOverObstacles ) { m_jumpOverObstacles = aJumpOverObstacles; }

    void SetStartDiagonal( bool aStartDiagonal ) { m_startDiagonal = aStartDiagonal; }

    bool CanViolateDRC() const { return m_canViolateDRC; }
    void SetCanViolateDRC( bool aViolate ) { m_canViolateDRC = aViolate; }

    bool GetFreeAngleMode() const { return m_freeAngleMode; }

    void SetFreeAngleMode( bool aEnable ) { m_freeAngleMode = aEnable; }

    const DIRECTION_45 InitialDirection() const;

    int ShoveIterationLimit() const;
    TIME_LIMIT ShoveTimeLimit() const;

    int WalkaroundIterationLimit() const { return m_walkaroundIterationLimit; };
    TIME_LIMIT WalkaroundTimeLimit() const;

    void SetInlineDragEnabled ( bool aEnable ) { m_inlineDragEnabled = aEnable; }
    bool InlineDragEnabled() const { return m_inlineDragEnabled; }

    void SetSnapToTracks( bool aSnap ) { m_snapToTracks = aSnap; }
    void SetSnapToPads( bool aSnap ) { m_snapToPads = aSnap; }

    bool GetSnapToTracks() const { return m_snapToTracks; }
    bool GetSnapToPads() const { return m_snapToPads; }

private:
    bool m_shoveVias;
    bool m_startDiagonal;
    bool m_removeLoops;
    bool m_smartPads;
    bool m_suggestFinish;
    bool m_followMouse;
    bool m_jumpOverObstacles;
    bool m_smoothDraggedSegments;
    bool m_canViolateDRC;
    bool m_freeAngleMode;
    bool m_inlineDragEnabled;
    bool m_snapToTracks;
    bool m_snapToPads;

    PNS_MODE m_routingMode;
    PNS_OPTIMIZATION_EFFORT m_optimizerEffort;

    int m_walkaroundIterationLimit;
    int m_shoveIterationLimit;
    TIME_LIMIT m_shoveTimeLimit;
    TIME_LIMIT m_walkaroundTimeLimit;
};

}

#endif

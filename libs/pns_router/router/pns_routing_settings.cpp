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


#include <geometry/direction45.h>

#include "pns_routing_settings.h"

namespace PNS {

ROUTING_SETTINGS::ROUTING_SETTINGS()
{
    m_routingMode = RM_Walkaround;
    m_optimizerEffort = OE_MEDIUM;
    m_removeLoops = true;
    m_smartPads = true;
    m_shoveVias = true;
    m_suggestFinish = false;
    m_followMouse = true;
    m_startDiagonal = false;
    m_shoveIterationLimit = 250;
    m_shoveTimeLimit = 1000;
    m_walkaroundIterationLimit = 40;
    m_jumpOverObstacles = false;
    m_smoothDraggedSegments = true;
    m_canViolateDRC = false;
    m_freeAngleMode = false;
    m_inlineDragEnabled = false;
}


const DIRECTION_45 ROUTING_SETTINGS::InitialDirection() const
{
    if( m_startDiagonal )
        return DIRECTION_45( DIRECTION_45::NE );
    else
        return DIRECTION_45( DIRECTION_45::N );
}


TIME_LIMIT ROUTING_SETTINGS::ShoveTimeLimit() const
{
    return TIME_LIMIT ( m_shoveTimeLimit );
}


int ROUTING_SETTINGS::ShoveIterationLimit() const
{
    return m_shoveIterationLimit;
}

}

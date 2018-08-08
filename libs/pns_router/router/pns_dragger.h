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

#ifndef __PNS_DRAGGER_H
#define __PNS_DRAGGER_H

#include <math/vector2d.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_algo_base.h"
#include "pns_itemset.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;

/**
 * Class DRAGGER
 *
 * Via, segment and corner dragging algorithm.
 */
class DRAGGER : public ALGO_BASE
{
public:
     DRAGGER( ROUTER* aRouter );
    ~DRAGGER();

    /**
     * Function SetWorld()
     *
     * Sets the board to work on.
     */
    void SetWorld( NODE* aWorld );

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor
     * (unless NULL). Returns true if a dragging operation has started.
     */
    bool Start( const VECTOR2I& aP, ITEM* aStartItem );

    /**
     * Function Drag()
     *
     * Drags the current segment/corner/via to the point aP.
     * @return true, if dragging finished with success.
     */
    bool Drag( const VECTOR2I& aP );

    /**
     * Function FixRoute()
     *
     * Checks if the result of current dragging operation is correct
     * and eventually commits it to the world.
     * @return true, if dragging finished with success.
     */
    bool FixRoute();

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state, including all
     * items changed due to dragging operation.
     */
    NODE* CurrentNode() const;

    /**
     * Function Traces()
     *
     * Returns the set of dragged items.
     */
    const ITEM_SET Traces();

    /// @copydoc ALGO_BASE::Logger()
    virtual LOGGER* Logger() override;

    void SetMode( int aDragMode );

private:


    bool dragMarkObstacles( const VECTOR2I& aP );
    bool dragShove(const VECTOR2I& aP );
    bool startDragSegment( const VECTOR2D& aP, SEGMENT* aSeg );
    bool startDragVia( const VECTOR2D& aP, VIA* aVia );
    void dumbDragVia( VIA* aVia, NODE* aNode, const VECTOR2I& aP );

    NODE*    m_world;
    NODE*    m_lastNode;
    int      m_mode;
    LINE     m_draggedLine;
    VIA*     m_draggedVia;
    LINE     m_lastValidDraggedLine;
    SHOVE*   m_shove;
    int      m_draggedSegmentIndex;
    bool     m_dragStatus;
    PNS_MODE m_currentMode;
    ITEM_SET m_origViaConnections;
    VIA*     m_initialVia;
    ITEM_SET m_draggedItems;
    bool     m_freeAngleMode;
};

}

#endif

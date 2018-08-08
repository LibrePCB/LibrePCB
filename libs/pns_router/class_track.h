/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_track.h
 * @brief Definitions for tracks, vias and zones.
 */

#ifndef CLASS_TRACK_H
#define CLASS_TRACK_H

/*
#include <pcbnew.h>
#include <class_board_item.h>
#include <class_board_connected_item.h>
#include <PolyLine.h>
#include <trigo.h>
*/

class TRACK;
class VIA;
class D_PAD;
class MSG_PANEL_ITEM;


// Via types
// Note that this enum must be synchronized to GAL_LAYER_ID
enum VIATYPE_T
{
    VIA_THROUGH      = 3,      /* Always a through hole via */
    VIA_BLIND_BURIED = 2,      /* this via can be on internal layers */
    VIA_MICROVIA     = 1,      /* this via which connect from an external layer
                                * to the near neighbor internal layer */
    VIA_NOT_DEFINED  = 0       /* not yet used */
};

#define UNDEFINED_DRILL_DIAMETER  -1       //< Undefined via drill diameter.

#define MIN_VIA_DRAW_SIZE          4       /// Minimum size in pixel for full drawing

/**
 * Function GetTrack
 * is a helper function to locate a trace segment having an end point at \a aPosition
 * on \a aLayerMask starting at \a aStartTrace and end at \a aEndTrace.
 * <p>
 * The segments of track that are flagged as deleted or busy are ignored.  Layer
 * visibility is also ignored.
 * </p>
 * @param aStartTrace A pointer to the TRACK object to begin searching.
 * @param aEndTrace A pointer to the TRACK object to stop the search.  A NULL value
 *                  searches to the end of the list.
 * @param aPosition A wxPoint object containing the position to test.
 * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
 *                   layer mask.
 * @return A TRACK object pointer if found otherwise NULL.
 */


#endif // CLASS_TRACK_H

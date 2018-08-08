/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Christian Gagneraud <chgans@gna.org>
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

#ifndef __PNS_DEBUG_DECORATOR_H
#define __PNS_DEBUG_DECORATOR_H

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

namespace PNS {

class DEBUG_DECORATOR
{
public:
    DEBUG_DECORATOR()
    {}

    virtual ~DEBUG_DECORATOR()
    {}

    virtual void AddPoint( VECTOR2I aP, int aColor ) {};
    virtual void AddLine( const SHAPE_LINE_CHAIN& aLine, int aType = 0, int aWidth = 0 ) {};
    virtual void AddSegment( SEG aS, int aColor ) {};
    virtual void AddBox( BOX2I aB, int aColor ) {};
    virtual void AddDirections( VECTOR2D aP, int aMask, int aColor ) {};
    virtual void Clear() {};
};

}

#endif

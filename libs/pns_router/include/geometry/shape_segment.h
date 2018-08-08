/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_SEGMENT_H
#define __SHAPE_SEGMENT_H

#include <geometry/shape.h>
#include <geometry/seg.h>

class SHAPE_SEGMENT : public SHAPE {

public:
    SHAPE_SEGMENT():
        SHAPE( SH_SEGMENT ), m_width( 0 ) {};

    SHAPE_SEGMENT( const VECTOR2I& aA, const VECTOR2I& aB, int aWidth = 0 ):
        SHAPE( SH_SEGMENT ), m_seg( aA, aB ), m_width( aWidth ) {};

    SHAPE_SEGMENT( const SEG& aSeg, int aWidth = 0 ):
        SHAPE( SH_SEGMENT ), m_seg( aSeg ), m_width( aWidth ) {};

    ~SHAPE_SEGMENT() {};

    SHAPE* Clone() const override
    {
        return new SHAPE_SEGMENT( m_seg, m_width );
    }

    const BOX2I BBox( int aClearance = 0 ) const override
    {
        return BOX2I( m_seg.A, m_seg.B - m_seg.A ).Inflate( aClearance + ( m_width + 1 ) / 2 );
    }

    bool Collide( const SEG& aSeg, int aClearance = 0 ) const override
    {
        return m_seg.Distance( aSeg ) < ( m_width + 1 ) / 2 + aClearance;
    }

    bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const override
    {
        return m_seg.Distance( aP ) < ( m_width + 1 ) / 2 + aClearance;
    }

    void SetSeg( const SEG& aSeg )
    {
        m_seg = aSeg;
    }

    const SEG& GetSeg() const
    {
        return m_seg;
    }

    void SetWidth( int aWidth )
    {
        m_width = aWidth;
    }

    int GetWidth() const
    {
        return m_width;
    }

    bool IsSolid() const override
    {
        return true;
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_seg.A += aVector;
        m_seg.B += aVector;
    }

private:
    SEG m_seg;
    int m_width;
};

#endif

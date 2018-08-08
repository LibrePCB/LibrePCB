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

#ifndef __SHAPE_CIRCLE_H
#define __SHAPE_CIRCLE_H

#include "shape.h"

class SHAPE_CIRCLE : public SHAPE
{
public:
    SHAPE_CIRCLE() :
        SHAPE( SH_CIRCLE ), m_radius( 0 )
    {}

    SHAPE_CIRCLE( const VECTOR2I& aCenter, int aRadius ) :
        SHAPE( SH_CIRCLE ), m_radius( aRadius ), m_center( aCenter )
    {}

    SHAPE_CIRCLE( const SHAPE_CIRCLE& aOther ) :
        SHAPE( SH_CIRCLE ),
        m_radius( aOther.m_radius ),
        m_center( aOther.m_center )
    {};

    ~SHAPE_CIRCLE()
    {}

    SHAPE* Clone() const override
    {
        return new SHAPE_CIRCLE( *this );
    }

    const BOX2I BBox( int aClearance = 0 ) const override
    {
        const VECTOR2I rc( m_radius + aClearance, m_radius + aClearance );

        return BOX2I( m_center - rc, rc * 2 );
    }

    bool Collide( const SEG& aSeg, int aClearance = 0 ) const override
    {
        int rc = aClearance + m_radius;

        return aSeg.Distance( m_center ) < rc;
    }

    void SetRadius( int aRadius )
    {
        m_radius = aRadius;
    }

    void SetCenter( const VECTOR2I& aCenter )
    {
        m_center = aCenter;
    }

    int GetRadius() const
    {
        return m_radius;
    }

    const VECTOR2I GetCenter() const
    {
        return m_center;
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_center += aVector;
    }

    bool IsSolid() const override
    {
        return true;
    }
private:
    int m_radius;
    VECTOR2I m_center;
};

#endif

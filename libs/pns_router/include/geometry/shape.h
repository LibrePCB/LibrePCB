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

#ifndef __SHAPE_H
#define __SHAPE_H

#include <string>
#include <sstream>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/seg.h>

/**
 * Enum SHAPE_TYPE
 * Lists all supported shapes
 */

enum SHAPE_TYPE
{
    SH_RECT = 0,        ///> axis-aligned rectangle
    SH_SEGMENT,         ///> line segment
    SH_LINE_CHAIN,      ///> line chain (polyline)
    SH_CIRCLE,          ///> circle
    SH_SIMPLE,          ///> simple polygon
    SH_POLY_SET,        ///> set of polygons (with holes, etc.)
    SH_COMPOUND,        ///> compound shape, consisting of multiple simple shapes
    SH_ARC              ///> circular arc
};

/**
 * Class SHAPE
 *
 * Represents an abstract shape on 2D plane.
 */
class SHAPE
{
protected:
    typedef VECTOR2I::extended_type ecoord;

public:
    /**
     * Constructor
     *
     * Creates an empty shape of type aType
     */

    SHAPE( SHAPE_TYPE aType ) : m_type( aType )
    {}

    // Destructor
    virtual ~SHAPE()
    {}

    /**
     * Function Type()
     *
     * Returns the type of the shape.
     * @retval the type
     */
    SHAPE_TYPE Type() const
    {
        return m_type;
    }

    /**
     * Function Clone()
     *
     * Returns a dynamically allocated copy of the shape
     * @retval copy of the shape
     */
    virtual SHAPE* Clone() const
    {
        assert( false );
        return NULL;
    };

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the point aP than aClearance,
     * indicating a collision.
     * @return true, if there is a collision.
     */
    virtual bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const
    {
        return Collide( SEG( aP, aP ), aClearance );
    }

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the shape aShape than aClearance,
     * indicating a collision.
     * @param aShape shape to check collision against
     * @param aClearance minimum clearance
     * @param aMTV minimum translation vector
     * @return true, if there is a collision.
     */
    virtual bool Collide( const SHAPE* aShape, int aClearance, VECTOR2I& aMTV ) const;
    virtual bool Collide( const SHAPE* aShape, int aClearance = 0 ) const;

    /**
     * Function Collide()
     *
     * Checks if the boundary of shape (this) lies closer to the segment aSeg than aClearance,
     * indicating a collision.
     * @return true, if there is a collision.
     */
    virtual bool Collide( const SEG& aSeg, int aClearance = 0 ) const = 0;

    /**
     * Function BBox()
     *
     * Computes a bounding box of the shape, with a margin of aClearance
     * a collision.
     * @param aClearance how much the bounding box is expanded wrs to the minimum enclosing rectangle
     * for the shape.
     * @return the bounding box.
     */
    virtual const BOX2I BBox( int aClearance = 0 ) const = 0;

    /**
     * Function Centre()
     *
     * Computes a center-of-mass of the shape
     * @return the center-of-mass point
     */
    virtual VECTOR2I Centre() const
    {
        return BBox( 0 ).Centre(); // if nothing better is available....
    }

    virtual void Move ( const VECTOR2I& aVector ) = 0;

    virtual bool IsSolid() const = 0;

    virtual bool Parse( std::stringstream& aStream );

    virtual const std::string Format( ) const;

protected:
    ///> type of our shape
    SHAPE_TYPE m_type;
};

bool CollideShapes( const SHAPE* aA, const SHAPE* aB, int aClearance,
                    bool aNeedMTV, VECTOR2I& aMTV );

#endif // __SHAPE_H

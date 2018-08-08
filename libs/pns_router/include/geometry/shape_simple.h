/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Kicad Developers, see change_log.txt for contributors.
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

#ifndef __SHAPE_SIMPLE_H
#define __SHAPE_SIMPLE_H

#include <vector>

#include <geometry/shape.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

/**
 * Class SHAPE_SIMPLE
 *
 * Represents a simple polygon consisting of a zero-thickness closed chain of
 * connected line segments.
 *
 * Internally the vertices are held in a SHAPE_LINE_CHAIN, please note that
 * there is a "virtual" line segment between the last and first vertex.
 */
class SHAPE_SIMPLE : public SHAPE
{
public:
    /**
     * Constructor
     * Creates an empty polygon
     */
    SHAPE_SIMPLE() :
        SHAPE( SH_SIMPLE )
    {
        m_points.SetClosed( true );
    }

    SHAPE_SIMPLE( const SHAPE_SIMPLE& aOther ) :
       SHAPE( SH_SIMPLE ), m_points( aOther.m_points )
    {}

    SHAPE* Clone() const override
    {
        return new SHAPE_SIMPLE( *this );
    }

    /**
     * Function Clear()
     * Removes all points from the polygon.
     */
    void Clear()
    {
        m_points.Clear();
    }

    /// @copydoc SHAPE::BBox()
    const BOX2I BBox( int aClearance = 0 ) const override
    {
        return m_points.BBox( aClearance );
    }

    /**
     * Function PointCount()
     *
     * Returns the number of points (vertices) in this polygon
     * @return number of points
     */
    int PointCount() const
    {
        return m_points.PointCount();
    }

    /**
     * Function Point()
     *
     * Returns a reference to a given point in the polygon. Negative indices
     * count from the end of the point list, e.g. -1 means "last point", -2
     * means "second to last point" and so on.
     * @param aIndex index of the point
     * @return reference to the point
     */
    VECTOR2I& Point( int aIndex )
    {
        return m_points.Point( aIndex );
    }

    /**
     * Function CPoint()
     *
     * Returns a const reference to a given point in the polygon. Negative
     * indices count from the end of the point list, e.g. -1 means "last
     * point", -2 means "second to last point" and so on.
     * @param aIndex index of the point
     * @return const reference to the point
     */
    const VECTOR2I& CPoint( int aIndex ) const
    {
        return m_points.CPoint( aIndex );
    }

    /**
     * Function CDPoint()
     *
     * Returns a given point as a vector with elements of type double.
     *
     * @param aIndex index of the point
     * @return the point with elements of type double
     */
    const VECTOR2D CDPoint( int aIndex ) const
    {
        const VECTOR2I& v = CPoint( aIndex );
        return VECTOR2D( v.x, v.y );
    }

    /**
     * Function Vertices()
     *
     * Returns the list of vertices defining this simple polygon.
     *
     * @return the list of vertices defining this simple polygon
     */
    const SHAPE_LINE_CHAIN& Vertices() const
    {
        return m_points;
    }

    /**
     * Function Append()
     *
     * Appends a new point at the end of the polygon.
     * @param aX is X coordinate of the new point
     * @param aY is Y coordinate of the new point
     */
    void Append( int aX, int aY )
    {
        VECTOR2I v( aX, aY );
        Append( v );
    }

    /**
     * Function Append()
     *
     * Appends a new point at the end of the polygon.
     * @param aP the new point
     */
    void Append( const VECTOR2I& aP )
    {
        m_points.Append( aP );
    }

    /// @copydoc SHAPE::Collide()
    bool Collide( const SEG& aSeg, int aClearance = 0 ) const override
    {
        return m_points.Collide( aSeg, aClearance );
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_points.Move( aVector );
    }

    bool IsSolid() const override
    {
        return true;
    }

private:
    // vertices
    SHAPE_LINE_CHAIN m_points;
};

#endif // __SHAPE_SIMPLE_H

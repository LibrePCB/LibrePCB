/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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

#ifndef __BOX2_H
#define __BOX2_H

#include <math/vector2d.h>
#include <limits>

#include <core/optional.h>

/**
 * Class BOX2
 * handles a 2-D bounding box, built on top of an origin point
 * and size vector, both of templated class Vec
 */
template <class Vec>
class BOX2
{
private:
    Vec m_Pos;      // Rectangle Origin
    Vec m_Size;     // Rectangle Size

public:
    typedef typename Vec::coord_type                 coord_type;
    typedef typename Vec::extended_type              ecoord_type;
    typedef std::numeric_limits<coord_type>          coord_limits;

    BOX2() {};

    BOX2( const Vec& aPos, const Vec& aSize ) :
        m_Pos( aPos ),
        m_Size( aSize )
    {
        Normalize();
    }

#ifdef WX_COMPATIBILITY
    /// Constructor with a wxRect as argument
    BOX2( const wxRect& aRect ) :
        m_Pos( aRect.GetPosition() ),
        m_Size( aRect.GetSize() )
    {
        Normalize();
    }
#endif

    void SetMaximum()
    {
        m_Pos.x  = m_Pos.y = coord_limits::lowest() / 2 + coord_limits::epsilon();
        m_Size.x = m_Size.y = coord_limits::max() - coord_limits::epsilon();
    }

    Vec Centre() const
    {
        return Vec( m_Pos.x + ( m_Size.x / 2 ),
                    m_Pos.y + ( m_Size.y / 2 ) );
    }

    /**
     * @brief Compute the bounding box from a given list of points.
     *
     * @param aPointList is the list points of the object.
     */
    template <class Container>
    void Compute( const Container& aPointList )
    {
        Vec vmin, vmax;

        typename Container::const_iterator i;

        if( !aPointList.size() )
            return;

        vmin = vmax = aPointList[0];

        for( i = aPointList.begin(); i != aPointList.end(); ++i )
        {
            Vec p( *i );
            vmin.x  = std::min( vmin.x, p.x );
            vmin.y  = std::min( vmin.y, p.y );
            vmax.x  = std::max( vmax.x, p.x );
            vmax.y  = std::max( vmax.y, p.y );
        }

        SetOrigin( vmin );
        SetSize( vmax - vmin );
    }

    /**
     * Function Move
     * moves the rectangle by the \a aMoveVector.
     * @param aMoveVector A point that is the value to move this rectangle
     */
    void Move( const Vec& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Normalize
     * ensures that the height ant width are positive.
     */
    BOX2<Vec>& Normalize()
    {
        if( m_Size.y < 0 )
        {
            m_Size.y = -m_Size.y;
            m_Pos.y -= m_Size.y;
        }

        if( m_Size.x < 0 )
        {
            m_Size.x = -m_Size.x;
            m_Pos.x -= m_Size.x;
        }

        return *this;
    }

    /**
     * Function Contains
     * @param aPoint = the point to test
     * @return true if aPoint is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( const Vec& aPoint ) const
    {
        Vec rel_pos = aPoint - m_Pos;
        Vec size    = m_Size;

        if( size.x < 0 )
        {
            size.x    = -size.x;
            rel_pos.x += size.x;
        }

        if( size.y < 0 )
        {
            size.y    = -size.y;
            rel_pos.y += size.y;
        }

        return ( rel_pos.x >= 0 ) && ( rel_pos.y >= 0 ) && ( rel_pos.y <= size.y) && ( rel_pos.x <= size.x);
    }

    /**
     * Function Contains
     * @param x = the x coordinate of the point to test
     * @param y = the x coordinate of the point to test
     * @return true if point is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( coord_type x, coord_type y ) const { return Contains( Vec( x, y ) ); }

    /**
     * Function Contains
     * @param aRect = the BOX2 to test
     * @return true if aRect is Contained. A common edge is seen as contained
     */
    bool Contains( const BOX2<Vec>& aRect ) const
    {
        return Contains( aRect.GetOrigin() ) && Contains( aRect.GetEnd() );
    }

    const Vec& GetSize() const { return m_Size; }
    coord_type GetX() const { return m_Pos.x; }
    coord_type GetY() const { return m_Pos.y; }

    const Vec& GetOrigin() const { return m_Pos; }
    const Vec& GetPosition() const { return m_Pos; }
    const Vec GetEnd() const { return Vec( GetRight(), GetBottom() ); }

    coord_type GetWidth() const { return m_Size.x; }
    coord_type GetHeight() const { return m_Size.y; }
    coord_type GetRight() const { return m_Pos.x + m_Size.x; }
    coord_type GetBottom() const { return m_Pos.y + m_Size.y; }

    // Compatibility aliases
    coord_type GetLeft() const { return GetX(); }
    coord_type GetTop() const { return GetY(); }
    void MoveTopTo( coord_type aTop ) { m_Pos.y = aTop; }
    void MoveBottomTo( coord_type aBottom ) { m_Size.y = aBottom - m_Pos.y; }
    void MoveLeftTo( coord_type aLeft ) { m_Pos.x = aLeft; }
    void MoveRightTo( coord_type aRight ) { m_Size.x = aRight - m_Pos.x; }

    void SetOrigin( const Vec& pos ) { m_Pos = pos; }
    void SetOrigin( coord_type x, coord_type y ) { m_Pos.x = x; m_Pos.y = y; }
    void SetSize( const Vec& size ) { m_Size = size; }
    void SetSize( coord_type w, coord_type h ) { m_Size.x = w; m_Size.y = h; }
    void Offset( coord_type dx, coord_type dy ) { m_Pos.x += dx; m_Pos.y += dy; }
    void Offset( const Vec& offset )
    {
        m_Pos.x += offset.x; m_Pos.y +=
            offset.y;
    }

    void SetX( coord_type val ) { m_Pos.x = val; }
    void SetY( coord_type val ) { m_Pos.y = val; }
    void SetWidth( coord_type val ) { m_Size.x = val; }
    void SetHeight( coord_type val ) { m_Size.y = val; }
    void SetEnd( coord_type x, coord_type y ) { SetEnd( Vec( x, y ) ); }
    void SetEnd( const Vec& pos )
    {
        m_Size.x = pos.x - m_Pos.x; m_Size.y = pos.y - m_Pos.y;
    }

    /**
     * Function Intersects
     * @return bool - true if the argument rectangle intersects this rectangle.
     * (i.e. if the 2 rectangles have at least a common point)
     */
    bool Intersects( const BOX2<Vec>& aRect ) const
    {
        // this logic taken from wxWidgets' geometry.cpp file:
        bool        rc;

        BOX2<Vec>   me( *this );
        BOX2<Vec>   rect( aRect );
        me.Normalize();         // ensure size is >= 0
        rect.Normalize();       // ensure size is >= 0

        // calculate the left common area coordinate:
        int  left   = std::max( me.m_Pos.x, rect.m_Pos.x );
        // calculate the right common area coordinate:
        int  right  = std::min( me.m_Pos.x + me.m_Size.x, rect.m_Pos.x + rect.m_Size.x );
        // calculate the upper common area coordinate:
        int  top    = std::max( me.m_Pos.y, aRect.m_Pos.y );
        // calculate the lower common area coordinate:
        int  bottom = std::min( me.m_Pos.y + me.m_Size.y, rect.m_Pos.y + rect.m_Size.y );

        // if a common area exists, it must have a positive (null accepted) size
        if( left <= right && top <= bottom )
            rc = true;
        else
            rc = false;

        return rc;
    }

    /**
     * Function Intersect
     * Returns the intersection of this with another rectangle.
     */
    BOX2<Vec> Intersect( const BOX2<Vec>& aRect )
    {
        BOX2<Vec> me( *this );
        BOX2<Vec> rect( aRect );
        me.Normalize();         // ensure size is >= 0
        rect.Normalize();       // ensure size is >= 0

        Vec topLeft, bottomRight;

        topLeft.x     = std::max( me.m_Pos.x, rect.m_Pos.x );
        bottomRight.x = std::min( me.m_Pos.x + me.m_Size.x, rect.m_Pos.x + rect.m_Size.x );
        topLeft.y     = std::max( me.m_Pos.y, rect.m_Pos.y );
        bottomRight.y = std::min( me.m_Pos.y + me.m_Size.y, rect.m_Pos.y + rect.m_Size.y );

        if ( topLeft.x < bottomRight.x && topLeft.y < bottomRight.y )
            return BOX2<Vec>( topLeft, bottomRight - topLeft );
        else
            return BOX2<Vec>( Vec( 0, 0 ), Vec( 0, 0 ) );
    }

    const std::string Format() const
    {
        std::stringstream ss;

        ss << "( box corner " << m_Pos.Format() << " w " << m_Size.x << " h " << m_Size.y << " )";

        return ss.str();
    }

    /**
     * Function Inflate
     * inflates the rectangle horizontally by \a dx and vertically by \a dy. If \a dx
     * and/or \a dy is negative the rectangle is deflated.
     */
    BOX2<Vec>& Inflate( coord_type dx, coord_type dy )
    {
        if( m_Size.x >= 0 )
        {
            if( m_Size.x < -2 * dx )
            {
                // Don't allow deflate to eat more width than we have,
                m_Pos.x += m_Size.x / 2;
                m_Size.x = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.x  -= dx;
                m_Size.x += 2 * dx;
            }
        }
        else    // size.x < 0:
        {
            if( m_Size.x > -2 * dx )
            {
                // Don't allow deflate to eat more width than we have,
                m_Pos.x -= m_Size.x / 2;
                m_Size.x = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.x  += dx;
                m_Size.x -= 2 * dx; // m_Size.x <0: inflate when dx > 0
            }
        }

        if( m_Size.y >= 0 )
        {
            if( m_Size.y < -2 * dy )
            {
                // Don't allow deflate to eat more height than we have,
                m_Pos.y += m_Size.y / 2;
                m_Size.y = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.y  -= dy;
                m_Size.y += 2 * dy;
            }
        }
        else    // size.y < 0:
        {
            if( m_Size.y > 2 * dy )
            {
                // Don't allow deflate to eat more height than we have,
                m_Pos.y -= m_Size.y / 2;
                m_Size.y = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.y  += dy;
                m_Size.y -= 2 * dy; // m_Size.y <0: inflate when dy > 0
            }
        }

        return *this;
    }

    /**
     * Function Inflate
     * inflates the rectangle horizontally and vertically by \a aDelta. If \a aDelta
     * is negative the rectangle is deflated.
     */
    BOX2<Vec>& Inflate( int aDelta )
    {
        Inflate( aDelta, aDelta );
        return *this;
    }

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain \a aRect.  It is
     * mainly used to calculate bounding boxes.
     * @param aRect  The rectangle to merge with this rectangle.
     */
    BOX2<Vec>& Merge( const BOX2<Vec>& aRect )
    {
        Normalize();        // ensure width and height >= 0
        BOX2<Vec> rect = aRect;
        rect.Normalize();   // ensure width and height >= 0
        Vec  end = GetEnd();
        Vec  rect_end = rect.GetEnd();

        // Change origin and size in order to contain the given rect
        m_Pos.x = std::min( m_Pos.x, rect.m_Pos.x );
        m_Pos.y = std::min( m_Pos.y, rect.m_Pos.y );
        end.x   = std::max( end.x, rect_end.x );
        end.y   = std::max( end.y, rect_end.y );
        SetEnd( end );
        return *this;
    }

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain the given point.
     * @param aPoint The point to merge with the rectangle.
     */
    BOX2<Vec>& Merge( const Vec& aPoint )
    {
        Normalize();        // ensure width and height >= 0

        Vec end = GetEnd();
        // Change origin and size in order to contain the given rect
        m_Pos.x = std::min( m_Pos.x, aPoint.x );
        m_Pos.y = std::min( m_Pos.y, aPoint.y );
        end.x   = std::max( end.x, aPoint.x );
        end.y   = std::max( end.y, aPoint.y );
        SetEnd( end );
        return *this;
    }

    /**
     * Function GetArea
     * returns the area of the rectangle.
     * @return The area of the rectangle.
     */
    ecoord_type GetArea() const
    {
        return (ecoord_type) GetWidth() * (ecoord_type) GetHeight();
    }

    /**
     * Function GetArea
     * returns the length of the diagonal of the rectangle.
     * @return The area of the diagonal.
     */
    ecoord_type Diagonal() const
    {
        return m_Size.EuclideanNorm();
    }

    ecoord_type SquaredDistance( const Vec& aP ) const
    {
        ecoord_type x2 = m_Pos.x + m_Size.x;
        ecoord_type y2 = m_Pos.y + m_Size.y;
        ecoord_type xdiff = std::max( aP.x < m_Pos.x ? m_Pos.x - aP.x : m_Pos.x - x2, (ecoord_type) 0 );
        ecoord_type ydiff = std::max( aP.y < m_Pos.y ? m_Pos.y - aP.y : m_Pos.y - y2, (ecoord_type) 0 );
        return xdiff * xdiff + ydiff * ydiff;
    }

    ecoord_type Distance( const Vec& aP ) const
    {
        return sqrt( SquaredDistance( aP ) );
    }

    /**
     * Function SquaredDistance
     * returns the square of the minimum distance between self and box aBox
     * @param aBox: the other box
     * @return The distance, squared
     */
    ecoord_type SquaredDistance( const BOX2<Vec>& aBox ) const
    {
        ecoord_type s = 0;

        if( aBox.m_Pos.x + aBox.m_Size.x < m_Pos.x )
        {
            ecoord_type d = aBox.m_Pos.x + aBox.m_Size.x - m_Pos.x;
            s += d * d;
        }
        else if( aBox.m_Pos.x > m_Pos.x + m_Size.x )
        {
            ecoord_type d = aBox.m_Pos.x - m_Size.x - m_Pos.x;
            s += d * d;
        }

        if( aBox.m_Pos.y + aBox.m_Size.y < m_Pos.y )
        {
            ecoord_type d = aBox.m_Pos.y + aBox.m_Size.y - m_Pos.y;
            s += d * d;
        }
        else if( aBox.m_Pos.y > m_Pos.y + m_Size.y )
        {
            ecoord_type d = aBox.m_Pos.y - m_Size.y - m_Pos.y;
            s += d * d;
        }

        return s;
    }

    /**
     * Function Distance
     * returns the minimum distance between self and box aBox
     * @param aBox: the other box
     * @return The distance
     */
    ecoord_type Distance( const BOX2<Vec>& aBox ) const
    {
        return sqrt( SquaredDistance( aBox ) );
    }
};

/* Default specializations */
typedef BOX2<VECTOR2I>    BOX2I;
typedef BOX2<VECTOR2D>    BOX2D;

typedef OPT<BOX2I> OPT_BOX2I;

// FIXME should be removed to avoid multiple typedefs for the same type
typedef BOX2D             DBOX;

#endif

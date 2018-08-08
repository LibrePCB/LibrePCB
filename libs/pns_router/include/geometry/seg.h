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

#ifndef __SEG_H
#define __SEG_H

#include <cstdio>
#include <climits>

#include <math/vector2d.h>
#include <core/optional.h>

typedef OPT<VECTOR2I> OPT_VECTOR2I;

class SEG
{
private:
    typedef VECTOR2I::extended_type ecoord;

public:
    friend inline std::ostream& operator<<( std::ostream& aStream, const SEG& aSeg );

    /* Start and the of the segment. Public, to make access simpler.
     */
    VECTOR2I A;
    VECTOR2I B;

    /** Default constructor
     * Creates an empty (0, 0) segment
     */
    SEG()
    {
        m_index = -1;
    }

    /**
     * Constructor
     * Creates a segment between (aX1, aY1) and (aX2, aY2)
     */
    SEG( int aX1, int aY1, int aX2, int aY2 ) :
        A ( VECTOR2I( aX1, aY1 ) ),
        B ( VECTOR2I( aX2, aY2 ) )
    {
        m_index = -1;
    }

    /**
     * Constructor
     * Creates a segment between (aA) and (aB)
     */
    SEG( const VECTOR2I& aA, const VECTOR2I& aB ) : A( aA ), B( aB )
    {
        m_index = -1;
    }

    /**
     * Constructor
     * Creates a segment between (aA) and (aB), referenced to a multi-segment shape
     * @param aA reference to the start point in the parent shape
     * @param aB reference to the end point in the parent shape
     * @param aIndex index of the segment within the parent shape
     */
    SEG( const VECTOR2I& aA, const VECTOR2I& aB, int aIndex ) : A( aA ), B( aB )
    {
        m_index = aIndex;
    }

    /**
     * Copy constructor
     */
    SEG( const SEG& aSeg ) : A( aSeg.A ), B( aSeg.B ), m_index( aSeg.m_index )
    {
    }

    SEG& operator=( const SEG& aSeg )
    {
        A = aSeg.A;
        B = aSeg.B;
        m_index = aSeg.m_index;

        return *this;
    }

    bool operator==( const SEG& aSeg ) const
    {
        return (A == aSeg.A && B == aSeg.B) ;
    }

    bool operator!=( const SEG& aSeg ) const
    {
        return (A != aSeg.A || B != aSeg.B);
    }

    /**
      * Function LineProject()
      *
      * Computes the perpendicular projection point of aP on a line passing through
      * ends of the segment.
      * @param aP point to project
      * @return projected point
      */
    VECTOR2I LineProject( const VECTOR2I& aP ) const;

    /**
      * Function Side()
      *
      * Determines on which side of directed line passing via segment ends point aP lies.
      * @param aP point to determine the orientation wrs to self
      * @return: < 0: left, 0 : on the line, > 0 : right
      */
    int Side( const VECTOR2I& aP ) const
    {
        const ecoord det = ( B - A ).Cross( aP - A );

        return det < 0 ? -1 : ( det > 0 ? 1 : 0 );
    }

    /**
      * Function LineDistance()
      *
      * Returns the closest Euclidean distance between point aP and the line defined by
      * the ends of segment (this).
      * @param aP the point to test
      * @param aDetermineSide: when true, the sign of the returned value indicates
      * the side of the line at which we are (negative = left)
      * @return the distance
      */
    int LineDistance( const VECTOR2I& aP, bool aDetermineSide = false ) const;

    /**
      * Function NearestPoint()
      *
      * Computes a point on the segment (this) that is closest to point aP.
      * @return the nearest point
      */
    const VECTOR2I NearestPoint( const VECTOR2I &aP ) const;

    /**
     * Function Intersect()
     *
     * Computes intersection point of segment (this) with segment aSeg.
     * @param aSeg: segment to intersect with
     * @param aIgnoreEndpoints: don't treat corner cases (i.e. end of one segment touching the
     * other) as intersections.
     * @param aLines: treat segments as infinite lines
     * @return intersection point, if exists
     */
    OPT_VECTOR2I Intersect( const SEG& aSeg, bool aIgnoreEndpoints = false,
                            bool aLines = false ) const;

    /**
     * Function IntersectLines()
     *
     * Computes the intersection point of lines passing through ends of (this) and aSeg
     * @param aSeg segment defining the line to intersect with
     * @return intersection point, if exists
     */
    OPT_VECTOR2I IntersectLines( const SEG& aSeg ) const
    {
        return Intersect( aSeg, false, true );
    }

    bool Collide( const SEG& aSeg, int aClearance ) const;

    ecoord SquaredDistance( const SEG& aSeg ) const;

    /**
     * Function Distance()
     *
     * Computes minimum Euclidean distance to segment aSeg.
     * @param aSeg other segment
     * @return minimum distance
     */
    int Distance( const SEG& aSeg ) const
    {
        return sqrt( SquaredDistance( aSeg ) );
    }

    ecoord SquaredDistance( const VECTOR2I& aP ) const
    {
        return ( NearestPoint( aP ) - aP ).SquaredEuclideanNorm();
    }

    /**
     * Function Distance()
     *
     * Computes minimum Euclidean distance to point aP.
     * @param aP the point
     * @return minimum distance
     */
    int Distance( const VECTOR2I& aP ) const
    {
        return sqrt( SquaredDistance( aP ) );
    }

    void CanonicalCoefs( ecoord& qA, ecoord& qB, ecoord& qC ) const
    {
        qA = A.y - B.y;
        qB = B.x - A.x;
        qC = -qA * A.x - qB * A.y;
    }

    /**
     * Function Collinear()
     *
     * Checks if segment aSeg lies on the same line as (this).
     * @param aSeg the segment to chech colinearity with
     * @return true, when segments are collinear.
     */
    bool Collinear( const SEG& aSeg ) const
    {
        ecoord qa, qb, qc;
        CanonicalCoefs( qa, qb, qc );

        ecoord d1 = std::abs( aSeg.A.x * qa + aSeg.A.y * qb + qc );
        ecoord d2 = std::abs( aSeg.B.x * qa + aSeg.B.y * qb + qc );

        return ( d1 <= 1 && d2 <= 1 );
    }

    bool ApproxCollinear( const SEG& aSeg ) const
    {
        ecoord p, q, r;
        CanonicalCoefs( p, q, r );

        ecoord dist1 = ( p * aSeg.A.x + q * aSeg.A.y + r ) / sqrt( p * p + q * q );
        ecoord dist2 = ( p * aSeg.B.x + q * aSeg.B.y + r ) / sqrt( p * p + q * q );

        return std::abs( dist1 ) <= 1 && std::abs( dist2 ) <= 1;
    }

    bool ApproxParallel ( const SEG& aSeg ) const
    {
        ecoord p, q, r;
        CanonicalCoefs( p, q, r );

        ecoord dist1 = ( p * aSeg.A.x + q * aSeg.A.y + r ) / sqrt( p * p + q * q );
        ecoord dist2 = ( p * aSeg.B.x + q * aSeg.B.y + r ) / sqrt( p * p + q * q );

        return std::abs( dist1 - dist2 ) <= 1;
    }


    bool Overlaps( const SEG& aSeg ) const
    {
        if( aSeg.A == aSeg.B ) // single point corner case
        {
            if( A == aSeg.A || B == aSeg.A )
                return false;

            return Contains( aSeg.A );
        }

        if( !Collinear( aSeg ) )
            return false;

        if( Contains( aSeg.A ) || Contains( aSeg.B ) )
            return true;
        if( aSeg.Contains( A ) || aSeg.Contains( B ) )
            return true;

        return false;
    }

    /**
     * Function Length()
     *
     * Returns the length (this)
     * @return length
     */
    int Length() const
    {
        return ( A - B ).EuclideanNorm();
    }

    ecoord SquaredLength() const
    {
        return ( A - B ).SquaredEuclideanNorm();
    }

    ecoord TCoef( const VECTOR2I& aP ) const;

    /**
     * Function Index()
     *
     * Return the index of this segment in its parent shape (applicable only to non-local segments)
     * @return index value
     */
    int Index() const
    {
        return m_index;
    }

    bool Contains( const VECTOR2I& aP ) const;

    bool PointCloserThan( const VECTOR2I& aP, int aDist ) const;

    void Reverse()
    {
        std::swap( A, B );
    }

    ///> Returns the center point of the line
    VECTOR2I Center() const
    {
        return A + ( B - A ) / 2;
    }

private:
    bool ccw( const VECTOR2I& aA, const VECTOR2I& aB, const VECTOR2I &aC ) const;

    ///> index withing the parent shape (used when m_is_local == false)
    int m_index;
};

inline VECTOR2I SEG::LineProject( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    ecoord l_squared = d.Dot( d );

    if( l_squared == 0 )
        return A;

    ecoord t = d.Dot( aP - A );

    int xp = rescale( t, (ecoord)d.x, l_squared );
    int yp = rescale( t, (ecoord)d.y, l_squared );

    return A + VECTOR2I( xp, yp );
}

inline int SEG::LineDistance( const VECTOR2I& aP, bool aDetermineSide ) const
{
    ecoord p = A.y - B.y;
    ecoord q = B.x - A.x;
    ecoord r = -p * A.x - q * A.y;

    ecoord dist = ( p * aP.x + q * aP.y + r ) / sqrt( p * p + q * q );

    return aDetermineSide ? dist : std::abs( dist );
}

inline SEG::ecoord SEG::TCoef( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    return d.Dot( aP - A);
}

inline const VECTOR2I SEG::NearestPoint( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    ecoord l_squared = d.Dot( d );

    if( l_squared == 0 )
        return A;

    ecoord t = d.Dot( aP - A );

    if( t < 0 )
        return A;
    else if( t > l_squared )
        return B;

    int xp = rescale( t, (ecoord)d.x, l_squared );
    int yp = rescale( t, (ecoord)d.y, l_squared );

    return A + VECTOR2I( xp, yp );
}

inline std::ostream& operator<<( std::ostream& aStream, const SEG& aSeg )
{
    aStream << "[ " << aSeg.A << " - " << aSeg.B << " ]";

    return aStream;
}

#endif // __SEG_H

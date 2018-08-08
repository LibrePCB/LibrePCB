/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef DIRECTION45_H
#define DIRECTION45_H

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

// believe or not, X11 headers have a F****ING macro called Opposite...
#undef Opposite

/**
 * Class DIRECTION_45.
 * Represents route directions & corner angles in a 45-degree metric.
 */

class DIRECTION_45
{
public:

    /**
     * Enum Directions
     * Represents available directions - there are 8 of them, as on a rectilinear map (north = up) +
     * an extra undefined direction, reserved for traces that don't respect 45-degree routing regime.
     */
    enum Directions
    {
        N           = 0,
        NE          = 1,
        E           = 2,
        SE          = 3,
        S           = 4,
        SW          = 5,
        W           = 6,
        NW          = 7,
        UNDEFINED   = -1
    };

    /**
     * Enum AngleType
     * Represents kind of angle formed by vectors heading in two DIRECTION_45s.
     */
    enum AngleType
    {
        ANG_OBTUSE      = 0x01,
        ANG_RIGHT       = 0x02,
        ANG_ACUTE       = 0x04,
        ANG_STRAIGHT    = 0x08,
        ANG_HALF_FULL   = 0x10,
        ANG_UNDEFINED   = 0x20
    };

    DIRECTION_45( Directions aDir = UNDEFINED ) : m_dir( aDir ) {}

    /**
     * Constructor
     * @param aVec vector, whose direction will be translated into a DIRECTION_45.
     */
    DIRECTION_45( const VECTOR2I& aVec )
    {
        construct_( aVec );
    }

    /**
     * Constructor
     * @param aSeg segment, whose direction will be translated into a DIRECTION_45.
     */
    DIRECTION_45( const SEG& aSeg )
    {
        construct_( aSeg.B - aSeg.A );
    }

    /**
     * Function Format()
     * Formats the direction in a human readable word.
     * @return name of the direction
     */
    const std::string Format() const
    {
        switch( m_dir )
        {
        case N:
            return "north";

        case NE:
            return "north-east";

        case E:
            return "east";

        case SE:
            return "south-east";

        case S:
            return "south";

        case SW:
            return "south-west";

        case W:
            return "west";

        case NW:
            return "north-west";

        case UNDEFINED:
            return "undefined";

        default:
            return "<Error>";
        }
    }

    /**
     * Function Opposite()
     * Returns a direction opposite (180 degree) to (this)
     * @return opposite direction
     */
    DIRECTION_45 Opposite() const
    {
        const Directions OppositeMap[] = { S, SW, W, NW, N, NE, E, SE, UNDEFINED };
        return OppositeMap[m_dir];
    }

    /**
     * Function Angle()
     * Returns the type of angle between directions (this) and aOther.
     * @param aOther direction to compare angle with
     */
    AngleType Angle( const DIRECTION_45& aOther ) const
    {
        if( m_dir == UNDEFINED || aOther.m_dir == UNDEFINED )
            return ANG_UNDEFINED;

        int d = std::abs( m_dir - aOther.m_dir );

        if( d == 1 || d == 7 )
            return ANG_OBTUSE;
        else if( d == 2 || d == 6 )
            return ANG_RIGHT;
        else if( d == 3 || d == 5 )
            return ANG_ACUTE;
        else if( d == 4 )
            return ANG_HALF_FULL;
        else
            return ANG_STRAIGHT;
    }

    /**
     * Function IsObtuse()
     * @return true, when (this) forms an obtuse angle with aOther
     */
    bool IsObtuse( const DIRECTION_45& aOther ) const
    {
        return Angle( aOther ) == ANG_OBTUSE;
    }

    /**
     * Function IsDiagonal()
     * Returns true if the direction is diagonal (e.g. North-West, South-East, etc)
     * @return true, when diagonal.
     */
    bool IsDiagonal() const
    {
        return ( m_dir % 2 ) == 1;
    }

    bool IsDefined() const
    {
        return m_dir != UNDEFINED;
    }

    /**
     * Function BuildInitialTrace()
     *
     * Builds a 2-segment line chain between points aP0 and aP1 and following 45-degree routing
     * regime. If aStartDiagonal is true, the trace starts with a diagonal segment.
     * @param aP0 starting point
     * @param aP1 ending point
     * @param aStartDiagonal whether the first segment has to be diagonal
     * @return the trace
     */
    const SHAPE_LINE_CHAIN BuildInitialTrace( const VECTOR2I& aP0,
            const VECTOR2I& aP1,
            bool aStartDiagonal = false ) const
    {
        int w = abs( aP1.x - aP0.x );
        int h = abs( aP1.y - aP0.y );
        int sw  = sign( aP1.x - aP0.x );
        int sh  = sign( aP1.y - aP0.y );

        VECTOR2I mp0, mp1;

        // we are more horizontal than vertical?
        if( w > h )
        {
            mp0 = VECTOR2I( ( w - h ) * sw, 0 );    // direction: E
            mp1 = VECTOR2I( h * sw, h * sh );       // direction: NE
        }
        else
        {
            mp0 = VECTOR2I( 0, sh * ( h - w ) );    // direction: N
            mp1 = VECTOR2I( sw * w, sh * w );       // direction: NE
        }

        bool start_diagonal;

        if( m_dir == UNDEFINED )
            start_diagonal = aStartDiagonal;
        else
            start_diagonal = IsDiagonal();

        SHAPE_LINE_CHAIN pl;

        pl.Append( aP0 );

        if( start_diagonal )
            pl.Append( aP0 + mp1 );
        else
            pl.Append( aP0 + mp0 );

        pl.Append( aP1 );
        pl.Simplify();
        return pl;
    }

    bool operator==( const DIRECTION_45& aOther ) const
    {
        return aOther.m_dir == m_dir;
    }

    bool operator!=( const DIRECTION_45& aOther ) const
    {
        return aOther.m_dir != m_dir;
    }

    /**
     * Function Right()
     *
     * Returns the direction on the right side of this (i.e. turns right
     * by 45 deg)
     */
    const DIRECTION_45 Right() const
    {
        DIRECTION_45 r;

        if ( m_dir != UNDEFINED )
            r.m_dir = static_cast<Directions>( ( m_dir + 1 ) % 8 );

        return r;
    }

    /**
     * Function Left()
     *
     * Returns the direction on the left side of this (i.e. turns left
     * by 45 deg)
     */
    const DIRECTION_45 Left() const
    {
        DIRECTION_45 l;

        if ( m_dir == UNDEFINED )
            return l;

        if( m_dir == N )
            l.m_dir = NW;
        else
            l.m_dir = static_cast<Directions>( m_dir - 1 );

        return l;
    }

    /**
     * Function ToVector()
     *
     * Returns a unit vector corresponding to our direction.
     */
    const VECTOR2I ToVector() const
    {
        switch( m_dir )
        {
            case N: return VECTOR2I( 0, 1 );
            case S: return VECTOR2I( 0, -1 );
            case E: return VECTOR2I( 1, 0 );
            case W: return VECTOR2I( -1, 0 );
            case NE: return VECTOR2I( 1, 1 );
            case NW: return VECTOR2I( -1, 1 );
            case SE: return VECTOR2I( 1, -1 );
            case SW: return VECTOR2I( -1, -1 );

            default:
                return VECTOR2I( 0, 0 );
        }
    }

    int Mask() const
    {
        return 1 << ( (int) m_dir );
    }

private:

    /**
     * Function construct()
     * Calculates the direction from a vector. If the vector's angle is not a multiple of 45
     * degrees, the direction is rounded to the nearest octant.
     * @param aVec our vector
     */
    void construct_( const VECTOR2I& aVec )
    {
        m_dir = UNDEFINED;

        if( aVec.x == 0 && aVec.y == 0 )
            return;

        double mag = 360.0 - ( 180.0 / M_PI * atan2( (double) aVec.y, (double) aVec.x ) ) + 90.0;

        if( mag >= 360.0 )
            mag -= 360.0;

        if( mag < 0.0 )
            mag += 360.0;

        int dir = ( mag + 22.5 ) / 45.0;

        if( dir >= 8 )
            dir = dir - 8;

        if( dir < 0 )
            dir = dir + 8;

        m_dir = (Directions) dir;

        return;
    }

    ///> our actual direction
    Directions m_dir;
};

#endif    // DIRECTION45_H

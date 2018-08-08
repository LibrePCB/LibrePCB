/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (c) 2005 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (C) CERN
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

#include <cmath>
#include <cstdlib>
#include <climits>
#include <math/math_util.h>

template<>
int rescale( int aNumerator, int aValue, int aDenominator )
{
    return (int) ( (int64_t) aNumerator * (int64_t) aValue / (int64_t) aDenominator );
}


template<>
int64_t rescale( int64_t aNumerator, int64_t aValue, int64_t aDenominator )
{
#ifdef __x86_64__
    return ( (__int128_t) aNumerator * (__int128_t) aValue ) / aDenominator;
#else
    int64_t r = 0;
    int64_t sign = ( ( aNumerator < 0 ) ? -1 : 1 ) * ( aDenominator < 0 ? -1 : 1 ) *
                                                     ( aValue < 0 ? -1 : 1 );

    int64_t a = std::abs( aNumerator );
    int64_t b = std::abs( aValue );
    int64_t c = std::abs( aDenominator );

    r = c / 2;

    if( b <= INT_MAX && c <= INT_MAX )
    {
        if( a <= INT_MAX )
            return sign * ( ( a * b + r ) / c );
        else
            return sign * ( a / c * b + ( a % c * b + r ) / c);
    }
    else
    {
        uint64_t a0 = a & 0xFFFFFFFF;
        uint64_t a1 = a >> 32;
        uint64_t b0 = b & 0xFFFFFFFF;
        uint64_t b1 = b >> 32;
        uint64_t t1 = a0 * b1 + a1 * b0;
        uint64_t t1a = t1 << 32;
        int i;

        a0 = a0 * b0 + t1a;
        a1 = a1 * b1 + ( t1 >> 32 ) + ( a0 < t1a );
        a0 += r;
        a1 += a0 < (uint64_t)r;

        for( i = 63; i >= 0; i-- )
        {
            a1  += a1 + ( ( a0 >> i ) & 1 );
            t1  += t1;

            if( (uint64_t) c <= a1 )
            {
                a1 -= c;
                t1++;
            }
        }

        return t1 * sign;
    }
#endif
}

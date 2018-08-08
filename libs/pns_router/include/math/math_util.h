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

#ifndef __MATH_UTIL_H
#define __MATH_UTIL_H

#include <stdint.h>

/**
 * Function rescale()
 *
 * Scales a number (value) by rational (numerator/denominator). Numerator must be <= denominator.
 */

template <typename T>
T rescale( T aNumerator, T aValue, T aDenominator )
{
    return aNumerator * aValue / aDenominator;
}

template <typename T>
int sign( T val )
{
    return ( T( 0 ) < val) - ( val < T( 0 ) );
}

// explicit specializations for integer types, taking care of overflow.
template <>
int rescale( int aNumerator, int aValue, int aDenominator );

template <>
int64_t rescale( int64_t aNumerator, int64_t aValue, int64_t aDenominator );

#endif // __MATH_UTIL_H

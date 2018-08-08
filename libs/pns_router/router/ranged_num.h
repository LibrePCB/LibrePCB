/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __RANGED_NUM_H
#define __RANGED_NUM_H

template <class T> class RANGED_NUM {
    public:
        RANGED_NUM( T aValue = 0, T aTolerancePlus = 0, T aToleranceMinus = 0 ) :
            m_value( aValue ),
            m_tolerancePlus( aTolerancePlus ),
            m_toleranceMinus( aToleranceMinus )
        {}

        operator T()
        {
            return m_value;
        }

        RANGED_NUM& operator=( const T aValue )
        {
            m_value = aValue;
            return *this;
        }

        bool Matches( const T& aOther ) const
        {
            return ( aOther >= m_value - m_toleranceMinus && aOther <= m_value + m_tolerancePlus );
        }

    private:
        T m_value, m_tolerancePlus, m_toleranceMinus;
};

#endif

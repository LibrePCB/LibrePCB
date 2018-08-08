/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#ifndef __RANGE_H
#define __RANGE_H

template<class T>
class RANGE
{
public:
    RANGE( T aMin, T aMax ) :
        m_min( aMin ),
        m_max( aMax ),
        m_defined( true ) {}

    RANGE():
        m_defined( false ) {}

    T MinV() const
    {
        return m_min;
    }

    T MaxV() const
    {
        return m_max;
    }

    void Set( T aMin, T aMax ) const
    {
        m_max = aMax;
        m_min = aMin;
    }

    void Grow( T aValue )
    {
        if( !m_defined )
        {
            m_min = aValue;
            m_max = aValue;
            m_defined = true;
        }
        else
        {
            m_min = std::min( m_min, aValue );
            m_max = std::max( m_max, aValue );
        }
    }

    bool Inside( const T& aValue ) const
    {
        if( !m_defined )
            return true;

        return aValue >= m_min && aValue <= m_max;
    }

    bool Overlaps ( const RANGE<T>& aOther ) const
    {
        if( !m_defined || !aOther.m_defined )
            return true;

        return m_max >= aOther.m_min && m_min <= aOther.m_max;
    }

    bool Defined() const
    {
        return m_defined;
    }

private:
    T m_min, m_max;
    bool m_defined;
};

#endif

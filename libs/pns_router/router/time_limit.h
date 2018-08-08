/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __TIME_LIMIT_H
#define __TIME_LIMIT_H

#include <stdint.h>

namespace PNS {

class TIME_LIMIT
{
public:
    TIME_LIMIT( int aMilliseconds = 0 );
    ~TIME_LIMIT();

    bool Expired() const;
    void Restart();

    void Set( int aMilliseconds );
    int Get() const { return m_limitMs; }

private:
    int m_limitMs;
    int64_t m_startTics;
};

}

#endif

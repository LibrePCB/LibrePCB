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

#ifndef __PNS_LOGGER_H
#define __PNS_LOGGER_H

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

#include <math/vector2d.h>

class SHAPE_LINE_CHAIN;
class SHAPE;

namespace PNS {

class ITEM;

class LOGGER
{
public:
    LOGGER();
    ~LOGGER();

    void Save( const std::string& aFilename );
    void Clear();

    void NewGroup( const std::string& aName, int aIter = 0 );
    void EndGroup();

    void Log( const ITEM* aItem, int aKind = 0, const std::string& aName = std::string() );
    void Log( const SHAPE_LINE_CHAIN *aL, int aKind = 0, const std::string& aName = std::string() );
    void Log( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aKind = 0,
              const std::string& aName = std::string() );

private:
    void dumpShape( const SHAPE* aSh );

    bool m_groupOpened;
    std::stringstream m_theLog;
};

}

#endif

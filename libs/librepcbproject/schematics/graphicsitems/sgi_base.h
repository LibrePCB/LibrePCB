/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROJECT_SGI_BASE_H
#define PROJECT_SGI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/graphics/graphicsitem.h>
#include "../schematic.h"

/*****************************************************************************************
 *  Class SGI_Base
 ****************************************************************************************/

namespace project {

/**
 * @brief The Schematic Graphics Item Base (SGI_Base) class
 */
class SGI_Base : public GraphicsItem
{
    public:

        // Constructors / Destructor
        explicit SGI_Base() noexcept;
        virtual ~SGI_Base() noexcept;


    private:

        // make some methods inaccessible...
        //SGI_Base() = delete;
        SGI_Base(const SGI_Base& other) = delete;
        SGI_Base& operator=(const SGI_Base& rhs) = delete;
};

} // namespace project

#endif // PROJECT_SGI_BASE_H

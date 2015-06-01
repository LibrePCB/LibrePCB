/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_BGI_BASE_H
#define PROJECT_BGI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../../../common/graphics/graphicsitem.h"
#include "../board.h"

/*****************************************************************************************
 *  Class BGI_Base
 ****************************************************************************************/

namespace project {

/**
 * @brief The Board Graphics Item Base (BGI_Base) class
 */
class BGI_Base : public GraphicsItem
{
    public:

        // Constructors / Destructor
        explicit BGI_Base() noexcept;
        virtual ~BGI_Base() noexcept;


    private:

        // make some methods inaccessible...
        //BGI_Base() = delete;
        BGI_Base(const BGI_Base& other) = delete;
        BGI_Base& operator=(const BGI_Base& rhs) = delete;
};

} // namespace project

#endif // PROJECT_BGI_BASE_H

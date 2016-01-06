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

#ifndef LIBREPCB_IF_BOARDLAYERPROVIDER_H
#define LIBREPCB_IF_BOARDLAYERPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class BoardLayer;

/*****************************************************************************************
 *  Interface IF_BoardLayerProvider
 ****************************************************************************************/

/**
 * @brief The IF_BoardLayerProvider class defines an interface for classes which
 *        provide board layers
 *
 * @author ubruhin
 * @date 2015-06-06
 */
class IF_BoardLayerProvider
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         */
        IF_BoardLayerProvider() {}

        /**
         * @brief Destructor
         */
        virtual ~IF_BoardLayerProvider() {}


        // Getters

        /**
         * @brief Get the board layer with a specific ID
         *
         * @param id                The ID of the requested board layer
         *
         * @retval BoardLayer*      Pointer to the board layer with the specified ID
         * @retval nullptr          If no layer with the specified ID is available
         */
        virtual BoardLayer* getBoardLayer(int id) const noexcept = 0;


    private:

        // make some methods inaccessible...
        IF_BoardLayerProvider(const IF_BoardLayerProvider& other) = delete;
        IF_BoardLayerProvider& operator=(const IF_BoardLayerProvider& rhs) = delete;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_IF_BOARDLAYERPROVIDER_H


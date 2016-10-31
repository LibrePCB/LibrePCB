/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_IF_SCHEMATICLAYERPROVIDER_H
#define LIBREPCB_IF_SCHEMATICLAYERPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class SchematicLayer;

/*****************************************************************************************
 *  Interface IF_SchematicLayerProvider
 ****************************************************************************************/

/**
 * @brief The IF_SchematicLayerProvider class defines an interface for classes which
 *        provide schematic layers
 *
 * @author ubruhin
 * @date 2015-05-31
 */
class IF_SchematicLayerProvider
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         */
        IF_SchematicLayerProvider() {}

        /**
         * @brief Destructor
         */
        virtual ~IF_SchematicLayerProvider() {}


        // Getters

        /**
         * @brief Get the schematic layer with a specific ID
         *
         * @param id                The ID of the requested schematic layer
         *
         * @retval SchematicLayer*  Pointer to the schematic layer with the specified ID
         * @retval nullptr          If no layer with the specified ID is available
         */
        virtual SchematicLayer* getSchematicLayer(int id) const noexcept = 0;


    private:

        // make some methods inaccessible...
        IF_SchematicLayerProvider(const IF_SchematicLayerProvider& other) = delete;
        IF_SchematicLayerProvider& operator=(const IF_SchematicLayerProvider& rhs) = delete;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_IF_SCHEMATICLAYERPROVIDER_H


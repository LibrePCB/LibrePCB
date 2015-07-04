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

#ifndef PROJECT_SCHEMATICLAYERPROVIDER_H
#define PROJECT_SCHEMATICLAYERPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/if_schematiclayerprovider.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
}

/*****************************************************************************************
 *  Class SchematicLayerProvider
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicLayerProvider class provides and manages all available schematic
 *        layers which are used in the project#SchematicEditor class
 */
class SchematicLayerProvider final : public IF_SchematicLayerProvider
{
    public:

        // Constructors / Destructor
        explicit SchematicLayerProvider(Project& project) throw (Exception);
        ~SchematicLayerProvider() noexcept;

        // Getters
        Project& getProject() const noexcept {return mProject;}

        /**
         * @copydoc IF_SchematicLayerProvider#getSchematicLayer()
         */
        SchematicLayer* getSchematicLayer(uint id) const noexcept {return mLayers.value(id, nullptr);}


    private:

        // make some methods inaccessible...
        SchematicLayerProvider() = delete;
        SchematicLayerProvider(const SchematicLayerProvider& other) = delete;
        SchematicLayerProvider& operator=(const SchematicLayerProvider& rhs) = delete;

        // Private Methods
        void addLayer(uint id) noexcept;


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        QMap<uint, SchematicLayer*> mLayers;
};

} // namespace project

#endif // PROJECT_SCHEMATICLAYERPROVIDER_H

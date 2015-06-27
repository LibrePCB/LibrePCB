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

#ifndef PROJECT_BOARDLAYERPROVIDER_H
#define PROJECT_BOARDLAYERPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/if_boardlayerprovider.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
}

/*****************************************************************************************
 *  Class BoardLayerProvider
 ****************************************************************************************/

namespace project {

/**
 * @brief The BoardLayerProvider class provides and manages all available board layers
 *        which are used in the project#BoardEditor class
 */
class BoardLayerProvider final : public IF_BoardLayerProvider
{
    public:

        // Constructors / Destructor
        explicit BoardLayerProvider(Project& project) throw (Exception);
        ~BoardLayerProvider() noexcept;

        // Getters
        Project& getProject() const noexcept {return mProject;}

        /**
         * @copydoc IF_BoardLayerProvider#getBoardLayer()
         */
        BoardLayer* getBoardLayer(uint id) const noexcept {return mLayers.value(id, nullptr);}


    private:

        // make some methods inaccessible...
        BoardLayerProvider() = delete;
        BoardLayerProvider(const BoardLayerProvider& other) = delete;
        BoardLayerProvider& operator=(const BoardLayerProvider& rhs) = delete;

        // Private Methods
        void addLayer(uint id) noexcept;


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        QMap<uint, BoardLayer*> mLayers;
};

} // namespace project

#endif // PROJECT_BOARDLAYERPROVIDER_H

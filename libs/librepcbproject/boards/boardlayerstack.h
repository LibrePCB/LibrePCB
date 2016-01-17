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

#ifndef LIBREPCB_PROJECT_BOARDLAYERSTACK_H
#define LIBREPCB_PROJECT_BOARDLAYERSTACK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/if_boardlayerprovider.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Project;

/*****************************************************************************************
 *  Class BoardLayerStack
 ****************************************************************************************/

/**
 * @brief The BoardLayerStack class provides and manages all available layers of a board
 */
class BoardLayerStack final : public IF_BoardLayerProvider
{
    public:

        // Constructors / Destructor
        explicit BoardLayerStack(Project& project) throw (Exception);
        ~BoardLayerStack() noexcept;

        // Getters
        Project& getProject() const noexcept {return mProject;}

        /**
         * @copydoc IF_BoardLayerProvider#getBoardLayer()
         */
        BoardLayer* getBoardLayer(int id) const noexcept {return mLayers.value(id, nullptr);}


    private:

        // make some methods inaccessible...
        BoardLayerStack() = delete;
        BoardLayerStack(const BoardLayerStack& other) = delete;
        BoardLayerStack& operator=(const BoardLayerStack& rhs) = delete;

        // Private Methods
        void addLayer(int id) noexcept;


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        QMap<int, BoardLayer*> mLayers;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDLAYERSTACK_H

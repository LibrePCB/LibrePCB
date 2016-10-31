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

#ifndef LIBREPCB_PROJECT_BOARDLAYERSTACK_H
#define LIBREPCB_PROJECT_BOARDLAYERSTACK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/if_boardlayerprovider.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;

/*****************************************************************************************
 *  Class BoardLayerStack
 ****************************************************************************************/

/**
 * @brief The BoardLayerStack class provides and manages all available layers of a board
 */
class BoardLayerStack final : public QObject, public IF_XmlSerializableObject,
                              public IF_BoardLayerProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BoardLayerStack() = delete;
        BoardLayerStack(const BoardLayerStack& other) = delete;
        BoardLayerStack(Board& board, const BoardLayerStack& other) throw (Exception);
        BoardLayerStack(Board& board, const XmlDomElement& domElement) throw (Exception);
        explicit BoardLayerStack(Board& board) throw (Exception);
        ~BoardLayerStack() noexcept;

        // Getters
        Board& getBoard() const noexcept {return mBoard;}

        /// @copydoc IF_BoardLayerProvider#getAllBoardLayerIds()
        QList<int> getAllBoardLayerIds() const noexcept override {return mLayers.keys();}

        /// @copydoc IF_BoardLayerProvider#getBoardLayer()
        BoardLayer* getBoardLayer(int id) const noexcept override {return mLayers.value(id, nullptr);}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        BoardLayerStack& operator=(const BoardLayerStack& rhs) = delete;


    private slots:

        void layerAttributesChanged() noexcept;
        void boardAttributesChanged() noexcept;


    private:

        void addAllRequiredLayers() noexcept;
        void addLayer(int id) noexcept;
        void addLayer(BoardLayer& layer) noexcept;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        Board& mBoard; ///< A reference to the Board object (from the ctor)
        QMap<int, BoardLayer*> mLayers;
        bool mLayersChanged;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDLAYERSTACK_H

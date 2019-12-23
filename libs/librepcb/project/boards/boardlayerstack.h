/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Board;

/*******************************************************************************
 *  Class BoardLayerStack
 ******************************************************************************/

/**
 * @brief The BoardLayerStack class provides and manages all available layers of
 * a board
 */
class BoardLayerStack final : public QObject,
                              public SerializableObject,
                              public IF_GraphicsLayerProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardLayerStack()                             = delete;
  BoardLayerStack(const BoardLayerStack& other) = delete;
  BoardLayerStack(Board& board, const BoardLayerStack& other);
  BoardLayerStack(Board& board, const SExpression& node);
  explicit BoardLayerStack(Board& board);
  ~BoardLayerStack() noexcept;

  // Getters
  Board& getBoard() const noexcept { return mBoard; }
  int    getInnerLayerCount() const noexcept { return mInnerLayerCount; }
  QList<GraphicsLayer*> getAllowedPolygonLayers() const noexcept;

  /// @copydoc ::librepcb::IF_GraphicsLayerProvider::getAllLayers()
  QList<GraphicsLayer*> getAllLayers() const noexcept override {
    return mLayers;
  }

  /// @copydoc ::librepcb::IF_GraphicsLayerProvider::getLayer()
  GraphicsLayer* getLayer(const QString& name) const noexcept override {
    foreach (GraphicsLayer* layer, mLayers) {
      if (layer->getName() == name) {
        return layer;
      }
    }
    return nullptr;
  }

  // Setters
  void setInnerLayerCount(int count) noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  BoardLayerStack& operator=(const BoardLayerStack& rhs) = delete;

private slots:
  void layerAttributesChanged() noexcept;
  void boardAttributesChanged() noexcept;

private:
  void addAllLayers() noexcept;
  void addLayer(const QString& name, bool disable = false) noexcept;
  void addLayer(GraphicsLayer* layer) noexcept;

  // General
  Board& mBoard;  ///< A reference to the Board object (from the ctor)
  QList<GraphicsLayer*> mLayers;
  bool                  mLayersChanged;

  // Settings
  int mInnerLayerCount;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDLAYERSTACK_H

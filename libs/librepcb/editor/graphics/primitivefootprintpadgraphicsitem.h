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

#ifndef LIBREPCB_EDITOR_PRIMITIVEFOOTPRINTPADGRAPHICSITEM_H
#define LIBREPCB_EDITOR_PRIMITIVEFOOTPRINTPADGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicslayer.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Layer;
class Length;
class PadGeometry;
class Point;

namespace editor {

class OriginCrossGraphicsItem;
class PrimitivePathGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class PrimitiveFootprintPadGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitiveFootprintPadGraphicsItem class
 */
class PrimitiveFootprintPadGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  PrimitiveFootprintPadGraphicsItem() = delete;
  PrimitiveFootprintPadGraphicsItem(
      const PrimitiveFootprintPadGraphicsItem& other) = delete;
  PrimitiveFootprintPadGraphicsItem(const IF_GraphicsLayerProvider& lp,
                                    bool originCrossVisible,
                                    QGraphicsItem* parent = nullptr) noexcept;
  ~PrimitiveFootprintPadGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setRotation(const Angle& rotation) noexcept;
  void setText(const QString& text) noexcept;
  void setLayer(const QString& layerName) noexcept;
  void setGeometries(const QHash<const Layer*, QList<PadGeometry>>& geometries,
                     const Length& clearance) noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  PrimitiveFootprintPadGraphicsItem& operator=(
      const PrimitiveFootprintPadGraphicsItem& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updatePathLayers() noexcept;
  void updateTextHeight() noexcept;
  void updateRegisteredLayers() noexcept;

private:  // Data
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<GraphicsLayer> mCopperLayer;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mTextGraphicsItem;
  struct PathItem {
    std::shared_ptr<GraphicsLayer> layer;
    bool isCopper;
    bool isClearance;
    std::shared_ptr<PrimitivePathGraphicsItem> item;
  };
  QVector<PathItem> mPathGraphicsItems;
  QMap<std::shared_ptr<GraphicsLayer>, QPainterPath> mShapes;
  QRectF mShapesBoundingRect;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

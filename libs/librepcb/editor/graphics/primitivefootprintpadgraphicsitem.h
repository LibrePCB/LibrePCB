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

// AI DISCLAIMER: Claude AI assisted in the modification of this file.
// Reviewed 2026-09-29

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
class ColorRole;
class Layer;
class Length;
class PadGeometry;
class Point;

namespace editor {

class GraphicsLayerList;
class OriginCrossGraphicsItem;
class PrimitivePathGraphicsItem;

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
  PrimitiveFootprintPadGraphicsItem(const GraphicsLayerList& layers,
                                    bool originCrossVisible, int textMinAlpha,
                                    QGraphicsItem* parent = nullptr) noexcept;
  ~PrimitiveFootprintPadGraphicsItem() noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setRotation(const Angle& rotation) noexcept;
  void setMirrored(bool mirrored) noexcept;
  void setText(const QString& text) noexcept;
  void setTextMirrored(bool mirrored) noexcept;
  void setLayer(const ColorRole& role) noexcept;
  void setState(GraphicsLayer::State state) noexcept;
  void setGeometries(const QHash<const Layer*, QList<PadGeometry>>& geometries,
                     const Length& clearance) noexcept;

  /**
   * @brief Get the visible geometry, used for rubber-band selection
   *
   * This is used during rubber-band selection.  "Visible geometry" includes
   * only what is drawn, not the origin cross nor the padding of the
   * (click) #shape().
   *
   * @return Path in item coordinates, empty if nothing is visible.
   */
  QPainterPath getVisibleShape() const noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  PrimitiveFootprintPadGraphicsItem& operator=(
      const PrimitiveFootprintPadGraphicsItem& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updatePathLayers() noexcept;
  void updateTextHeight() noexcept;
  void updateRegisteredLayers() noexcept;

private:  // Data
  const GraphicsLayerList& mLayers;
  bool mMirror;
  GraphicsLayer::State mState;
  std::shared_ptr<const GraphicsLayer> mCopperLayer;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QScopedPointer<PrimitivePathGraphicsItem> mTextGraphicsItem;
  struct PathItem {
    std::shared_ptr<const GraphicsLayer> layer;
    bool isCopper;
    bool isClearance;
    std::shared_ptr<PrimitivePathGraphicsItem> item;
  };
  QVector<PathItem> mPathGraphicsItems;
  QMap<std::shared_ptr<const GraphicsLayer>, QPainterPath> mShapes;
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

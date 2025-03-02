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

#ifndef LIBREPCB_EDITOR_SGI_NETLABEL_H
#define LIBREPCB_EDITOR_SGI_NETLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_netlabel.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class IF_GraphicsLayerProvider;
class LineGraphicsItem;

/*******************************************************************************
 *  Class SGI_NetLabel
 ******************************************************************************/

/**
 * @brief The SGI_NetLabel class
 */
class SGI_NetLabel final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SGI_NetLabel() = delete;
  SGI_NetLabel(const SGI_NetLabel& other) = delete;
  SGI_NetLabel(SI_NetLabel& netlabel, const IF_GraphicsLayerProvider& lp,
               std::shared_ptr<const QSet<const NetSignal*>>
                   highlightedNetSignals) noexcept;
  virtual ~SGI_NetLabel() noexcept;

  // General Methods
  SI_NetLabel& getNetLabel() noexcept { return mNetLabel; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  SGI_NetLabel& operator=(const SGI_NetLabel& rhs) = delete;

private:  // Methods
  void netLabelEdited(const SI_NetLabel& obj,
                      SI_NetLabel::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updatePosition() noexcept;
  void updateRotation() noexcept;
  void updateText() noexcept;
  void updateAnchor() noexcept;

private:  // Data
  SI_NetLabel& mNetLabel;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  std::shared_ptr<GraphicsLayer> mOriginCrossLayer;
  std::shared_ptr<GraphicsLayer> mNetLabelLayer;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Cached Attributes
  QStaticText mStaticText;
  QVector<QLineF> mOverlines;
  QFont mFont;
  bool mRotate180;
  QPointF mTextOrigin;
  QRectF mBoundingRect;

  // Slots
  SI_NetLabel::OnEditedSlot mOnEditedSlot;

  // Static Stuff
  static QVector<QLineF> sOriginCrossLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

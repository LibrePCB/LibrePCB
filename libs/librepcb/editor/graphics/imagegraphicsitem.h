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

#ifndef LIBREPCB_EDITOR_IMAGEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_IMAGEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/image.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalDirectory;

namespace editor {

class GraphicsLayer;
class GraphicsLayerList;
class OriginCrossGraphicsItem;

/*******************************************************************************
 *  Class ImageGraphicsItem
 ******************************************************************************/

/**
 * @brief The ImageGraphicsItem class is the graphical representation of a
 *        ::librepcb::Image
 */
class ImageGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  ImageGraphicsItem() = delete;
  ImageGraphicsItem(const ImageGraphicsItem& other) = delete;
  ImageGraphicsItem(const TransactionalDirectory& dir,
                    const std::shared_ptr<Image>& image,
                    const GraphicsLayerList& layers,
                    QGraphicsItem* parent = nullptr) noexcept;
  virtual ~ImageGraphicsItem() noexcept;

  // Getters
  const std::shared_ptr<Image>& getObj() noexcept { return mImage; }

  /// Check if the resize handle is at a specific position
  ///
  /// @param pos    The position to check for the handle.
  /// @return       True if there is a handle, false if not.
  bool isResizeHandleAtPosition(const Point& pos) const noexcept;

  // Setters

  /**
   * Enable/disable editing mode when selected
   *
   * If the item is editable and selected, vertex handles will be shown to
   * indicate that they can be moved. If not editable, handles will not be
   * shown.
   *
   * @param editable  Whether the image is (visually) editable or not.
   */
  void setEditable(bool editable) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }

  // Operator Overloadings
  ImageGraphicsItem& operator=(const ImageGraphicsItem& rhs) = delete;

private:  // Methods
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;
  void imageEdited(const Image& image, Image::Event event) noexcept;
  void updatePixmap() noexcept;
  void updateBoundingRectAndShape() noexcept;

private:  // Data
  const TransactionalDirectory& mDir;
  std::shared_ptr<Image> mImage;
  bool mEditable;
  std::shared_ptr<const GraphicsLayer> mBordersLayer;
  std::unique_ptr<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;

  // Cached attributes
  QPixmap mPixmap;
  QRectF mImageRectPx;
  QRectF mBoundingRect;
  QPainterPath mShape;
  qreal mVertexHandleRadiusPx;
  bool mInvalidImage;

  // Slots
  Image::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

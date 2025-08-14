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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "imagegraphicsitem.h"

#include "graphicslayerlist.h"
#include "origincrossgraphicsitem.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ImageGraphicsItem::ImageGraphicsItem(const TransactionalDirectory& dir,
                                     const std::shared_ptr<Image>& text,
                                     const GraphicsLayerList& layers,
                                     QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mDir(dir),
    mImage(text),
    mEditable(false),
    mBordersLayer(layers.get(Theme::Color::sSchematicImageBorders)),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mVertexHandleRadiusPx(0),
    mInvalidImage(false),
    mOnEditedSlot(*this, &ImageGraphicsItem::imageEdited) {
  Q_ASSERT(mImage);

  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));
  mOriginCrossGraphicsItem->setLayer(
      layers.get(Theme::Color::sSchematicReferences));

  // It's hard to decide what Z-value images should have. At the moment I think
  // images should be on top of filled polygons/circles (z=0), but below
  // non-filled polygons/circles (z=2) and texts (z=5). This way images with
  // transparent background can be placed over any polygons (e.g. within
  // symbol's grab areas) but it's still possible to draw lines or texts over
  // the image.
  setZValue(1);

  setPos(mImage->getPosition().toPxQPointF());
  setRotation(-mImage->getRotation().toDeg());
  updatePixmap();
  updateBoundingRectAndShape();

  // register to the text to get attribute updates
  mImage->onEdited.attach(mOnEditedSlot);
}

ImageGraphicsItem::~ImageGraphicsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool ImageGraphicsItem::isResizeHandleAtPosition(
    const Point& pos) const noexcept {
  const Point relPos =
      pos.rotated(-mImage->getRotation(), mImage->getPosition()) -
      mImage->getPosition();
  const Length distance =
      *(Point(*mImage->getWidth(), *mImage->getHeight()) - relPos).getLength();
  return (distance.toPx() <= mVertexHandleRadiusPx);
}

void ImageGraphicsItem::setEditable(bool editable) noexcept {
  mEditable = editable;
  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QVariant ImageGraphicsItem::itemChange(GraphicsItemChange change,
                                       const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mOriginCrossGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

void ImageGraphicsItem::paint(QPainter* painter,
                              const QStyleOptionGraphicsItem* option,
                              QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());
  mVertexHandleRadiusPx = 20 / lod;

  // Draw pixmap.
  painter->drawPixmap(mImageRectPx, mPixmap, QRectF(mPixmap.rect()));

  // Draw border, if border is enabled or item is selected.
  if (mInvalidImage) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(isSelected ? Qt::red : Qt::darkRed, 0));
    painter->drawRect(mImageRectPx);
  } else if (mBordersLayer) {
    std::optional<UnsignedLength> borderWidth = mImage->getBorderWidth();
    if ((!borderWidth) && mEditable && isSelected) {
      borderWidth = UnsignedLength(0);
    }
    if (borderWidth) {
      const qreal w = (*borderWidth)->toPx();
      painter->setBrush(Qt::NoBrush);
      painter->setPen(QPen(mBordersLayer->getColor(isSelected), w,
                           Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      painter->drawRect(mImageRectPx.adjusted(-w / 2, -w / 2, w / 2, w / 2));
    }
  }

  // Draw resize handle if selected and editable.
  if (mEditable && isSelected && mBordersLayer) {
    QColor color = mBordersLayer->getColor(isSelected);
    color.setAlpha(color.alpha() / 3);
    painter->setBrush(Qt::NoBrush);

    qreal glowRadius = mVertexHandleRadiusPx;
    QPointF glowCenter = mImageRectPx.topRight();
    if (auto width = mImage->getBorderWidth()) {
      glowRadius = std::max(glowRadius, (*width)->toPx() * 2);
      glowCenter += QPointF((*width)->toPx() / 2, -(*width)->toPx() / 2);
    }
    QRadialGradient gradient(glowCenter, glowRadius);
    gradient.setColorAt(0, color);
    gradient.setColorAt(0.5, color);
    gradient.setColorAt(1, Qt::transparent);
    painter->setPen(QPen(QBrush(gradient), glowRadius * 2));
    painter->drawPoint(glowCenter);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ImageGraphicsItem::imageEdited(const Image& image,
                                    Image::Event event) noexcept {
  switch (event) {
    case Image::Event::FileNameChanged:
      updatePixmap();
      updateBoundingRectAndShape();
      break;
    case Image::Event::PositionChanged:
      setPos(image.getPosition().toPxQPointF());
      break;
    case Image::Event::RotationChanged:
      setRotation(-image.getRotation().toDeg());
      break;
    case Image::Event::WidthChanged:
    case Image::Event::HeightChanged:
      updateBoundingRectAndShape();
      break;
    case Image::Event::BorderWidthChanged:
      update();
      break;
    default:
      qWarning() << "Unhandled switch-case in ImageGraphicsItem::imageEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void ImageGraphicsItem::updatePixmap() noexcept {
  try {
    const QByteArray data = mDir.read(*mImage->getFileName());  // can throw
    const QString format = mImage->getFileName()->split(".").last();
    if (auto img = Image::tryLoad(data, format)) {
      mPixmap = QPixmap::fromImage(*img);
      return;
    }
  } catch (const Exception& e) {
  }

  qWarning() << "Failed to load image:" << *mImage->getFileName();
  mPixmap = QPixmap(":/fa/solid/triangle-exclamation.svg");
}

void ImageGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();

  mImageRectPx =
      QRectF(0, -mImage->getHeight()->toPx(), mImage->getWidth()->toPx(),
             mImage->getHeight()->toPx());

  const qreal m = Length(2000000).toPx();
  mBoundingRect = mImageRectPx.adjusted(-m, -m, m, m);

  QTransform t;
  t.scale(mImageRectPx.width() / std::max(mPixmap.width(), 1),
          mImageRectPx.height() / std::max(mPixmap.height(), 1));
  const QBitmap mask = mPixmap.mask().transformed(t);

  mShape = QPainterPath();
  if (!mask.isNull()) {
    mShape.addRegion(mask);
    mShape.translate(0, -mImageRectPx.height());
  } else {
    mShape.addRect(mImageRectPx);
  }
  mShape |= mOriginCrossGraphicsItem->shape();

  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

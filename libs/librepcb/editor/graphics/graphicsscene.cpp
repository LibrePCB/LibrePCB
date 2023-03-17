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
#include "graphicsscene.h"

#include <librepcb/core/types/point.h>

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

GraphicsScene::GraphicsScene(QObject* parent) noexcept
  : QGraphicsScene(parent), mSelectionRectItem(nullptr) {
  mSelectionRectItem = new QGraphicsRectItem();
  mSelectionRectItem->setPen(QPen(QColor(120, 170, 255, 255), 0));
  mSelectionRectItem->setBrush(QColor(150, 200, 255, 80));
  mSelectionRectItem->setZValue(1000);
  QGraphicsScene::addItem(mSelectionRectItem);
}

GraphicsScene::~GraphicsScene() noexcept {
  QGraphicsScene::removeItem(mSelectionRectItem);
  delete mSelectionRectItem;
  mSelectionRectItem = nullptr;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsScene::addItem(QGraphicsItem& item) noexcept {
  Q_ASSERT(!items().contains(&item));
  QGraphicsScene::addItem(&item);
}

void GraphicsScene::removeItem(QGraphicsItem& item) noexcept {
  Q_ASSERT(items().contains(&item));
  QGraphicsScene::removeItem(&item);
}

void GraphicsScene::setSelectionRectColors(const QColor& line,
                                           const QColor& fill) noexcept {
  mSelectionRectItem->setPen(QPen(line, 0));
  mSelectionRectItem->setBrush(fill);
}

void GraphicsScene::setSelectionRect(const Point& p1,
                                     const Point& p2) noexcept {
  QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  mSelectionRectItem->setRect(rectPx);
}

void GraphicsScene::clearSelectionRect() noexcept {
  mSelectionRectItem->setRect(QRectF());
}

QPixmap GraphicsScene::toPixmap(int dpi, const QColor& background) noexcept {
  QRectF rect = itemsBoundingRect();
  return toPixmap(QSize(qCeil(dpi * Length::fromPx(rect.width()).toInch()),
                        qCeil(dpi * Length::fromPx(rect.height()).toInch())),
                  background);
}

QPixmap GraphicsScene::toPixmap(const QSize& size,
                                const QColor& background) noexcept {
  QRectF rect = itemsBoundingRect();
  QPixmap pixmap(size);
  pixmap.fill(background);
  QPainter painter(&pixmap);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                         QPainter::SmoothPixmapTransform);
  render(&painter, QRectF(), rect, Qt::KeepAspectRatio);
  return pixmap;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

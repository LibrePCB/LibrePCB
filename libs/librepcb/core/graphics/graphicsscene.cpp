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

#include "../units/point.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsScene::GraphicsScene() noexcept
  : QGraphicsScene(nullptr), mSelectionRectItem(nullptr) {
  /*QBrush selectBrush = QGuiApplication::palette().highlight();
  QColor selectColor = selectBrush.color();
  selectColor.setAlpha(50);
  selectBrush.setColor(selectColor);*/
  QBrush selectBrush(QColor(150, 200, 255, 80), Qt::SolidPattern);
  mSelectionRectItem = new QGraphicsRectItem();
  mSelectionRectItem->setPen(QPen(QColor(120, 170, 255, 255), 0));
  mSelectionRectItem->setBrush(selectBrush);
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
  QGraphicsScene::addItem(&item);
}

void GraphicsScene::removeItem(QGraphicsItem& item) noexcept {
  QGraphicsScene::removeItem(&item);
}

void GraphicsScene::setSelectionRect(const Point& p1,
                                     const Point& p2) noexcept {
  QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  mSelectionRectItem->setRect(rectPx);
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

}  // namespace librepcb

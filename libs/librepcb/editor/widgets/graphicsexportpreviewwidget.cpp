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
#include "graphicsexportpreviewwidget.h"

#include <librepcb/core/application.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PageItem
 ******************************************************************************/

GraphicsExportWidget::PageItem::PageItem(bool showPageNumber,
                                         bool showResolution,
                                         int number) noexcept
  : mShowPageNumbers(showPageNumber),
    mShowResolution(showResolution),
    mNumber(number),
    mSize(),
    mMargins(),
    mPicture() {
}

GraphicsExportWidget::PageItem::~PageItem() noexcept {
}

void GraphicsExportWidget::PageItem::setContent(
    const QSize& pageSize, const QRectF margins,
    std::shared_ptr<QPicture> picture) noexcept {
  prepareGeometryChange();
  mSize = pageSize;
  mMargins = margins;
  mPicture = picture;
  if (mShowResolution) {
    setToolTip(getResolution());
  }
  update();
}

QRectF GraphicsExportWidget::PageItem::boundingRect() const noexcept {
  QSizeF size = getSize();
  qreal xMargin = size.width() / 5;
  qreal yMargin = size.height() / 20;
  return QRectF(-xMargin / 2, -yMargin / 2, size.width() + xMargin,
                size.height() + yMargin);
}

void GraphicsExportWidget::PageItem::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option,
    QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const QRectF paperRect(QPointF(0, 0), getSize());
  const QRectF margins = mMargins.isEmpty() ? paperRect : mMargins;
  QFont font = qApp->getDefaultSansSerifFont();

  // Draw shadow.
  painter->setClipRect(option->exposedRect);
  qreal shWidth = paperRect.width() / 100;
  QRectF rshadow(paperRect.topRight() + QPointF(0, shWidth),
                 paperRect.bottomRight() + QPointF(shWidth, 0));
  QLinearGradient rgrad(rshadow.topLeft(), rshadow.topRight());
  rgrad.setColorAt(0.0, QColor(0, 0, 0, 255));
  rgrad.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter->fillRect(rshadow, QBrush(rgrad));
  QRectF bshadow(paperRect.bottomLeft() + QPointF(shWidth, 0),
                 paperRect.bottomRight() + QPointF(0, shWidth));
  QLinearGradient bgrad(bshadow.topLeft(), bshadow.bottomLeft());
  bgrad.setColorAt(0.0, QColor(0, 0, 0, 255));
  bgrad.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter->fillRect(bshadow, QBrush(bgrad));
  QRectF cshadow(paperRect.bottomRight(),
                 paperRect.bottomRight() + QPointF(shWidth, shWidth));
  QRadialGradient cgrad(cshadow.topLeft(), shWidth, cshadow.topLeft());
  cgrad.setColorAt(0.0, QColor(0, 0, 0, 255));
  cgrad.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter->fillRect(cshadow, QBrush(cgrad));
  painter->setClipRect(paperRect & option->exposedRect);

  // Fill page background.
  painter->fillRect(paperRect, Qt::white);

  // Draw content.
  if (mPicture) {
    painter->drawPicture(0, 0, *mPicture);
  }

  // Draw margins.
  painter->setPen(QPen(Qt::gray, 0, Qt::DashLine));
  painter->drawRect(margins);

  // Draw page number.
  if (mShowPageNumbers) {
    font.setPixelSize(qCeil(qMin(margins.width(), margins.height() / 3)));
    painter->setFont(font);
    painter->setPen(Qt::gray);
    painter->drawText(margins, Qt::AlignCenter, QString::number(mNumber));
  }
}

QSize GraphicsExportWidget::PageItem::getSize() const noexcept {
  return mSize.isEmpty() ? QSize(500, 500) : mSize;
}

QString GraphicsExportWidget::PageItem::getResolution() const noexcept {
  return QString("%1x%2").arg(mSize.width()).arg(mSize.height());
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsExportWidget::GraphicsExportWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mView(new QGraphicsView(this)),
    mScene(new QGraphicsScene(this)),
    mItems(),
    mShowPageNumbers(true),
    mShowResolution(false) {
  mView->setInteractive(false);
  mView->setRenderHints(QPainter::Antialiasing |
                        QPainter::SmoothPixmapTransform);
  mView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  mView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  mView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  mView->setDragMode(QGraphicsView::ScrollHandDrag);
  mView->setBackgroundBrush(Qt::gray);
  mView->setScene(mScene.data());

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mView.data());

  mView->viewport()->installEventFilter(this);
}

GraphicsExportWidget::~GraphicsExportWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsExportWidget::setShowPageNumbers(bool show) noexcept {
  mShowPageNumbers = show;
}

void GraphicsExportWidget::setShowResolution(bool show) noexcept {
  mShowResolution = show;
}

void GraphicsExportWidget::setNumberOfPages(int number) noexcept {
  while (mItems.count() > number) {
    mItems.takeLast();
  }
  while (mItems.count() < number) {
    auto item = std::make_shared<PageItem>(mShowPageNumbers, mShowResolution,
                                           mItems.count() + 1);
    mScene->addItem(item.get());
    mItems.append(item);
  }
  updateItemPositions();
  updateScale();
}

void GraphicsExportWidget::setPageContent(
    int index, const QSize& pageSize, const QRectF margins,
    std::shared_ptr<QPicture> picture) noexcept {
  if ((index >= 0) && (index < mItems.count())) {
    mItems.at(index)->setContent(pageSize, margins, picture);
    updateItemPositions();
    updateScale();
  } else {
    qWarning() << "Graphics export preview page index out of bounds:" << index;
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void GraphicsExportWidget::resizeEvent(QResizeEvent* e) noexcept {
  QWidget::resizeEvent(e);
  updateScale();
}

void GraphicsExportWidget::showEvent(QShowEvent* e) noexcept {
  QWidget::showEvent(e);
  updateScale();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsExportWidget::updateScale() noexcept {
  QRect rect = mView->viewport()->rect();
  qreal scale = rect.width() / (mScene->itemsBoundingRect().width() + 0.1);
  mView->setTransform(QTransform::fromScale(scale, scale));
}

void GraphicsExportWidget::updateItemPositions() noexcept {
  qreal y = 0;
  foreach (auto item, mItems) {
    QRectF rect = item->boundingRect();
    item->setPos(-rect.center().x(), y - rect.top());
    y += rect.height();
  }

  // Resize scrollable area to current preview size.
  mScene->setSceneRect(mScene->itemsBoundingRect());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

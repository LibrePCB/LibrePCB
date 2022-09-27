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
#include "waitingspinnerwidget.h"

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

WaitingSpinnerWidget::WaitingSpinnerWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mTotalRotations(7),
    mCurrentRotation(0),
    mCircleDiameter(14),
    mDotDiameter(5),
    mMargin(4),
    mTimer(new QTimer(this)) {
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_TranslucentBackground);

  mTimer->setInterval(100);
  connect(mTimer.data(), &QTimer::timeout, this, [this]() {
    mCurrentRotation = (mCurrentRotation + 1) % mTotalRotations;
    update();
  });

  if (QAbstractScrollArea* sa = qobject_cast<QAbstractScrollArea*>(parent)) {
    // Avoid painting on the scrollbars.
    mEventFilterObject = sa->viewport();
  } else if (parent) {
    mEventFilterObject = parent;
  }
  if (mEventFilterObject) {
    mEventFilterObject->installEventFilter(this);
  }

  updateSize();
  updatePosition();
}

WaitingSpinnerWidget::~WaitingSpinnerWidget() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void WaitingSpinnerWidget::showEvent(QShowEvent* e) noexcept {
  mTimer->start();
  QWidget::showEvent(e);
}

void WaitingSpinnerWidget::hideEvent(QHideEvent* e) noexcept {
  mTimer->stop();
  QWidget::hideEvent(e);
}

void WaitingSpinnerWidget::paintEvent(QPaintEvent* e) noexcept {
  Q_UNUSED(e);

  const qreal center = calculateSize() / qreal(2);
  QColor color = palette().color(QPalette::Text);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setBrush(Qt::NoBrush);
  painter.translate(center, center);
  painter.rotate(qreal(360) * mCurrentRotation / mTotalRotations);
  for (int i = 0; i < mTotalRotations; ++i) {
    const qreal dotDiameter =
        mDotDiameter * (qreal(mTotalRotations - i) / mTotalRotations);
    painter.setPen(QPen(color, dotDiameter, Qt::SolidLine, Qt::RoundCap));
    painter.drawPoint(QPointF(mCircleDiameter / qreal(2), 0));
    painter.rotate(qreal(-360) / mTotalRotations);
    color.setAlphaF(color.alphaF() * 0.8);
  }
}

bool WaitingSpinnerWidget::eventFilter(QObject* watched,
                                       QEvent* event) noexcept {
  if ((watched == mEventFilterObject) && (event->type() == QEvent::Resize)) {
    updatePosition();
  }
  return QWidget::eventFilter(watched, event);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int WaitingSpinnerWidget::calculateSize() const noexcept {
  return mCircleDiameter + mDotDiameter + (mMargin * 2);
}

void WaitingSpinnerWidget::updateSize() noexcept {
  const int size = calculateSize();
  setFixedSize(size, size);
}

void WaitingSpinnerWidget::updatePosition() noexcept {
  if (QWidget* parent = parentWidget()) {
    int parentWidth = parent->width();
    if (mEventFilterObject && (parent != mEventFilterObject)) {
      QPoint topRight = mEventFilterObject->geometry().topRight();
      parentWidth = mEventFilterObject->mapTo(parent, topRight).x();
    }
    move(parentWidth - width(), 0);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

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

#ifndef UNITTESTS_CORE_GRAPHICSEXPORTTEST_H
#define UNITTESTS_CORE_GRAPHICSEXPORTTEST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  GraphicsPagePainter
 ******************************************************************************/

class GraphicsPagePainterMock : public GraphicsPagePainter {
  Point mPos;
  Length mWidth;
  Length mHeight;

public:
  GraphicsPagePainterMock(const Length& x = Length(0),
                          const Length& y = Length(),
                          const Length& width = Length(200000000),
                          const Length& height = Length(100000000)) noexcept
    : mPos(x, y), mWidth(width), mHeight(height) {}

  virtual ~GraphicsPagePainterMock() noexcept {}

  void paint(QPainter& painter,
             const GraphicsExportSettings& settings) const noexcept override {
    Q_UNUSED(settings);

    Point topLeft(mPos.getX() - mWidth / 2, mPos.getY() + mHeight / 2);
    Point bottomRight(mPos.getX() + mWidth / 2, mPos.getY() - mHeight / 2);
    QRectF rect(topLeft.toPxQPointF(), bottomRight.toPxQPointF());
    rect.adjust(0, 0, -1, -1);  // Ensure content is *within* bounds.

    painter.setPen(QPen(Qt::red, 5));
    painter.drawEllipse(rect.adjusted(20, 20, -20, -20));
    painter.setPen(QPen(Qt::black, 0));
    painter.drawRect(rect);
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif

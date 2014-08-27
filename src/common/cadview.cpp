/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QtOpenGL>
#include "cadview.h"
#include "cadscene.h"

/*****************************************************************************************
 *  Static Variables
 ****************************************************************************************/

qreal CADView::sZoomFactor = 1.15;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CADView::CADView(QWidget* parent) :
    QGraphicsView(parent),
    mGridType(noGrid), mGridColor(Qt::lightGray), mOriginCrossColor(Qt::black)
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing| QPainter::SmoothPixmapTransform);
    //setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::AlphaChannel | QGL::SampleBuffers)));
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);
    setSceneRect(-10000, -10000, 20000, 20000); ///< @todo is there a better solution?
}

CADView::~CADView()
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

CADScene* CADView::getCadScene() const
{
    return dynamic_cast<CADScene*>(scene());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CADView::setGridType(GridType type)
{
    mGridType = type;
}

void CADView::setGridColor(const QColor& color)
{
    mGridColor = color;
}

/*****************************************************************************************
 *  Zoom Functions
 ****************************************************************************************/

void CADView::zoomIn()
{
    scale(sZoomFactor, sZoomFactor);
}

void CADView::zoomOut()
{
    scale((qreal)1 / sZoomFactor, (qreal)1 / sZoomFactor);
}

void CADView::zoomAll()
{
    if (scene())
        fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

void CADView::drawBackground(QPainter* painter, const QRectF& rect)
{
    if (!getCadScene())
        return;

    qreal gridIntervalPixels = getCadScene()->getGridInterval().toPx();
    qreal scaleFactor = width() / rect.width();
    qreal left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
    qreal top = qFloor(rect.top() / gridIntervalPixels) * gridIntervalPixels;

    QPen gridPen(mGridColor);
    gridPen.setWidth(0);

    painter->setPen(gridPen);
    painter->setBrush(backgroundBrush());
    painter->setWorldMatrixEnabled(true);

    // draw background color
    painter->fillRect(rect, backgroundBrush());

    // draw background grid lines
    if (scaleFactor >= (qreal)0.5)
    {
        switch (mGridType)
        {
            case gridLines:
            {
                QVarLengthArray<QLineF, 500> lines;

                for (qreal x = left; x < rect.right(); x += gridIntervalPixels)
                    lines.append(QLineF(x, rect.top(), x, rect.bottom()));

                for (qreal y = top; y < rect.bottom(); y += gridIntervalPixels)
                    lines.append(QLineF(rect.left(), y, rect.right(), y));

                painter->setOpacity(0.5);
                painter->drawLines(lines.data(), lines.size());
                break;
            }

            case gridDots:
            {
                QVarLengthArray<QPointF, 2000> dots;

                for (qreal x = left; x < rect.right(); x += gridIntervalPixels)
                    for (qreal y = top; y < rect.bottom(); y += gridIntervalPixels)
                        dots.append(QPointF(x, y));

                painter->drawPoints(dots.data(), dots.size());
                break;
            }

            default:
                break;
        }
    }

    // draw origin cross
    QPen originPen(mOriginCrossColor);
    originPen.setWidth(0);
    painter->setPen(originPen);
    painter->drawLine(-21.6, 0, 21.6, 0);
    painter->drawLine(0, -21.6, 0, 21.6);
}

void CADView::wheelEvent(QWheelEvent* event)
{
    // Zoom
    const QPointF p0scene = mapToScene(event->pos());
    qreal scaleFactor = (event->delta() > 0) ? sZoomFactor : (qreal)1 / sZoomFactor;
    scale(scaleFactor, scaleFactor);

    // Pan
    const QPointF p1mouse = mapFromScene(p0scene);
    const QPointF diff = p1mouse - event->pos();
    horizontalScrollBar()->setValue(diff.x() + horizontalScrollBar()->value());
    verticalScrollBar()->setValue(diff.y() + verticalScrollBar()->value());

    /// @todo the variant above works also with non-interactive views,
    /// but does it also work with multitouch gestures / pinch to zoom?
    /// the variant below does NOT work with non-interactive views!

    //if(event->delta() > 0)
    //    scale(sZoomFactor, sZoomFactor); // Zoom in
    //else
    //    scale((qreal)1 / sZoomFactor, (qreal)1 / sZoomFactor); // Zoom out
}

void CADView::mouseMoveEvent(QMouseEvent* event)
{
    if (scene())
    {
        try
        {
            Point pos = Point::fromPx(mapToScene(event->pos()),
                                      getCadScene()->getGridInterval());

            // emit the signal only if the position has really changed
            if (pos != mLastMouseMoveEventPos)
            {
                emit mousePosChanged(pos);
                mLastMouseMoveEventPos = pos;
            }
        }
        catch (...)
        {
            // If the cursor is outside the +/-2.147 meters for the 32bit type LengthBase_t,
            // the Length class will throw a range error. This would be not good, so we catch
            // that exception and ignore it...
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

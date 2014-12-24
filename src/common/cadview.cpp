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
    mGridType(GridType_t::Off), mGridColor(Qt::lightGray), mOriginCrossColor(Qt::black),
    mGridInterval(2540000), mGridIntervalUnit(LengthUnit::millimeters()),
    mPositionLabel(0)
{
    mPositionLabel = new QLabel(this);
    mPositionLabel->move(5, 5);
    mPositionLabel->show();
    updatePositionLabelText();

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
    delete mPositionLabel;      mPositionLabel = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

CADScene* CADView::getCadScene() const
{
    return dynamic_cast<CADScene*>(scene());
}

QRectF CADView::getVisibleSceneRect() const
{
    return mapToScene(viewport()->rect()).boundingRect();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CADView::setCadScene(CADScene* scene)
{
    setScene(scene);
    updatePositionLabelText();
}

void CADView::setVisibleSceneRect(const QRectF& rect)
{
    fitInView(rect, Qt::KeepAspectRatio);
}

void CADView::setGridType(GridType_t type)
{
    mGridType = type;
    QGraphicsView::setBackgroundBrush(QBrush(Qt::NoBrush)); // this will repaint the background
}

void CADView::setGridColor(const QColor& color)
{
    mGridColor = color;
    QGraphicsView::setBackgroundBrush(QBrush(Qt::NoBrush)); // this will repaint the background
}

void CADView::setGridInterval(const Length& newInterval)
{
    mGridInterval = newInterval;
    QGraphicsView::setBackgroundBrush(QBrush(Qt::NoBrush)); // this will repaint the background
}

void CADView::setGridIntervalUnit(const LengthUnit& newUnit)
{
    mGridIntervalUnit = newUnit;
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

    qreal gridIntervalPixels = mGridInterval.toPx();
    qreal scaleFactor = width() / rect.width();
    qreal left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
    qreal top = qFloor(rect.top() / gridIntervalPixels) * gridIntervalPixels;

    QPen gridPen(mGridColor);
    gridPen.setCapStyle(Qt::RoundCap);
    gridPen.setWidth((mGridType == GridType_t::Dots) ? 2 : 1);
    gridPen.setCosmetic(true);

    painter->setPen(gridPen);
    painter->setBrush(backgroundBrush());
    painter->setWorldMatrixEnabled(true);

    // draw background color
    painter->fillRect(rect, backgroundBrush());

    // draw background grid lines
    if (gridIntervalPixels * scaleFactor >= (qreal)3)
    {
        switch (mGridType)
        {
            case GridType_t::Lines:
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

            case GridType_t::Dots:
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
}

void CADView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    if (!getCadScene())
        return;

    // draw origin cross
    QPen originPen(mOriginCrossColor);
    originPen.setWidth(0);
    painter->setPen(originPen);
    painter->drawLine(-21.6, 0, 21.6, 0);
    painter->drawLine(0, -21.6, 0, 21.6);
}

void CADView::wheelEvent(QWheelEvent* event)
{
    if (scene())
    {
        // First, redirect the event to the scene. If the scene has an event handler
        // object (like SchematicEditor), the event may first go through a finite state
        // machine. If the FSM accepts the event, we will disable zooming with the wheel.
        // Note: This code fragment is copied from Qt's "qgraphicsview.cpp" (Qt 5.3.2)
        QGraphicsSceneWheelEvent wheelEvent(QEvent::GraphicsSceneWheel);
        wheelEvent.setWidget(viewport());
        wheelEvent.setScenePos(mapToScene(event->pos()));
        wheelEvent.setScreenPos(event->globalPos());
        wheelEvent.setButtons(event->buttons());
        wheelEvent.setModifiers(event->modifiers());
        wheelEvent.setDelta(event->delta());
        wheelEvent.setOrientation(event->orientation());
        wheelEvent.setAccepted(false);
        QApplication::sendEvent(scene(), &wheelEvent);
        event->setAccepted(wheelEvent.isAccepted());
        if (event->isAccepted()) return; // the scene (or an FSM) has accepted the event
    }

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
    if (getCadScene())
        updatePositionLabelText(Point::fromPx(mapToScene(event->pos()), mGridInterval).toMmQPointF());
    else
        updatePositionLabelText();

    QGraphicsView::mouseMoveEvent(event);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CADView::updatePositionLabelText(const QPointF pos)
{
    mPositionLabel->setText(QString("Grid: %1mm\nX: %2mm\nY: %3mm")
                            .arg(mGridInterval.toMm()).arg(pos.x()).arg(pos.y()));
    mPositionLabel->adjustSize();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

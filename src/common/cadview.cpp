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
#include "../workspace/workspace.h"
#include "../workspace/settings/workspacesettings.h"
#include "gridproperties.h"

/*****************************************************************************************
 *  Static Variables
 ****************************************************************************************/

qreal CADView::sZoomFactor = 1.15;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CADView::CADView(QWidget* parent) :
    QGraphicsView(parent),
    mGridColor(Qt::lightGray), mGridBoundedToPageBorders(false),
    mOriginCrossVisible(true), mOriginCrossColor(Qt::black), mPageSizePx(),
    mPositionLabel(nullptr), mZoomAnimation(nullptr)
{
    mGridProperties = new GridProperties();

    mPositionLabel = new QLabel(this);
    mPositionLabel->move(5, 5);
    mPositionLabel->show();
    updatePositionLabelText();

    mZoomAnimation = new QVariantAnimation();
    connect(mZoomAnimation, &QVariantAnimation::valueChanged,
            this, &CADView::zoomAnimationValueChanged);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if (Workspace::instance().getSettings().getAppearance()->getUseOpenGl())
        setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::AlphaChannel | QGL::SampleBuffers)));
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);
    setSceneRect(-2000, -2000, 4000, 4000); ///< @todo is there a better solution?
}

CADView::~CADView()
{
    delete mZoomAnimation;      mZoomAnimation = nullptr;
    delete mPositionLabel;      mPositionLabel = nullptr;
    delete mGridProperties;     mGridProperties = nullptr;
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

void CADView::setGridProperties(const GridProperties& properties) noexcept
{
    *mGridProperties = properties;
    QGraphicsView::setBackgroundBrush(backgroundBrush()); // this will repaint the background
}

void CADView::setOriginCrossVisible(bool visible) noexcept
{
    mOriginCrossVisible = visible;
}

void CADView::setPaperSize(const Point& size) noexcept
{
    if (size.isOrigin())
        mPageSizePx = QSizeF();
    else
        mPageSizePx = QSizeF(size.getX().toPx(), size.getY().toPx());
    QGraphicsView::setBackgroundBrush(backgroundBrush()); // this will repaint the background
}

void CADView::setPositionLabelVisible(bool visible) noexcept
{
    mPositionLabel->setVisible(visible);
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
    {
        QRectF rect = scene()->itemsBoundingRect();
        if (rect.isEmpty()) rect = QRectF(0, -500, 800, 500);
        mZoomAnimation->setDuration(500);
        mZoomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
        mZoomAnimation->setStartValue(getVisibleSceneRect());
        mZoomAnimation->setEndValue(rect);
        mZoomAnimation->start();
    }
}


/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void CADView::zoomAnimationValueChanged(const QVariant& value) noexcept
{
    if (value.canConvert(QMetaType::QRectF))
        fitInView(value.toRectF(), Qt::KeepAspectRatio); // zoom all smoothly
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

void CADView::drawBackground(QPainter* painter, const QRectF& rect)
{
    if (!getCadScene())
        return;

    QPen gridPen(mGridColor);
    gridPen.setCosmetic(true);

    // draw background color
    painter->setPen(Qt::NoPen);
    painter->setBrush(backgroundBrush());
    painter->fillRect(rect, backgroundBrush());

    if (!mPageSizePx.isEmpty())
    {
        // draw page size
        gridPen.setWidth(2);
        painter->setPen(gridPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(0, 0, mPageSizePx.width(), -mPageSizePx.height());
    }

    // draw background grid lines
    gridPen.setWidth((mGridProperties->getType() == GridProperties::Type_t::Dots) ? 2 : 1);
    painter->setPen(gridPen);
    painter->setBrush(Qt::NoBrush);
    qreal gridIntervalPixels = mGridProperties->getInterval().toPx();
    qreal scaleFactor = width() / rect.width();
    if (gridIntervalPixels * scaleFactor >= (qreal)5)
    {
        qreal left, right, top, bottom;
        if ((mGridBoundedToPageBorders) && (!mPageSizePx.isEmpty()))
        {
            left = 0;
            right = mPageSizePx.width();
            top = mPageSizePx.height();
            bottom = 0;
        }
        else
        {
            left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
            right = rect.right();
            top = rect.top();
            bottom = qFloor(rect.bottom() / gridIntervalPixels) * gridIntervalPixels;
        }
        switch (mGridProperties->getType())
        {
            case GridProperties::Type_t::Lines:
            {
                QVarLengthArray<QLineF, 500> lines;
                for (qreal x = left; x < right; x += gridIntervalPixels)
                    lines.append(QLineF(x, top, x, bottom));
                for (qreal y = bottom; y > top; y -= gridIntervalPixels)
                    lines.append(QLineF(left, y, right, y));
                painter->setOpacity(0.5);
                painter->drawLines(lines.data(), lines.size());
                break;
            }

            case GridProperties::Type_t::Dots:
            {
                QVarLengthArray<QPointF, 2000> dots;
                for (qreal x = left; x < right; x += gridIntervalPixels)
                    for (qreal y = bottom; y > top; y -= gridIntervalPixels)
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

    if (!mOriginCrossVisible) return;
    if (!getCadScene()) return;

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

    if(event->modifiers().testFlag(Qt::ShiftModifier))
    {
        // horizontal scrolling
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - event->delta());
    }
    else if(event->modifiers().testFlag(Qt::ControlModifier))
    {
        // vertical scrolling
        verticalScrollBar()->setValue(verticalScrollBar()->value() - event->delta());
    }
    else
    {
        // Zoom to mouse
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        qreal scaleFactor = (event->delta() > 0) ? sZoomFactor : (qreal)1 / sZoomFactor;
        scale(scaleFactor, scaleFactor);
    }

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
        updatePositionLabelText(Point::fromPx(mapToScene(event->pos()),
                                              mGridProperties->getInterval()).toMmQPointF());
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
                            .arg(mGridProperties->getInterval().toMm())
                            .arg(pos.x()).arg(pos.y()));
    mPositionLabel->adjustSize();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

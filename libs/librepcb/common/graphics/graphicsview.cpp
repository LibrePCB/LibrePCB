/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "QtOpenGL"
#include "graphicsview.h"
#include "graphicsscene.h"
#include "if_graphicsvieweventhandler.h"
#include "../gridproperties.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GraphicsView::GraphicsView(QWidget* parent, IF_GraphicsViewEventHandler* eventHandler) noexcept :
    QGraphicsView(parent), mEventHandlerObject(eventHandler), mScene(nullptr),
    mZoomAnimation(nullptr), mGridProperties(new GridProperties()), mOriginCrossVisible(true),
    mUseOpenGl(false), mPanningActive(false)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setSceneRect(-2000, -2000, 4000, 4000);

    mZoomAnimation = new QVariantAnimation();
    connect(mZoomAnimation, &QVariantAnimation::valueChanged,
            this, &GraphicsView::zoomAnimationValueChanged);
}

GraphicsView::~GraphicsView() noexcept
{
    delete mZoomAnimation;      mZoomAnimation = nullptr;
    delete mGridProperties;     mGridProperties = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QRectF GraphicsView::getVisibleSceneRect() const noexcept
{
    return mapToScene(viewport()->rect()).boundingRect();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GraphicsView::setUseOpenGl(bool useOpenGl) noexcept
{
    if (useOpenGl != mUseOpenGl)
    {
        if (useOpenGl)
            setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::AlphaChannel | QGL::SampleBuffers)));
        else
            setViewport(nullptr);
        mUseOpenGl = useOpenGl;
    }
}

void GraphicsView::setGridProperties(const GridProperties& properties) noexcept
{
    *mGridProperties = properties;
    setBackgroundBrush(backgroundBrush()); // this will repaint the background
}

void GraphicsView::setScene(GraphicsScene* scene) noexcept
{
    if (mScene) mScene->removeEventFilter(this);
    mScene = scene;
    if (mScene) mScene->installEventFilter(this);
    QGraphicsView::setScene(mScene);
}

void GraphicsView::setVisibleSceneRect(const QRectF& rect) noexcept
{
    fitInView(rect, Qt::KeepAspectRatio);
}

void GraphicsView::setOriginCrossVisible(bool visible) noexcept
{
    mOriginCrossVisible = visible;
    setForegroundBrush(foregroundBrush()); // this will repaint the foreground
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

Point GraphicsView::mapGlobalPosToScenePos(const QPoint& globalPosPx, bool boundToView,
                                           bool mapToGrid) const noexcept
{
    QPoint localPosPx = mapFromGlobal(globalPosPx);
    if (boundToView) {
        localPosPx.setX(qBound(0, localPosPx.x(), width()));
        localPosPx.setY(qBound(0, localPosPx.y(), height()));
    }
    Length gridInterval = mapToGrid ? mGridProperties->getInterval() : Length(0);
    return Point::fromPx(mapToScene(localPosPx), gridInterval);
}

void GraphicsView::handleMouseWheelEvent(QGraphicsSceneWheelEvent* event) noexcept
{
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
        qreal scaleFactor = qPow(sZoomStepFactor, event->delta()/qreal(120));
        scale(scaleFactor, scaleFactor);
    }
    event->setAccepted(true);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void GraphicsView::zoomIn() noexcept
{
    if (!mScene) return;
    scale(sZoomStepFactor, sZoomStepFactor);
}

void GraphicsView::zoomOut() noexcept
{
    if (!mScene) return;
    scale(1/sZoomStepFactor, 1/sZoomStepFactor);
}

void GraphicsView::zoomAll() noexcept
{
    if (!mScene) return;
    QRectF rect = mScene->itemsBoundingRect();
    if (rect.isEmpty()) rect = QRectF(-100, -100, 200, 200);
    qreal xMargins = rect.width() / 50;
    qreal yMargins = rect.height() / 50;
    rect.adjust(-xMargins, -yMargins, xMargins, yMargins);
    mZoomAnimation->setDuration(500);
    mZoomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    mZoomAnimation->setStartValue(getVisibleSceneRect());
    mZoomAnimation->setEndValue(rect);
    mZoomAnimation->start();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void GraphicsView::zoomAnimationValueChanged(const QVariant& value) noexcept
{
    if (value.canConvert(QMetaType::QRectF))
        fitInView(value.toRectF(), Qt::KeepAspectRatio); // zoom smoothly
}

/*****************************************************************************************
 *  Inherited from QGraphicsView
 ****************************************************************************************/

bool GraphicsView::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type())
    {
        case QEvent::GraphicsSceneMouseMove:
        {
            if (!underMouse()) break;
            QGraphicsSceneMouseEvent* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event); Q_ASSERT(e);
            if (e->buttons().testFlag(Qt::MiddleButton) && (!mPanningActive)) {
                QPoint diff = mapFromScene(e->scenePos()) - mapFromScene(e->buttonDownScenePos(Qt::MiddleButton));
                mPanningActive = true; // avoid recursive calls (=> stack overflow)
                horizontalScrollBar()->setValue(horizontalScrollBar()->value() - diff.x());
                verticalScrollBar()->setValue(verticalScrollBar()->value() - diff.y());
                mPanningActive = false;
                return true;
            } else if (mPanningActive) {
                return true;
            }
            // no break here!
        }
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        case QEvent::GraphicsSceneMouseRelease:
        case QEvent::GraphicsSceneContextMenu:
        {
            if (!underMouse()) break;
            if (mEventHandlerObject)
                mEventHandlerObject->graphicsViewEventHandler(event);
            return true;
        }
        case QEvent::GraphicsSceneWheel:
        {
            if (!underMouse()) break;
            if (mEventHandlerObject)
            {
                if (!mEventHandlerObject->graphicsViewEventHandler(event))
                    handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
            }
            else
            {
                handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
            }
            return true;
        }
        default:
            break;
    }
    return QWidget::eventFilter(obj, event);
}

void GraphicsView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QPen gridPen(Qt::gray);
    gridPen.setCosmetic(true);

    // draw background color
    painter->setPen(Qt::NoPen);
    painter->setBrush(backgroundBrush());
    painter->fillRect(rect, backgroundBrush());

    // draw background grid lines
    gridPen.setWidth((mGridProperties->getType() == GridProperties::Type_t::Dots) ? 2 : 1);
    painter->setPen(gridPen);
    painter->setBrush(Qt::NoBrush);
    qreal gridIntervalPixels = mGridProperties->getInterval().toPx();
    qreal scaleFactor = width() / rect.width();
    if (gridIntervalPixels * scaleFactor >= (qreal)5)
    {
        qreal left, right, top, bottom;
        left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
        right = rect.right();
        top = rect.top();
        bottom = qFloor(rect.bottom() / gridIntervalPixels) * gridIntervalPixels;
        switch (mGridProperties->getType())
        {
            case GridProperties::Type_t::Lines:
            {
                QVarLengthArray<QLineF, 500> lines;
                for (qreal x = left; x < right; x += gridIntervalPixels)
                    lines.append(QLineF(x, rect.top(), x, rect.bottom()));
                for (qreal y = bottom; y > top; y -= gridIntervalPixels)
                    lines.append(QLineF(rect.left(), y, rect.right(), y));
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

void GraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    if (mOriginCrossVisible)
    {
        // draw origin cross
        qreal len = Length::fromMm(2.54).toPx();
        QPen originPen(foregroundBrush().color());
        originPen.setWidth(0);
        painter->setPen(originPen);
        painter->drawLine(QLineF(-len, 0.0, len, 0.0));
        painter->drawLine(QLineF(0.0, -len, 0.0, len));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

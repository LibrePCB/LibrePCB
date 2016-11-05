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

#ifndef LIBREPCB_GRAPHICSVIEW_H
#define LIBREPCB_GRAPHICSVIEW_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class IF_GraphicsViewEventHandler;
class GraphicsScene;
class GridProperties;

/*****************************************************************************************
 *  Class GraphicsView
 ****************************************************************************************/

/**
 * @brief The GraphicsView class
 */
class GraphicsView final : public QGraphicsView
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GraphicsView(QWidget* parent = nullptr,
                              IF_GraphicsViewEventHandler* eventHandler = nullptr) noexcept;
        ~GraphicsView() noexcept;

        // Getters
        GraphicsScene* getScene() const noexcept {return mScene;}
        QRectF getVisibleSceneRect() const noexcept;
        bool getUseOpenGl() const noexcept {return mUseOpenGl;}
        const GridProperties& getGridProperties() const noexcept {return *mGridProperties;}

        // Setters
        void setUseOpenGl(bool useOpenGl) noexcept;
        void setGridProperties(const GridProperties& properties) noexcept;
        void setScene(GraphicsScene* scene) noexcept;
        void setVisibleSceneRect(const QRectF& rect) noexcept;
        void setOriginCrossVisible(bool visible) noexcept;
        void setEventHandlerObject(IF_GraphicsViewEventHandler* eventHandler) noexcept;

        // General Methods
        Point mapGlobalPosToScenePos(const QPoint& globalPosPx, bool boundToView,
                                     bool mapToGrid) const noexcept;
        void handleMouseWheelEvent(QGraphicsSceneWheelEvent* event) noexcept;


    public slots:

        // Public Slots
        void zoomIn() noexcept;
        void zoomOut() noexcept;
        void zoomAll() noexcept;


    signals:
        /**
         * @brief Cursor scene position changed signal
         *
         * @param pos   The new cursor position (*not* mapped to grid!)
         */
        void cursorScenePositionChanged(const Point& pos);


    private slots:

        // Private Slots
        void zoomAnimationValueChanged(const QVariant& value) noexcept;


    private:

        // make some methods inaccessible...
        GraphicsView(const GraphicsView& other) = delete;
        GraphicsView& operator=(const GraphicsView& rhs) = delete;

        // Inherited Methods
        bool eventFilter(QObject* obj, QEvent* event);
        void drawBackground(QPainter* painter, const QRectF& rect);
        void drawForeground(QPainter* painter, const QRectF& rect);


        // General Attributes
        IF_GraphicsViewEventHandler* mEventHandlerObject;
        GraphicsScene* mScene;
        QVariantAnimation* mZoomAnimation;
        GridProperties* mGridProperties;
        bool mOriginCrossVisible;
        bool mUseOpenGl;
        volatile bool mPanningActive;
        QCursor mCursorBeforePanning;

        // Static Variables
        static constexpr qreal sZoomStepFactor = 1.3;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_GRAPHICSVIEW_H

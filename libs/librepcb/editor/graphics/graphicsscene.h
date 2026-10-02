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

#ifndef LIBREPCB_EDITOR_GRAPHICSSCENE_H
#define LIBREPCB_EDITOR_GRAPHICSSCENE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/enums.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Struct RectSelection
 ******************************************************************************/

/**
 * @brief Parameters of a rubber-band (rectangle) selection
 *
 * Decides whether a piece of geometry is selected by a selection rectangle.
 * There are two modes:
 *
 *  - Crossing: Geometry is selected if any part of it touches the rectangle.
 *  - Window: Geometry is selected only if it is entirely within the rectangle.
 *
 * Items are tested with their footprint. This is the visible geometry of an
 * item (see \c getVisibleShape() of the item classes) or, for items without
 * such a method, just their shape. Composite items add the footprints of their
 * parts (i.e., a symbol adds its pins and fields; a device adds its pads).
 * Both modes use the same footprint.
 */
struct RectSelection {
  enum class Mode {
    Crossing,  ///< Select items which touch the rectangle.
    Window,  ///< Select items which are completely enclosed by the rectangle.
  };

  QRectF rect;
  Mode mode;
};

/**
 * @brief Check if a footprint is hit by a selection rectangle
 *
 * @param selection   The selection rectangle and mode to test against.
 * @param scenePath   The footprint in scene coordinates (in pixels). An
 *                    empty path (e.g. of an item on a hidden layer) is
 *                    never hit, in any mode.
 * @retval true   The footprint is selected by the rectangle.
 * @retval false  The footprint is not selected by the rectangle.
 */
bool hits(const RectSelection& selection,
          const QPainterPath& scenePath) noexcept;

/**
 * @brief Check if the shape of an item is hit by a selection rectangle
 *
 * @param selection   The selection rectangle and mode to test against.
 * @param item        The item whose shape() is used as its footprint.
 * @retval true   The item is selected by the rectangle.
 * @retval false  The item is not selected by the rectangle.
 */
bool hits(const RectSelection& selection, const QGraphicsItem& item) noexcept;

/**
 * @brief Check if a composite footprint is hit by a selection rectangle
 *
 * The footprint consists of several parts, e.g. a symbol plus its pins
 * and fields. In crossing mode it is hit if any part is
 * touched by the rectangle, in window mode it is hit only if all parts are
 * entirely inside the rectangle. Empty parts are ignored, and a footprint
 * without any geometry is never hit.
 *
 * @param selection   The selection rectangle and mode to test against.
 * @param scenePaths  The parts of the footprint in scene coordinates.
 * @retval true   The footprint is selected by the rectangle.
 * @retval false  The footprint is not selected by the rectangle.
 */
bool hits(const RectSelection& selection,
          const QVector<QPainterPath>& scenePaths) noexcept;

/**
 * @brief Get the footprint of an item in scene coordinates
 *
 * @tparam T      Item type providing `getVisibleShape()`.
 * @param item    The item whose visible shape is used as its footprint.
 * @return The footprint (or one of its parts), to be passed to #hits().
 */
template <typename T>
QPainterPath footprintOf(const T& item) noexcept {
  return item.mapToScene(item.getVisibleShape());
}

/**
 * @brief Check if the visible shape of an item is hit by a selection
 *        rectangle
 *
 * @tparam T      Item type providing `getVisibleShape()`.
 * @param selection   The selection rectangle and mode to test against.
 * @param item    The item whose visible shape is used as its footprint.
 * @retval true   The item is selected by the rectangle.
 * @retval false  The item is not selected by the rectangle.
 */
template <typename T>
bool hitsVisible(const RectSelection& selection, const T& item) noexcept {
  return hits(selection, footprintOf(item));
}

/**
 * @brief Resolve the rubber-band selection mode from raw screen-space drag
 *        positions
 *
 * Dragging to the right (from \p downScreenPos to \p screenPos) gives a
 * window selection. Dragging to the left (or a purely vertical drag) gives a
 * crossing selection. Evaluated directly in screen space.
 *
 * @param downScreenPos   Raw, pre-mirror screen-space position of the
 *                        initial left mouse button press.
 * @param screenPos       Raw, pre-mirror screen-space position of the
 *                        current mouse event.
 * @return  The resolved selection mode.
 */
inline RectSelection::Mode rectSelectionModeFromScreenDrag(
    const QPointF& downScreenPos, const QPointF& screenPos) noexcept {
  return (screenPos.x() > downScreenPos.x()) ? RectSelection::Mode::Window
                                             : RectSelection::Mode::Crossing;
}

/*******************************************************************************
 *  Event Data Structs
 ******************************************************************************/

struct GraphicsSceneMouseEvent {
  Point scenePos;
  Point downPos;
  Qt::MouseButtons buttons = Qt::MouseButtons();
  Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers();

  RectSelection::Mode rectSelectionMode = RectSelection::Mode::Crossing;
};

struct GraphicsSceneKeyEvent {
  Qt::Key key = Qt::Key(0);
  Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers();
};

/*******************************************************************************
 *  Class GraphicsScene
 ******************************************************************************/

/**
 * @brief The GraphicsScene class
 */
class GraphicsScene : public QGraphicsScene {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit GraphicsScene(QObject* parent = nullptr) noexcept;
  ~GraphicsScene() noexcept override;

  // Getters
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }
  GridStyle getGridStyle() const noexcept { return mGridStyle; }

  // Setters
  void setBackgroundColors(const QColor& fill, const QColor& grid) noexcept;
  void setOverlayColors(const QColor& fill, const QColor& content) noexcept;
  void setSelectionRectColors(const QColor& line, const QColor& fill) noexcept;
  void setGridStyle(GridStyle style) noexcept;
  void setGridInterval(const PositiveLength& interval) noexcept;
  void setOriginCrossVisible(bool visible) noexcept;
  void setGrayOut(bool grayOut) noexcept;

  // General Methods
  void setSelectionRect(const Point& p1, const Point& p2) noexcept;
  void setSelectionRect(const Point& p1, const Point& p2,
                        RectSelection::Mode mode) noexcept;
  void clearSelectionRect() noexcept;

  /**
   * @brief Update the selection state of items during a rubber-band selection
   *
   * Shows the selection rectangle (dashed for crossing, solid for window
   * mode) and lets the derived scene select or deselect its items by calling
   * #applyRectSelection().
   *
   * @param p1    One corner of the selection rectangle.
   * @param p2    The opposite corner of the selection rectangle.
   * @param mode  Whether to select touched (crossing) or enclosed (window)
   *              items.
   */
  void selectItemsInRect(const Point& p1, const Point& p2,
                         RectSelection::Mode mode) noexcept;

  /**
   * @brief Setup the marker for a specific scene rect
   *
   * This is intended to mark a specific area in a scene, with a line starting
   * from the top left of the view, so the user can easily locate the specified
   * area, even if it is very small.
   *
   * @param rect    The rect to mark. Pass an empty rect to clear the marker.
   */
  void setSceneRectMarker(const QRectF& rect) noexcept;
  void setSceneCursor(const Point& pos, bool cross, bool circle) noexcept;
  void setRulerPositions(
      const std::optional<std::pair<Point, Point>>& pos) noexcept;

  void addItem(QGraphicsItem& item) noexcept;
  void removeItem(QGraphicsItem& item) noexcept;

  QPixmap toPixmap(int dpi,
                   const QColor& background = Qt::transparent) noexcept;
  QPixmap toPixmap(const QSize& size,
                   const QColor& background = Qt::transparent) noexcept;

protected:
  /**
   * @brief Select or deselect the items of the scene by a selection rectangle
   *
   * Called by #selectItemsInRect(). Derived scenes which support rubber-band
   * selection must set the selection state of all their items, using
   * RectSelection::hits() to test them. The default implementation does
   * nothing.
   *
   * @param selection   The rubber-band selection to apply.
   */
  virtual void applyRectSelection(const RectSelection& selection) noexcept;

  void drawBackground(QPainter* painter, const QRectF& rect) noexcept override;
  void drawForeground(QPainter* painter, const QRectF& rect) noexcept override;

private:
  GridStyle mGridStyle;
  PositiveLength mGridInterval;
  QColor mBackgroundColor;
  QColor mGridColor;
  QColor mOverlayFillColor;
  QColor mOverlayContentColor;
  QRectF mSceneRectMarker;
  bool mOriginCrossVisible;
  bool mGrayOut;

  std::unique_ptr<QGraphicsRectItem> mSelectionRectItem;

  // Overlay scene cursor
  Point mSceneCursorPos;
  bool mSceneCursorCross;
  bool mSceneCursorCircle;

  // Configuration for the ruler overlay
  struct RulerGauge {
    int xScale;
    LengthUnit unit;
    QString unitSeparator;
    Length minTickInterval;
    Length currentTickInterval;
  };
  QVector<RulerGauge> mRulerGauges;
  std::optional<std::pair<Point, Point>> mRulerPositions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

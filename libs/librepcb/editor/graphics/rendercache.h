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

#ifndef LIBREPCB_EDITOR_RENDERCACHE_H
#define LIBREPCB_EDITOR_RENDERCACHE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;

/*******************************************************************************
 *  Class RenderCache
 ******************************************************************************/

/**
 * @brief Pan and zoom cache for the rendered content of a graphics scene
 *
 * The scene content of the view plus a surrounding margin is kept in a 2D ring
 * buffer addressed by wrapped device coordinates (device pixels at the cached
 * scale, independent of the pan offset). Panning therefore never moves cached
 * pixels; newly exposed strips overwrite the area which has left the view.
 *
 * Content changes reported by the scene are re-rendered in place. While the
 * scale differs from the cached one, a scaled preview of the cached pixels is
 * provided until the zoom has settled, after which the cache is rebuilt at
 * the new scale. The margin is completed in idle time.
 *
 * The scene's foreground layer is not part of the cache; the owner draws it
 * over the returned content.
 */
class RenderCache final {
public:
  /// Draws into the given paint device.
  using PaintFunction = std::function<void(QPaintDevice*)>;

  /**
   * @brief Provides the surface for rendering a region
   *
   * Runs the paint function on a paint device of the given size and returns
   * the result, or a null image on failure. Called from ::render() and from
   * the idle refill, so it must make any required rendering context current
   * itself.
   */
  using SurfaceRenderer =
      std::function<QImage(const QSize& size, const PaintFunction& paint)>;

  /// Requests the owner to trigger a repaint, i.e. call ::render() again.
  using RepaintRequest = std::function<void()>;

  // Constructors / Destructor
  RenderCache() = delete;
  RenderCache(const RenderCache& other) = delete;
  RenderCache(SurfaceRenderer renderer, RepaintRequest repaint) noexcept;
  ~RenderCache() noexcept;

  // General Methods

  /**
   * @brief Get the scene content of the view
   *
   * Renders whatever is missing or dirty, then copies the view from the cache.
   * While the scale differs from the cached one, a scaled preview of the
   * cached pixels is drawn instead and the repaint request is issued once the
   * zoom has settled.
   *
   * @param scene   The scene to render.
   * @param pos     Top-left of the view in whole device pixels at `scale`,
   *                see ::snapPos().
   * @param size    Size of the view in device pixels.
   * @param scale   Device pixels per scene pixel.
   * @param mirror  Whether the view is mirrored horizontally.
   * @param out     Image of `size` in format `Format_RGBA8888_Premultiplied`
   *                to be filled with the scene content (no foreground).
   *
   * @retval true   `out` contains the view.
   * @retval false  The surface renderer failed; `out` is undefined.
   */
  bool render(GraphicsScene& scene, const QPoint& pos, const QSize& size,
              qreal scale, bool mirror, QImage& out) noexcept;

  /**
   * @brief Drop all cached pixels and cancel pending work
   */
  void invalidate() noexcept;

  // Static Methods

  /**
   * @brief Get the view position to be passed to ::render()
   *
   * @param pos      Top-left of the view in device pixels at the view scale.
   * @param snapped  Receives `pos` rounded to whole device pixels.
   *
   * @retval true   `snapped` is valid.
   * @retval false  `pos` is not supported by the cache. Do not render it.
   */
  static bool snapPos(const QPointF& pos, QPoint& snapped) noexcept;

  /**
   * @brief Wrap an index into `[0, size)`
   *
   * @param value  Index, may be negative.
   * @param size   Ring size, must be positive.
   */
  static int wrapIndex(int value, int size) noexcept;

  /**
   * @brief Get the parts of a rect which are not covered by another rect
   *
   * @param outer  The rect to subtract from.
   * @param inner  The rect to subtract.
   *
   * @return Disjoint rects covering exactly `outer` minus `inner`.
   */
  static QList<QRect> subtractRect(const QRect& outer,
                                   const QRect& inner) noexcept;

  /**
   * @brief Get the largest rect covered by two overlapping rects
   *
   * @param old   The rect covered so far.
   * @param view  The rect which must be contained in the result.
   *
   * @return Largest rectangle inside the union of both which contains `view`.
   */
  static QRect mergeValid(const QRect& old, const QRect& view) noexcept;

  // Operator Overloadings
  RenderCache& operator=(const RenderCache& rhs) = delete;

private:  // Methods
  bool renderAndStore(GraphicsScene& scene, const QRect& region) noexcept;
  QImage renderRegion(GraphicsScene& scene, const QRect& region) noexcept;
  void storeRegion(const QRect& region, QImage img) noexcept;
  void cropRegion(const QRect& region, QImage& out) const noexcept;
  void applyDirtyRects(GraphicsScene& scene,
                       const QList<QRectF>& dirty) noexcept;
  void drawZoomPreview(const GraphicsScene& scene, const QRect& view,
                       qreal scale, const QRectF& src,
                       QImage& out) const noexcept;
  void refill() noexcept;
  void settle() noexcept;

private:  // Data
  const SurfaceRenderer mRenderer;
  const RepaintRequest mRepaint;
  QImage mRing;  ///< Wrapped storage, size of the view plus margins.
  QRect mValid;  ///< Rendered device rect, always inside ::mView plus margins.
  QRect mView;  ///< Device rect of the view at the last frame.
  qreal mScale;
  bool mMirror;
  QPointer<GraphicsScene> mScene;
  QTimer mRefillTimer;
  QTimer mZoomSettleTimer;
  bool mZoomSettled;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

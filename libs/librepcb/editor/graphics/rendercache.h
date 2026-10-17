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
 * buffer addressed by wrapped coordinates. Panning therefore never moves
 * cached pixels; areas newly exposed by the pan overwrite the area which has
 * left the view. The margin is rendered in idle time.
 *
 * Rendered areas are transformed using a caller-provided transform to account
 * for mirror and scaling operations performed by the view itself.
 *
 * Content changes reported by the scene are re-rendered in place. During zoom
 * operations, a scaled preview of the cached pixels is used until the zoom has
 * settled, after which the cache is rebuilt at the new zoom level.
 *
 * The scene's foreground layer is not part of the cache; the owner draws it
 * over the returned content.
 */
class RenderCache final {
public:
  /**
   * @brief Render scene content into a new image
   *
   * Called from ::render() and from the idle refill.
   *
   * @param scene          The scene to render.
   * @param sceneToDevice  Maps scene coordinates to pixels of the image.
   * @param size           Size of the image in pixels.
   *
   * @return The rendered content, or a null image on failure.
   */
  using RegionRenderer =
      std::function<QImage(GraphicsScene& scene,
                           const QTransform& sceneToDevice, const QSize& size)>;

  /// Requests the owner to trigger a repaint, i.e. call ::render() again.
  using RepaintRequest = std::function<void()>;

  // Constructors / Destructor
  RenderCache() = delete;
  RenderCache(const RenderCache& other) = delete;
  RenderCache(RegionRenderer renderer, RepaintRequest repaint) noexcept;
  ~RenderCache() noexcept;

  // General Methods

  /**
   * @brief Render view content
   *
   * Renders whatever is missing or dirty, then copies the view from the cache.
   * During zoom operations, a scaled preview of the cached pixels is drawn. A
   * repaint request is issued once the zoom has settled.
   *
   * @param scene          The scene to render.
   * @param sceneToCache   Maps scene coordinates to cache space. Only a
   *                       uniform scale with an optional horizontal flip is
   *                       supported, without any translation.
   * @param view           Rect of the view in cache space, in whole pixels
   *                       (see ::snapPos()).
   * @param backdrop       Color shown where a preview has no cached pixels.
   * @param out            Image of the size of `view` in format
   *                       `Format_RGBA8888_Premultiplied` to be filled with
   *                       the scene content.
   *
   * @retval true   `out` contains the view.
   * @retval false  The transform is not supported or the region renderer
   *                failed; `out` is undefined.
   */
  bool render(GraphicsScene& scene, const QTransform& sceneToCache,
              const QRect& view, const QColor& backdrop, QImage& out) noexcept;

  /**
   * @brief Drop all cached pixels and cancel pending work
   */
  void invalidate() noexcept;

  // Static Methods

  /**
   * @brief Check whether ::render() supports a scene-to-cache transform
   */
  static bool isSupported(const QTransform& transform) noexcept;

  /**
   * @brief Get the view position to be used for ::render()
   *
   * @param pos      Top-left of the view in pixels of the cache space.
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
  void drawZoomPreview(const QColor& backdrop, const QRect& view, qreal scale,
                       const QRectF& src, QImage& out) const noexcept;
  void refill() noexcept;
  void settle() noexcept;

private:  // Data
  const RegionRenderer mRenderer;
  const RepaintRequest mRepaint;
  QImage mRing;  ///< Wrapped storage, size of the view plus margins.
  QRect mValid;  ///< Rendered device rect, always inside ::mView plus margins.
  QRect mView;  ///< Device rect of the view at the last frame.
  QTransform mTransform;  ///< Maps pixels from scene- to cache-space.
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

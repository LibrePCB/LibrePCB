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

#ifndef LIBREPCB_CORE_TRANSFORM_H
#define LIBREPCB_CORE_TRANSFORM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../graphics/graphicslayername.h"
#include "../types/angle.h"
#include "../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Transform
 ******************************************************************************/

/**
 * @brief Helper class to perform coordinate transformation with various types
 *
 * The class is similar to `QTransform`, but with the following differences:
 *
 *   - It transforms mainly LibrePCB types instead of Qt types (e.g.
 *     ::librepcb::Point instead of `QPointF`).
 *   - It does not allow arbitrary transformations, but only mirror, rotate
 *     and translate. The order of the transformation is not configurable, it
 *     is hardcoded to the order of transformations applied to symbols within
 *     a schematic, and to footprints within a board. This order is:
 *     rotate CCW -> mirror horizontally (negating X-coordinate) -> translate.
 *
 * Long story short, this class converts symbol- or footprint coordinates
 * into schematic- resp. board coordinates.
 */
class Transform {
public:
  // Constructors / Destructor

  /**
   * @brief (Default) construdtor
   *
   * @param position  Transformation position.
   * @param rotation  Transformation rotation.
   * @param mirrored  Transformation mirror state.
   */
  Transform(const Point& position = Point(0, 0),
            const Angle& rotation = Angle(0), bool mirrored = false) noexcept
    : mPosition(position), mRotation(rotation), mMirrored(mirrored) {}

  /**
   * @brief Constructor to copy the transformation of an object
   *
   * @tparam T    Any type which provides the methods `Point get Position()`,
   *              `Angle getRotation()` and `bool getMirrored()`.
   * @param obj   Any object of the given type. The transformation is copied
   *              from the provided object.
   */
  template <typename T>
  explicit Transform(const T& obj)
    : mPosition(obj.getPosition()),
      mRotation(obj.getRotation()),
      mMirrored(obj.getMirrored()) {}

  /**
   * @brief Copy constructor
   *
   * @param other Object to copy.
   */
  Transform(const Transform& other) noexcept
    : mPosition(other.mPosition),
      mRotation(other.mRotation),
      mMirrored(other.mMirrored) {}

  /**
   * @brief Destructor
   */
  ~Transform() noexcept {}

  // Getters
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  bool getMirrored() const noexcept { return mMirrored; }

  // Setters
  void setPosition(const Point& position) noexcept { mPosition = position; }
  void setRotation(const Angle& rotation) noexcept { mRotation = rotation; }
  void setMirrored(bool mirrored) noexcept { mMirrored = mirrored; }

  // General Methods

  /**
   * @brief Map a given mirror state to the transformed coordinate system
   *
   * @param mirror  The mirror state to map.
   * @return The passed mirror state inverted if the transformation is
   *         mirroring, otherwise not inverted.
   */
  bool map(bool mirror) const noexcept;

  /**
   * @brief Map a given angle to the transformed coordinate system
   *
   * @param angle The angle to map.
   * @return The passed angle, rotated by the transformations rotation, and
   *         mirrored horizontally if the transformation is mirroring.
   */
  Angle map(const Angle& angle) const noexcept;

  /**
   * @brief Map a given point to the transformed coordinate system
   *
   * @param point The point to map.
   * @return The passed point, rotated by the transformations rotation,
   *         mirrored horizontally if the transformation is mirroring, and
   *         translated by the transformation offset.
   */
  Point map(const Point& point) const noexcept;

  /**
   * @brief Map a given path to the transformed coordinate system
   *
   * @param path  The path to map.
   * @return The passed path, rotated by the transformations rotation,
   *         mirrored horizontally if the transformation is mirroring, and
   *         translated by the transformation offset.
   */
  Path map(const Path& path) const noexcept;

  /**
   * @brief Map a given path to the transformed coordinate system
   *
   * @param path  The path to map.
   * @return The passed path, rotated by the transformations rotation,
   *         mirrored horizontally if the transformation is mirroring, and
   *         translated by the transformation offset.
   */
  NonEmptyPath map(const NonEmptyPath& path) const noexcept;

  /**
   * @brief Map a given layer name to the transformed coordinate system
   *
   * @param layerName The layer to map.
   * @return The mirrored layer name if it's a symetric layer and the
   *         transformation is mirroring, otherwise the layer is returned as-is.
   */
  QString map(const QString& layerName) const noexcept;

  /**
   * @brief Map a given layer name to the transformed coordinate system
   *
   * @param layerName The layer to map.
   * @return The mirrored layer name if it's a symetric layer and the
   *         transformation is mirroring, otherwise the layer is returned as-is.
   */
  GraphicsLayerName map(const GraphicsLayerName& layerName) const noexcept;

  /**
   * @brief Map all items of a container to the transformed coordinate system
   *
   * @tparam Container type.
   * @param container The items to map.
   * @return The passed items, rotated by the transformations rotation,
   *         mirrored horizontally if the transformation is mirroring, and
   *         translated by the transformation offset.
   */
  template <typename T>
  T map(const T& container) const noexcept {
    T copy = container;
    for (auto& item : copy) {
      item = map(item);
    }
    return copy;
  }

  // Operator Overloadings
  bool operator==(const Transform& rhs) const noexcept {
    return (mPosition == rhs.mPosition) && (mRotation == rhs.mRotation) &&
        (mMirrored == rhs.mMirrored);
  }
  bool operator!=(const Transform& rhs) const noexcept {
    return !(*this == rhs);
  }
  Transform& operator=(const Transform& rhs) noexcept {
    mPosition = rhs.mPosition;
    mRotation = rhs.mRotation;
    mMirrored = rhs.mMirrored;
    return *this;
  }

private:  // Data
  Point mPosition;
  Angle mRotation;
  bool mMirrored;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_PROJECT_EDITOR_VIATOOLBAR_H
#define LIBREPCB_PROJECT_EDITOR_VIATOOLBAR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../project/boards/items/bi_via.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PositiveLengthEdit;

namespace project {
namespace editor {

/*******************************************************************************
 *  Class ViaShapeSelector
 ******************************************************************************/

/**
 * @brief The ViaShapeSelector class provides a panel to control the shape, size
 * and drill diameter of a via.
 */
class ViaShapeSelector final : public QToolBar {
  Q_OBJECT

public:
  explicit ViaShapeSelector(QWidget* parent = nullptr) noexcept;
  ViaShapeSelector(const ViaShapeSelector& other) = delete;

  // Setters
  /**
   * @brief Set the via shape to one of the available in BI_Via::Shape
   * @param shape The new shape.
   * @note If the shape is not available for selection (e.g
   * BI_Via::Shape::Count), the current shape is kept and nothing happens.
   */
  void setShape(const BI_Via::Shape shape) noexcept;

  /**
   * @brief Set the via size
   * @param size The new size.
   */
  void setSize(const PositiveLength& size) noexcept;

  /**
   * @brief Change the via size by a certain amount of steps.
   *
   * @param steps The amount of steps that the via size is increased or
   * decreased by.
   */
  void stepSize(const int steps) noexcept;

  /**
   * @brief Set the via drill diameter
   * @param drill The new drill diameter.
   */
  void setDrill(const PositiveLength& drill) noexcept;

  /**
   * @brief Change the drill diameter by a certain amount of steps.
   *
   * @param steps The amount of steps that the drill diameter is increased
   * or decreased by.
   */
  void stepDrill(const int steps) noexcept;

  // Operator Overloadings
  ViaShapeSelector& operator=(const ViaShapeSelector& rhs) = delete;

signals:
  void shapeChanged(const BI_Via::Shape shape);
  void sizeChanged(const PositiveLength& value);
  void drillChanged(const PositiveLength& value);

private:  // Data
  QMap<BI_Via::Shape, QToolButton*> mButtons;
  PositiveLengthEdit*               mSizeEdit;
  PositiveLengthEdit*               mDrillEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_VIATOOLBAR_H

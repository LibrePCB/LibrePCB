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

#ifndef LIBREPCB_LIBRARY_EDITOR_SHAPESELECTOR_H
#define LIBREPCB_LIBRARY_EDITOR_SHAPESELECTOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/footprintpad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PositiveLengthEdit;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class PadShapeSelector
 ******************************************************************************/

/**
 * @brief The PadShapeSelector class provides a panel to control the shape and
 * size of a footprint pad.
 */
class PadShapeSelector final : public QToolBar {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PadShapeSelector(const LengthUnit defaultUnit,
                            QWidget*         parent = nullptr) noexcept;
  PadShapeSelector(const PadShapeSelector& other) = delete;

  // Setters
  /**
   * @brief Set the pad shape to one available in FootprintPad::Shape
   * @param shape The new shape.
   * @note If the shape is not available for selection (e.g
   * BI_Via::Shape::COUNT), the current shape is kept and nothing happens.
   */
  void setShape(const FootprintPad::Shape shape) noexcept;

  /**
   * @brief Set the pad width
   * @param width The new pad width.
   */
  void setWidth(const PositiveLength& width) noexcept;

  /**
   * @brief Set the pad height
   * @param height The new pad height.
   */
  void setHeight(const PositiveLength& height) noexcept;

  // Operator Overloadings
  PadShapeSelector& operator=(const PadShapeSelector& rhs) = delete;

signals:
  void shapeChanged(const FootprintPad::Shape shape);
  void widthChanged(const PositiveLength& width);
  void heightChanged(const PositiveLength& height);

private:  // Data
  QMap<FootprintPad::Shape, QToolButton*> mButtons;
  PositiveLengthEdit*                     mWidthEdit;
  PositiveLengthEdit*                     mHeightEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SHAPESELECTOR_H

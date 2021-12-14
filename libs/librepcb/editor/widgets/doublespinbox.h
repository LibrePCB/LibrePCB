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

#ifndef LIBREPCB_EDITOR_DOUBLESPINBOX_H
#define LIBREPCB_EDITOR_DOUBLESPINBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class DoubleSpinBox
 ******************************************************************************/

/**
 * @brief The DoubleSpinBox class is a customized QDoubleSpinBox widget
 *
 * Differences to QDoubleSpinBox:
 *   - Trailing zeros are omitted
 */
class DoubleSpinBox final : public QDoubleSpinBox {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit DoubleSpinBox(QWidget* parent = nullptr) noexcept;
  DoubleSpinBox(const DoubleSpinBox& other) = delete;
  virtual ~DoubleSpinBox() noexcept;

  // Operator Overloadings
  DoubleSpinBox& operator=(const DoubleSpinBox& rhs) = delete;

  // Inherited Methods
  virtual QString textFromValue(double val) const override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

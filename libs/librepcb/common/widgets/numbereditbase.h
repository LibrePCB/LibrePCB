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

#ifndef LIBREPCB_NUMBEREDITBASE_H
#define LIBREPCB_NUMBEREDITBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class DoubleSpinBox;

/*******************************************************************************
 *  Class NumberEditBase
 ******************************************************************************/

/**
 * @brief The NumberEditBase class is a widget base class to edit various kinds
 *        of numbers
 *
 * See subclasses for details.
 */
class NumberEditBase : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit NumberEditBase(QWidget* parent = nullptr) noexcept;
  NumberEditBase(const NumberEditBase& other) = delete;
  virtual ~NumberEditBase() noexcept;

  // General Methods
  void setSingleStep(tl::optional<double> step) noexcept;
  void setFrame(bool frame) noexcept;
  void selectAll() noexcept;

  // Operator Overloadings
  NumberEditBase& operator=(const NumberEditBase& rhs) = delete;

signals:
  void editingFinished();

protected:  // Methods
  virtual void updateSpinBox() noexcept = 0;
  virtual void spinBoxValueChanged(double value) noexcept = 0;

protected:  // Data
  QScopedPointer<DoubleSpinBox> mSpinBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_NUMBEREDITBASE_H

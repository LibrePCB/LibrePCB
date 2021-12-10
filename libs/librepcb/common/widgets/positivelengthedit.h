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

#ifndef LIBREPCB_COMMON_POSITIVELENGTHEDIT_H
#define LIBREPCB_COMMON_POSITIVELENGTHEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "lengtheditbase.h"

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PositiveLengthEdit
 ******************************************************************************/

/**
 * @brief The PositiveLengthEdit class is a widget to view/edit
 *        ::librepcb::PositiveLength values
 */
class PositiveLengthEdit final : public LengthEditBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PositiveLengthEdit(QWidget* parent = nullptr) noexcept;
  PositiveLengthEdit(const PositiveLengthEdit& other) = delete;
  virtual ~PositiveLengthEdit() noexcept;

  // Getters
  PositiveLength getValue() const noexcept;

  // Setters
  void setValue(const PositiveLength& value) noexcept;

  // Operator Overloadings
  PositiveLengthEdit& operator=(const PositiveLengthEdit& rhs) = delete;

signals:
  // Note: Full namespace librepcb::PositiveLength is required for the MOC!
  void valueChanged(const librepcb::PositiveLength& value);

private:
  virtual void valueChangedImpl() noexcept override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

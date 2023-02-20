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

#ifndef LIBREPCB_EDITOR_UNSIGNEDLIMITEDRATIOEDIT_H
#define LIBREPCB_EDITOR_UNSIGNEDLIMITEDRATIOEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../widgets/numbereditbase.h"

#include <librepcb/core/types/ratio.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class UnsignedLimitedRatioEdit
 ******************************************************************************/

/**
 * @brief The UnsignedLimitedRatioEdit class is a widget to view/edit
 *        ::librepcb::UnsignedLimitedRatio values
 */
class UnsignedLimitedRatioEdit final : public NumberEditBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit UnsignedLimitedRatioEdit(QWidget* parent = nullptr) noexcept;
  UnsignedLimitedRatioEdit(const UnsignedLimitedRatioEdit& other) = delete;
  virtual ~UnsignedLimitedRatioEdit() noexcept;

  // Getters
  const UnsignedLimitedRatio& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const UnsignedLimitedRatio& value) noexcept;

  // Operator Overloadings
  UnsignedLimitedRatioEdit& operator=(const UnsignedLimitedRatioEdit& rhs) =
      delete;

signals:
  void valueChanged(const UnsignedLimitedRatio& value);

private:  // Methods
  void updateSpinBox() noexcept override;
  void spinBoxValueChanged(double value) noexcept override;

private:  // Data
  UnsignedLimitedRatio mMinValue;
  UnsignedLimitedRatio mMaxValue;
  UnsignedLimitedRatio mValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

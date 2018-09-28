/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_SIGNALROLECOMBOBOX_H
#define LIBREPCB_SIGNALROLECOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../signalrole.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SignalRoleComboBox
 ******************************************************************************/

/**
 * @brief The SignalRoleComboBox class
 *
 * @author ubruhin
 * @date 2016-10-20
 */
class SignalRoleComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit SignalRoleComboBox(QWidget* parent = nullptr) noexcept;
  SignalRoleComboBox(const SignalRoleComboBox& other) = delete;
  ~SignalRoleComboBox() noexcept;

  // Getters
  SignalRole getCurrentItem() const noexcept;

  // Setters
  void setCurrentItem(const SignalRole& role) noexcept;

  // Operator Overloadings
  SignalRoleComboBox& operator=(const SignalRoleComboBox& rhs) = delete;

signals:
  void currentItemChanged(const SignalRole& role);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SIGNALROLECOMBOBOX_H

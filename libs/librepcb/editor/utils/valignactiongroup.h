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

#ifndef LIBREPCB_COMMON_VALIGNACTIONGROUP_H
#define LIBREPCB_COMMON_VALIGNACTIONGROUP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../alignment.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class VAlignActionGroup
 ******************************************************************************/

/**
 * @brief The VAlignActionGroup class is a helper to add ::librepcb::VAlign
 *        chooser toolbuttons to a toolbar
 *
 * @see ::librepcb::HAlignActionGroup
 */
class VAlignActionGroup final : public QActionGroup {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit VAlignActionGroup(QWidget* parent = nullptr) noexcept;
  VAlignActionGroup(const VAlignActionGroup& other) = delete;
  virtual ~VAlignActionGroup() noexcept;

  // Getters
  const VAlign& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const VAlign& value) noexcept;

  // Operator Overloadings
  VAlignActionGroup& operator=(const VAlignActionGroup& rhs) = delete;

signals:
  void valueChanged(const VAlign& value);

private:  // Methods
  void updateSelection() noexcept;
  void actionTriggered(QAction* action) noexcept;

private:  // Data
  VAlign mValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif

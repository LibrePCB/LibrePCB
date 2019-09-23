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

#ifndef LIBREPCB_HALIGNACTIONGROUP_H
#define LIBREPCB_HALIGNACTIONGROUP_H

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
 *  Class HAlignActionGroup
 ******************************************************************************/

/**
 * @brief The HAlignActionGroup class is a helper to add ::librepcb::HAlign
 *        chooser toolbuttons to a toolbar
 *
 * @see ::librepcb::VAlignActionGroup
 */
class HAlignActionGroup final : public QActionGroup {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit HAlignActionGroup(QWidget* parent = nullptr) noexcept;
  HAlignActionGroup(const HAlignActionGroup& other) = delete;
  virtual ~HAlignActionGroup() noexcept;

  // Getters
  const HAlign& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const HAlign& value) noexcept;

  // Operator Overloadings
  HAlignActionGroup& operator=(const HAlignActionGroup& rhs) = delete;

signals:
  void valueChanged(const HAlign& value);

private:  // Methods
  void updateSelection() noexcept;
  void actionTriggered(QAction* action) noexcept;

private:  // Data
  HAlign mValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_HALIGNACTIONGROUP_H

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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "valignactiongroup.h"

#include "../editorcommandset.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

VAlignActionGroup::VAlignActionGroup(QWidget* parent) noexcept
  : QActionGroup(parent), mValue(VAlign::bottom()) {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  QAction* bottom = cmd.alignVerticalBottom.createAction(this);
  bottom->setCheckable(true);
  bottom->setData(QVariant::fromValue(VAlign::bottom()));

  QAction* center = cmd.alignVerticalCenter.createAction(this);
  center->setCheckable(true);
  center->setData(QVariant::fromValue(VAlign::center()));

  QAction* top = cmd.alignVerticalTop.createAction(this);
  top->setCheckable(true);
  top->setData(QVariant::fromValue(VAlign::top()));

  updateSelection();

  connect(this, &VAlignActionGroup::triggered, this,
          &VAlignActionGroup::actionTriggered);
}

VAlignActionGroup::~VAlignActionGroup() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void VAlignActionGroup::setValue(const VAlign& value) noexcept {
  if (value != mValue) {
    mValue = value;
    updateSelection();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void VAlignActionGroup::updateSelection() noexcept {
  foreach (QAction* action, actions()) {
    if (action->data().value<VAlign>() == mValue) {
      action->setChecked(true);
      return;
    }
  }
  Q_ASSERT(false);
}

void VAlignActionGroup::actionTriggered(QAction* action) noexcept {
  Q_ASSERT(action);
  Q_ASSERT(action->data().canConvert<VAlign>());
  VAlign value = action->data().value<VAlign>();
  if (value != mValue) {
    mValue = value;
    emit valueChanged(mValue);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

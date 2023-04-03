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
#include "exclusiveactiongroup.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ExclusiveActionGroup::ExclusiveActionGroup() noexcept : QObject(nullptr) {
}

ExclusiveActionGroup::~ExclusiveActionGroup() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ExclusiveActionGroup::reset() noexcept {
  setCurrentAction(-1);
  setEnabled(false);
}

void ExclusiveActionGroup::setEnabled(bool enabled) noexcept {
  for (auto it = mActions.begin(); it != mActions.end(); it++) {
    foreach (auto& pair, it.value()) {
      if (pair.first) pair.first->setEnabled(enabled);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ExclusiveActionGroup::addAction(QPointer<QAction> action, int id,
                                     const QVariant& mode) noexcept {
  Q_ASSERT(id != -1);
  Q_ASSERT(action);
  mActions[id].append(std::make_pair(action, mode));
  connect(action, &QAction::triggered, this,
          &ExclusiveActionGroup::actionTriggeredSlot);
}

void ExclusiveActionGroup::setActionEnabled(int id, bool enabled) noexcept {
  foreach (auto& pair, mActions.value(id)) {
    if (pair.first) pair.first->setEnabled(enabled);
  }
}

void ExclusiveActionGroup::setCurrentAction(int id) noexcept {
  for (auto it = mActions.begin(); it != mActions.end(); it++) {
    Q_ASSERT(!it.value().isEmpty());
    if (QAction* action = it.value().first().first) {
      QSignalBlocker blocker(action);
      action->setCheckable(it.key() == id);
      action->setChecked(it.key() == id);
    }
  }
}

void ExclusiveActionGroup::actionTriggeredSlot() noexcept {
  QAction* action = dynamic_cast<QAction*>(sender());
  Q_ASSERT(action);
  if (action->isCheckable()) {
    QSignalBlocker blocker(action);
    action->setChecked(true);
  } else {
    for (auto it = mActions.begin(); it != mActions.end(); it++) {
      foreach (auto& pair, it.value()) {
        if (pair.first == action) {
          emit actionTriggered(it.key(), pair.second);
          return;
        }
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

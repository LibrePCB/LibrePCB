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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "exclusiveactiongroup.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ExclusiveActionGroup::ExclusiveActionGroup() noexcept :
    QObject(nullptr), mCurrentAction()
{
}

ExclusiveActionGroup::~ExclusiveActionGroup() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ExclusiveActionGroup::reset() noexcept
{
    setCurrentAction(QVariant());
    setEnabled(false);
}

void ExclusiveActionGroup::setEnabled(bool enabled) noexcept
{
    foreach (QAction* action, mActions) {
        if (action) action->setEnabled(enabled);
    }
}

void ExclusiveActionGroup::addAction(const QVariant& key, QAction* action) noexcept
{
    Q_ASSERT(!key.isNull());
    Q_ASSERT(!mActions.contains(key));
    mActions.insert(key, action);
    if (action) {
        connect(action, &QAction::triggered, this, &ExclusiveActionGroup::actionTriggered);
        action->setCheckable(key == mCurrentAction);
        action->setChecked(key == mCurrentAction);
    }
}

void ExclusiveActionGroup::setActionEnabled(const QVariant& key, bool enabled) noexcept
{
    QAction* action = mActions.value(key);
    if (action) action->setEnabled(enabled);
}

void ExclusiveActionGroup::setCurrentAction(const QVariant& key) noexcept
{
    mCurrentAction = key;
    foreach (const QVariant& val, mActions.keys()) {
        QAction* action = mActions.value(val);
        if (action) {
            action->setCheckable(val == mCurrentAction);
            action->setChecked(val == mCurrentAction);
        }
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ExclusiveActionGroup::actionTriggered() noexcept
{
    QAction* action = dynamic_cast<QAction*>(sender()); Q_ASSERT(action);
    QVariant key = mActions.key(action); Q_ASSERT(!key.isNull());
    if (key != mCurrentAction) {
        emit changeRequestTriggered(key);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

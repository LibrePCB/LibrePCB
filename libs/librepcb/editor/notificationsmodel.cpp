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
#include "notificationsmodel.h"

#include "notification.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NotificationsModel::NotificationsModel(Workspace& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mUnreadNotifications(0),
    mCurrentProgressIndex(-1) {
  connect(
      &mWorkspace.getSettings().dismissedMessages,
      &WorkspaceSettingsItem::edited, this,
      [this]() {
        notify_reset();
        updateUnreadNotificationsCount();
        updateCurrentProgressIndex();
      },
      Qt::QueuedConnection);
  updateUnreadNotificationsCount();
  updateCurrentProgressIndex();
}

NotificationsModel::~NotificationsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NotificationsModel::push(
    std::shared_ptr<Notification> notification) noexcept {
  if (std::find(mItems.begin(), mItems.end(), notification) != mItems.end()) {
    return;  // Already added.
  }

  notification->resetState();
  connect(notification.get(), &Notification::changed, this,
          &NotificationsModel::itemChanged);
  mItems.insert(mItems.begin(), notification);

  const QString dismissKey = notification->getDismissKey();
  if (dismissKey.isEmpty() ||
      (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) {
    notify_row_added(0, 1);
    updateUnreadNotificationsCount();
    updateCurrentProgressIndex();
    if (notification->getAutoPopUp()) {
      emit autoPopUpRequested();
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t NotificationsModel::row_count() const {
  std::size_t count = 0;
  for (const auto& item : mItems) {
    const QString dismissKey = item->getDismissKey();
    if (dismissKey.isEmpty() ||
        (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) {
      ++count;
    }
  }
  return count;
}

std::optional<ui::NotificationData> NotificationsModel::row_data(
    std::size_t i) const {
  const int itemIndex = mapIndex(i);
  if ((itemIndex >= 0) && (itemIndex < static_cast<int>(mItems.size()))) {
    return mItems.at(itemIndex)->getUiData();
  } else {
    return std::nullopt;
  }
}

void NotificationsModel::set_row_data(
    std::size_t i, const ui::NotificationData& obj) noexcept {
  const int itemIndex = mapIndex(i);
  if ((itemIndex >= 0) && (itemIndex < static_cast<int>(mItems.size()))) {
    mItems[itemIndex]->setUiData(obj);
    notify_row_changed(i);

    // Handle "don't show again".
    const QString dismissKey = mItems[itemIndex]->getDismissKey();
    if (obj.dont_show_again && (!dismissKey.isEmpty())) {
      try {
        mWorkspace.getSettings().dismissedMessages.add(dismissKey);
        mWorkspace.saveSettings();  // can throw
      } catch (const Exception& e) {
        qCritical().noquote() << "Failed to dismiss message:" << e.getMsg();
      }
    }

    // Handle "dismiss".
    if (obj.dismissed) {
      removeItem(i, itemIndex);
    }

    updateUnreadNotificationsCount();
    updateCurrentProgressIndex();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int NotificationsModel::mapIndex(int i) const noexcept {
  int logicalIndex = 0;
  for (int itemIndex = 0; itemIndex < static_cast<int>(mItems.size());
       ++itemIndex) {
    const QString dismissKey = mItems.at(itemIndex)->getDismissKey();
    if (dismissKey.isEmpty() ||
        (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) {
      if (logicalIndex == i) {
        return itemIndex;
      } else {
        ++logicalIndex;
      }
    }
  }
  return -1;
}

void NotificationsModel::itemChanged(bool dismissed) noexcept {
  const Notification* notification = static_cast<const Notification*>(sender());
  std::size_t logicalIndex = 0;
  for (std::size_t i = 0; i < mItems.size(); ++i) {
    if (mItems.at(i).get() == notification) {
      if (dismissed) {
        removeItem(logicalIndex, i);
      } else {
        notify_row_changed(logicalIndex);
      }
      updateUnreadNotificationsCount();
      updateCurrentProgressIndex();
      return;
    }
    const QString dismissKey = mItems.at(i)->getDismissKey();
    if (dismissKey.isEmpty() ||
        (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) {
      ++logicalIndex;
    }
  }
}

void NotificationsModel::removeItem(std::size_t i, int itemIndex) noexcept {
  disconnect(mItems[itemIndex].get(), &Notification::changed, this,
             &NotificationsModel::itemChanged);
  mItems.erase(mItems.begin() + itemIndex);
  notify_row_removed(i, 1);
}

void NotificationsModel::updateUnreadNotificationsCount() noexcept {
  int count = 0;
  for (const auto& item : mItems) {
    const QString dismissKey = item->getDismissKey();
    if ((dismissKey.isEmpty() ||
         (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) &&
        (item->getUiData().type != ui::NotificationType::Progress) &&
        (item->getUiData().unread)) {
      ++count;
    }
  }
  if (count != mUnreadNotifications) {
    mUnreadNotifications = count;
    emit unreadNotificationsCountChanged(mUnreadNotifications);
  }
}

void NotificationsModel::updateCurrentProgressIndex() noexcept {
  int index = -1;

  int logicalIndex = 0;
  for (const auto& item : mItems) {
    const QString dismissKey = item->getDismissKey();
    if (dismissKey.isEmpty() ||
        (!mWorkspace.getSettings().dismissedMessages.contains(dismissKey))) {
      if (item->getUiData().type == ui::NotificationType::Progress) {
        index = logicalIndex;
        break;
      }
      ++logicalIndex;
    }
  }

  if (index != mCurrentProgressIndex) {
    mCurrentProgressIndex = index;
    emit currentProgressIndexChanged(mCurrentProgressIndex);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

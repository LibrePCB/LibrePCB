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

#ifndef LIBREPCB_EDITOR_NOTIFICATIONSMODEL_H
#define LIBREPCB_EDITOR_NOTIFICATIONSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

#include <memory>
#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class Notification;

/*******************************************************************************
 *  Class NotificationsModel
 ******************************************************************************/

/**
 * @brief The NotificationsModel class
 */
class NotificationsModel final : public QObject,
                                 public slint::Model<ui::NotificationData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  NotificationsModel() = delete;
  NotificationsModel(const NotificationsModel& other) = delete;
  explicit NotificationsModel(Workspace& ws,
                              QObject* parent = nullptr) noexcept;
  ~NotificationsModel() noexcept;

  // General Methods
  void push(std::shared_ptr<Notification> notification) noexcept;
  int getUnreadNotificationsCount() const noexcept {
    return mUnreadNotifications;
  }
  int getCurrentProgressIndex() const noexcept { return mCurrentProgressIndex; }

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::NotificationData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::NotificationData& obj) noexcept override;

  // Operator Overloadings
  NotificationsModel& operator=(const NotificationsModel& rhs) = delete;

signals:
  void autoPopUpRequested();
  void unreadNotificationsCountChanged(int count);
  void currentProgressIndexChanged(int index);

private:
  int mapIndex(int i) const noexcept;
  void itemChanged(bool dismissed) noexcept;
  void removeItem(std::size_t i, int itemIndex) noexcept;
  void updateUnreadNotificationsCount() noexcept;
  void updateCurrentProgressIndex() noexcept;

private:
  Workspace& mWorkspace;
  std::vector<std::shared_ptr<Notification>> mItems;
  int mUnreadNotifications;
  int mCurrentProgressIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

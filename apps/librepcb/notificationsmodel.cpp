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

#include "apptoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NotificationsModel::NotificationsModel(Workspace& ws, QObject* parent) noexcept
  : QObject(parent), mWorkspace(ws) {
}

NotificationsModel::~NotificationsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NotificationsModel::add(ui::NotificationType type, const QString& title,
                             const QString& description,
                             const QString& buttonText,
                             bool supportsDontShowAgain) noexcept {
  mItems.push_back(ui::NotificationData{
      type,  // Type
      q2s(title),  // Title
      q2s(description),  // Description
      q2s(buttonText),  // Button text
      0,  // Progress
      supportsDontShowAgain,  // Supports "don't show again"
      true,  // Unread
      false,  // Displayed
      false,  // Button clicked
      false,  // Dismissed
      false,  // Don't show again clicked
  });
  row_added(mItems.size() - 1, 1);
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t NotificationsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::NotificationData> NotificationsModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void NotificationsModel::set_row_data(
    std::size_t i, const ui::NotificationData& obj) noexcept {
  if ((i >= 0) && (i < mItems.size())) {
    mItems[i] = obj;

    if (obj.dismissed) {
      mItems.erase(mItems.begin() + i);
      row_removed(i, 1);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

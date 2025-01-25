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

#ifndef LIBREPCB_NOTIFICATIONSMODEL_H
#define LIBREPCB_NOTIFICATIONSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {
namespace app {

class ProjectEditor;

/*******************************************************************************
 *  Class NotificationsModel
 ******************************************************************************/

/**
 * @brief The NotificationsModel class
 */
class NotificationsModel : public QObject,
                           public slint::Model<ui::NotificationData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  NotificationsModel() = delete;
  NotificationsModel(const NotificationsModel& other) = delete;
  explicit NotificationsModel(Workspace& ws,
                              QObject* parent = nullptr) noexcept;
  virtual ~NotificationsModel() noexcept;

  // General Methods
  void add(ui::NotificationType type, const QString& title,
           const QString& description, const QString& buttonText,
           bool supportsDontShowAgain) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::NotificationData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::NotificationData& obj) noexcept override;

  // Operator Overloadings
  NotificationsModel& operator=(const NotificationsModel& rhs) = delete;

private:
  Workspace& mWorkspace;
  std::vector<ui::NotificationData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif

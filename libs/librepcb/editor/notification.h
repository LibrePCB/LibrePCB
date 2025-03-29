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

#ifndef LIBREPCB_EDITOR_NOTIFICATION_H
#define LIBREPCB_EDITOR_NOTIFICATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class Notification
 ******************************************************************************/

/**
 * @brief The Notification class
 */
class Notification final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  Notification() = delete;
  Notification(const Notification& other) = delete;
  explicit Notification(ui::NotificationType type, const QString& title,
                        const QString& description, const QString& buttonText,
                        const QString& dismissKey, bool autoPopUp,
                        QObject* parent = nullptr) noexcept;
  ~Notification() noexcept;

  // General Methods
  const QString& getDismissKey() const noexcept { return mDismissKey; }
  bool getAutoPopUp() const noexcept { return mAutoPopUp; }
  const ui::NotificationData& getUiData() const noexcept { return mUiData; }
  void setUiData(const ui::NotificationData& data) noexcept;
  void resetState() noexcept;
  void setTitle(const QString& title) noexcept;
  void setDescription(const QString& description) noexcept;
  void setProgress(int progress) noexcept;
  void dismiss() noexcept;

  // Operator Overloadings
  Notification& operator=(const Notification& rhs) = delete;

signals:
  void changed(bool dismissed);
  void buttonClicked();

private:
  const QString mDismissKey;
  const bool mAutoPopUp;
  ui::NotificationData mUiData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

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
#include "notification.h"

#include "utils/slinthelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Notification::Notification(ui::NotificationType type, const QString& title,
                           const QString& description,
                           const QString& buttonText, const QString& dismissKey,
                           bool autoPopUp, QObject* parent) noexcept
  : QObject(parent),
    mDismissKey(dismissKey),
    mAutoPopUp(autoPopUp),
    mUiData{
        type,  // Type
        q2s(title),  // Title
        q2s(description),  // Description
        q2s(buttonText),  // Button text
        0,  // Progress
        !dismissKey.isEmpty(),  // Supports "don't show again"
        false,  // Unread
        false,  // Button clicked
        false,  // Dismissed
        false,  // Don't show again
    } {
}

Notification::~Notification() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Notification::setUiData(const ui::NotificationData& data) noexcept {
  mUiData = data;

  // Handle button click and immediately reset its state to keep the button
  // enabled.
  if (mUiData.button_clicked) {
    mUiData.button_clicked = false;
    emit buttonClicked();
  }

  // Never store the "do not show again" to make the notification pop-up again
  // immediately when any dismissed messages in the workspace are reset.
  mUiData.dont_show_again = false;
}

void Notification::resetState() noexcept {
  mUiData.unread = (mUiData.type != ui::NotificationType::Progress);
  mUiData.button_clicked = false;
  mUiData.dismissed = false;
  mUiData.dont_show_again = false;
}

void Notification::setTitle(const QString& title) noexcept {
  mUiData.title = q2s(title);
  emit changed(false);
}

void Notification::setDescription(const QString& description) noexcept {
  mUiData.description = q2s(description);
  emit changed(false);
}

void Notification::setProgress(int progress) noexcept {
  mUiData.progress = progress;
  emit changed(false);
}

void Notification::dismiss() noexcept {
  mUiData.dismissed = true;
  emit changed(true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

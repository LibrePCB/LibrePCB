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
#include "dismissablemessagecontext.h"

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

DismissableMessageContext::DismissableMessageContext(Workspace& workspace,
                                                     const QString& dismissKey,
                                                     bool active,
                                                     QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(&workspace),
    mDismissKey(dismissKey),
    mActive(active),
    mTemporarilyHidden(false),
    mVisible(false) {
  connect(&mWorkspace->getSettings().dismissedMessages,
          &WorkspaceSettingsItem::edited, this,
          &DismissableMessageContext::updateVisibility);
  updateVisibility();
}

DismissableMessageContext::DismissableMessageContext(bool active,
                                                     QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(),
    mDismissKey(),
    mActive(active),
    mTemporarilyHidden(false),
    mVisible(false) {
  updateVisibility();
}

DismissableMessageContext::~DismissableMessageContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::DismissableMessageData DismissableMessageContext::getUiData()
    const noexcept {
  return ui::DismissableMessageData{
      isVisible(),  // Visible
      !mDismissKey.isEmpty(),  // Supports don't show again
      ui::DismissableMessageAction::None,  // Action
  };
}

void DismissableMessageContext::setUiData(
    const ui::DismissableMessageData& data) noexcept {
  if (data.action != ui::DismissableMessageAction::None) {
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    const auto a = data.action;
    QMetaObject::invokeMethod(
        this, [this, a]() { trigger(a); }, Qt::QueuedConnection);
  }
}

void DismissableMessageContext::setActive(bool active) noexcept {
  mActive = active;
  if (!active) {
    mTemporarilyHidden = false;
  }
  updateVisibility();
}

void DismissableMessageContext::dismiss() noexcept {
  if (!mTemporarilyHidden) {
    mTemporarilyHidden = true;
    updateVisibility();
  }
}

void DismissableMessageContext::dontShowAgain() noexcept {
  try {
    if (mWorkspace && (!mDismissKey.isEmpty()) &&
        (!mWorkspace->getSettings().dismissedMessages.contains(mDismissKey))) {
      mWorkspace->getSettings().dismissedMessages.add(mDismissKey);
      mWorkspace->saveSettings();  // can throw
      updateVisibility();
    }
  } catch (const Exception& e) {
    qCritical().noquote() << "Failed to dismiss message:" << e.getMsg();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DismissableMessageContext::trigger(
    ui::DismissableMessageAction a) noexcept {
  switch (a) {
    case ui::DismissableMessageAction::Dismiss: {
      dismiss();
      break;
    }
    case ui::DismissableMessageAction::DontShowAgain: {
      dontShowAgain();
      break;
    }
    default: {
      qWarning() << "Unhandled action in DismissableMessageContext:"
                 << static_cast<int>(a);
      break;
    }
  }
}

void DismissableMessageContext::updateVisibility() noexcept {
  bool isVisible = mActive && (!mTemporarilyHidden);
  if (isVisible && mWorkspace && (!mDismissKey.isEmpty()) &&
      mWorkspace->getSettings().dismissedMessages.contains(mDismissKey)) {
    isVisible = false;
  }
  if (isVisible != mVisible) {
    mVisible = isVisible;
    emit visibilityChanged(mVisible);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

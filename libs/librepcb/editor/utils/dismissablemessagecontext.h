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

#ifndef LIBREPCB_EDITOR_DISMISSABLEMESSAGECONTEXT_H
#define LIBREPCB_EDITOR_DISMISSABLEMESSAGECONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

/*******************************************************************************
 *  Class DismissableMessageContext
 ******************************************************************************/

/**
 * @brief A handle to a optionally dismissable message in the UI
 *
 * Helper which holds and updates the state of some kind of message in the
 * UI which is dismissable, optionally even persistently with a "don't show
 * again" link. The dismissed state is just a `bool` hold in this class while
 * the "don't show again" feature is implemented with
 * ::librepcb::WorkspaceSettings::dismissedMessages and is therefore only
 * available if a ::librepcb::Workspace is set.
 */
class DismissableMessageContext final : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Constructor for a message without the "don't show again" feature
   *
   * Allows only temporary dismissing, but not permanently.
   *
   * @param active     Whether the message should currently be shown or not.
   * @param parent     Parent object.
   */
  explicit DismissableMessageContext(bool active = false,
                                     QObject* parent = nullptr) noexcept;

  /**
   * @brief Constructor for a message with the "don't show again" feature
   *
   * Allows both, temporary or permanently dismissing the message.
   *
   * @param workspace  The workspace to use for the persistent settings.
   * @param dismissKey The unique identifier for this message, see details at
   *                   ::librepcb::WorkspaceSettings::dismissedMessages.
   * @param active     Whether the message should currently be shown or not.
   * @param parent     Parent object.
   */
  explicit DismissableMessageContext(Workspace& workspace,
                                     const QString& dismissKey,
                                     bool active = false,
                                     QObject* parent = nullptr) noexcept;
  DismissableMessageContext(const DismissableMessageContext& other) = delete;
  ~DismissableMessageContext() noexcept;

  /**
   * @brief Get UI data
   *
   * @return UI data.
   */
  ui::DismissableMessageData getUiData() const noexcept;

  /**
   * @brief Set UI data
   *
   * @param data  UI data.
   */
  void setUiData(const ui::DismissableMessageData& data) noexcept;

  /**
   * @brief Set whether the message should be shown (if not dismissed) or not
   *
   * The widget will be visible only if `true` is passed and the message was
   * not dismissed.
   *
   * @param active    Whether the message should currently be shown or not.
   */
  void setActive(bool active) noexcept;

  /**
   * @brief Dismiss (hide) the message temporarily
   */
  void dismiss() noexcept;

  /**
   * @brief Dismiss (hide) the message persistently
   */
  void dontShowAgain() noexcept;

  /**
   * @brief Get whether the message shall currently be visible or not
   *
   * @retval true Message shall be visible.
   * @retval false Message shall be hidden.
   */
  bool isVisible() const noexcept { return mVisible; }

  // Operator Overloadings
  DismissableMessageContext& operator=(const DismissableMessageContext& rhs) =
      delete;

signals:
  void visibilityChanged(bool visible);

private:  // Methods
  void trigger(ui::DismissableMessageAction a) noexcept;
  void dismissedMessagesModified() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  const QPointer<Workspace> mWorkspace;
  const QString mDismissKey;
  bool mActive;
  bool mTemporarilyHidden;
  bool mVisible;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

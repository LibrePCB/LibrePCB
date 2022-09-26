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

#ifndef LIBREPCB_EDITOR_MESSAGEWIDGET_H
#define LIBREPCB_EDITOR_MESSAGEWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

/*******************************************************************************
 *  Class MessageWidget
 ******************************************************************************/

/**
 * @brief A widget containing a hidable, optionally dismissable message label
 *
 * This is a QLabel to show a custom message in the GUI, with a "hide" and
 * "don't show again" label on the right side to allow hiding it. The
 * "don't show again" feature is implemented with
 * ::librepcb::WorkspaceSettings::dismissedMessages and is therefore only
 * available if the Workspace is set.
 *
 * You have to call one of the #init() methods to make this widget working.
 * Do not call `QWidget::show()`, `QWidget::hide()` or `QWidget::setVisible()`
 * manually!
 */
class MessageWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit MessageWidget(QWidget* parent = nullptr) noexcept;
  MessageWidget(const MessageWidget& other) = delete;
  ~MessageWidget() noexcept;

  /**
   * @brief Initialize widget without the "don't show again" feature
   *
   * Allows only temporary dismissing, but not permanently.
   *
   * @param message   The message to show.
   * @param active    Whether the message should currently be shown or not.
   */
  void init(const QString& message, bool active) noexcept;

  /**
   * @brief Initialize widget with the "don't show again" feature
   *
   * Allows both, temporary or permanently dismissing the message.
   *
   * @param workspace   The workspace to use for the persistent settings.
   * @param dismissKey  The unique identifier for this message, see details at
   *                    ::librepcb::WorkspaceSettings::dismissedMessages.
   * @param message     The message to show.
   * @param active      Whether the message should currently be shown or not.
   */
  void init(Workspace& workspace, const QString& dismissKey,
            const QString& message, bool active) noexcept;

  /**
   * @brief Set whether the message should be shown (if not dismissed) or not
   *
   * The widget will be visible only if `true` is passed and the message was
   * not dismissed.
   *
   * @param active    Whether the message should currently be shown or not.
   */
  void setActive(bool active) noexcept;

  // Operator Overloadings
  MessageWidget& operator=(const MessageWidget& rhs) = delete;

signals:
  /**
   * @brief A link in the message label has been clicked
   *
   * @param link  The clicked link.
   */
  void linkActivated(const QString& link);

private:  // Methods
  void setWorkspace(Workspace* workspace) noexcept;
  void dismissedMessagesModified() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  QPointer<QHBoxLayout> mLayout;
  QScopedPointer<QLabel> mMessageLabel;
  QScopedPointer<QLabel> mDismissLabel;
  QScopedPointer<QLabel> mHideLabel;

  QPointer<Workspace> mWorkspace;
  QString mDismissKey;
  bool mActive;
  bool mTemporarilyHidden;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

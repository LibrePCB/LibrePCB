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

#ifndef LIBREPCB_EDITOR_EDITORCOMMAND_H
#define LIBREPCB_EDITOR_EDITORCOMMAND_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class EditorCommand
 ******************************************************************************/

/**
 * @brief Command for editors, e.g. to be added to a QMenu
 */
class EditorCommand final : public QObject {
  Q_OBJECT

public:
  // Types
  enum class Flag {
    OpensPopup = (1 << 0),
    AboutRole = (1 << 4),
    AboutQtRole = (1 << 5),
    PreferencesRole = (1 << 6),
    QuitRole = (1 << 7),
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  enum class ActionFlag {
    NoShortcuts = (1 << 0),  ///< Create an action without shortcuts.
    WidgetShortcut = (1 << 1),  ///< Restrict the shortcut to its widget.
    ApplicationShortcut = (1 << 2),  ///< Make the shortcut application global.
    QueuedConnection = (1 << 3),  ///< Create a queued signal/slot connection.
  };
  Q_DECLARE_FLAGS(ActionFlags, ActionFlag)

  // Constructors / Destructor
  EditorCommand() = delete;
  EditorCommand(const EditorCommand& other) = delete;
  EditorCommand(const QString& identifier, const char* text,
                const char* description, const QIcon& icon, Flags flags,
                const QList<QKeySequence>& defaultKeySequences,
                QObject* parent = nullptr) noexcept;
  ~EditorCommand() noexcept;

  // Getters
  const QString& getIdentifier() const noexcept { return mIdentifier; }
  const QString& getText() const noexcept { return mText; }
  QString getDisplayText() const noexcept { return unescapeAmpersand(mText); }
  QString getDisplayTextNoTr() const noexcept {
    return unescapeAmpersand(mTextNoTr);
  }
  const QString& getDescription() const noexcept { return mDescription; }
  const QIcon& getIcon() const noexcept { return mIcon; }
  const Flags& getFlags() const noexcept { return mFlags; }
  const QList<QKeySequence> getDefaultKeySequences() const noexcept {
    return mDefaultKeySequences;
  }
  const QList<QKeySequence>& getKeySequences() const noexcept {
    return mKeySequences;
  }

  // Setters
  void setKeySequences(const QList<QKeySequence>& sequences) noexcept;

  // General Methods
  QAction* createAction(QObject* parent,
                        ActionFlags flags = ActionFlags()) const noexcept;
  template <typename TContext, typename TSlot>
  QAction* createAction(QObject* parent, const TContext* context, TSlot slot,
                        ActionFlags flags = ActionFlags()) const noexcept {
    QAction* action = createAction(parent, flags);
    Qt::ConnectionType conType = flags.testFlag(ActionFlag::QueuedConnection)
        ? Qt::QueuedConnection
        : Qt::AutoConnection;
    QObject::connect(action, &QAction::triggered, context, slot, conType);
    return action;
  }

  // Operator Overloadings
  EditorCommand& operator=(const EditorCommand& rhs) = delete;

signals:
  void shortcutsChanged(const QList<QKeySequence>& sequences);

private:  // Methods
  QAction* setupAction(QAction* action, ActionFlags flags) const noexcept;
  bool eventFilter(QObject* obj, QEvent* event) noexcept override;
  static QString unescapeAmpersand(QString text) noexcept;

private:  // Data
  QString mIdentifier;
  const char* mTextNoTr;
  QString mText;
  QString mDescription;
  QIcon mIcon;
  Flags mFlags;
  QList<QKeySequence> mDefaultKeySequences;
  QList<QKeySequence> mKeySequences;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::EditorCommand::Flags)
Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::EditorCommand::ActionFlags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif

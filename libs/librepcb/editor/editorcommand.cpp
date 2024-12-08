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
#include "editorcommand.h"

#include "utils/editortoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static bool enableDarkIcons() noexcept {
  auto detect = []() {
    // This environment variable should not actively be promoted, it is only
    // here as a last resort if the auto-detection doesn't work for some users.
    const QString override = qgetenv("LIBREPCB_DARK_ICONS");
    if (override == "1") {
      return true;
    } else if (override == "0") {
      return false;
    } else {
      return EditorToolbox::isWindowBackgroundDark();
    }
  };
  static bool value = detect();
  return value;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditorCommand::EditorCommand(const QString& identifier, const char* text,
                             const char* description, const QString& iconFp,
                             Flags flags,
                             const QList<QKeySequence>& defaultKeySequences,
                             QObject* parent) noexcept
  : QObject(parent),
    mIdentifier(identifier),
    mTextNoTr(text),
    mText(text),
    mDescriptionNoTr(description),
    mDescription(description),
    mIcon(),
    mFlags(flags),
    mDefaultKeySequences(defaultKeySequences),
    mKeySequences(defaultKeySequences) {
  updateTranslations();

  const QStringList splitFp = iconFp.split('.');
  const QString darkIconFp = splitFp.first() % "_dark." % splitFp.last();
  if (enableDarkIcons() && QFileInfo::exists(darkIconFp)) {
    mIcon = QIcon(darkIconFp);
  } else {
    mIcon = QIcon(iconFp);
  }
}

EditorCommand::~EditorCommand() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EditorCommand::setKeySequences(
    const QList<QKeySequence>& sequences) noexcept {
  if (sequences != mKeySequences) {
    mKeySequences = sequences;
    emit shortcutsChanged(mKeySequences);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EditorCommand::updateTranslations() noexcept {
  // Note: Translations are done within the EditorCommandSet context.
  mText = QCoreApplication::translate("EditorCommandSet", mTextNoTr);
  mDescription =
      QCoreApplication::translate("EditorCommandSet", mDescriptionNoTr);
}

QAction* EditorCommand::createAction(QObject* parent,
                                     ActionFlags flags) const noexcept {
  return setupAction(new QAction(parent), flags);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QAction* EditorCommand::setupAction(QAction* action,
                                    ActionFlags flags) const noexcept {
  QString name = "action";
  foreach (const QString& fragment, mIdentifier.split('.').last().split('_')) {
    name.append(fragment.mid(0, 1).toUpper() % fragment.mid(1));
  }
  action->setObjectName(name);
  QString text = mText;
  if (mFlags.testFlag(Flag::OpensPopup)) {
    text.append("...");
  }
  action->setText(text);
  action->setIcon(mIcon);
  action->setStatusTip(mDescription);
  if (mFlags.testFlag(Flag::AboutRole)) {
    action->setMenuRole(QAction::AboutRole);
  } else if (mFlags.testFlag(Flag::AboutQtRole)) {
    action->setMenuRole(QAction::AboutQtRole);
  } else if (mFlags.testFlag(Flag::PreferencesRole)) {
    action->setMenuRole(QAction::PreferencesRole);
  } else if (mFlags.testFlag(Flag::QuitRole)) {
    action->setMenuRole(QAction::QuitRole);
  }
  if (!flags.testFlag(ActionFlag::NoShortcuts)) {
    action->setShortcuts(mKeySequences);
    if (flags.testFlag(ActionFlag::WidgetShortcut)) {
      action->setShortcutContext(Qt::WidgetShortcut);
    } else if (flags.testFlag(ActionFlag::ApplicationShortcut)) {
      action->setShortcutContext(Qt::ApplicationShortcut);
    }
    if ((!mKeySequences.isEmpty()) && (!mKeySequences.first().isEmpty())) {
      action->setToolTip(QString("%1 (%2)").arg(
          mText, mKeySequences.first().toString(QKeySequence::NativeText)));
    }
    action->installEventFilter(const_cast<EditorCommand*>(this));
    connect(this, &EditorCommand::shortcutsChanged, action,
            static_cast<void (QAction::*)(const QList<QKeySequence>&)>(
                &QAction::setShortcuts));
  }
  return action;
}

bool EditorCommand::eventFilter(QObject* obj, QEvent* event) noexcept {
  if (event->type() == QEvent::Shortcut) {
    QShortcutEvent* se = static_cast<QShortcutEvent*>(event);
    if (se->isAmbiguous()) {
      QAction* action = static_cast<QAction*>(obj);
      QList<QAction*> candidates;
      if (QWidget* widget = qobject_cast<QWidget*>(action->parent())) {
        if (QWidget* window = widget->window()) {
          foreach (QAction* a, window->findChildren<QAction*>()) {
            if (a->shortcuts().contains(se->key())) {
              QWidget* aParentWidget = qobject_cast<QWidget*>(a->parent());
              if ((a->shortcutContext() == Qt::ApplicationShortcut) ||
                  (a->shortcutContext() == Qt::WindowShortcut) ||
                  ((a->shortcutContext() == Qt::WidgetShortcut) &&
                   (aParentWidget) && (qApp->focusWidget()) &&
                   (aParentWidget->isAncestorOf(qApp->focusWidget())))) {
                candidates.append(a);
              }
            }
          }
        }
      }
      std::sort(candidates.begin(), candidates.end(),
                [](const QAction* a, const QAction* b) {
                  if (a->shortcutContext() != b->shortcutContext()) {
                    return a->shortcutContext() < b->shortcutContext();
                  } else if (a->priority() != b->priority()) {
                    return a->priority() > b->priority();
                  } else {
                    QWidget* pA = qobject_cast<QWidget*>(a->parent());
                    QWidget* pB = qobject_cast<QWidget*>(b->parent());
                    return ((pA) && (pB) && (pB->isAncestorOf(pA)));
                  }
                });
      if (!candidates.empty()) {
        qDebug() << "Ambiguous shortcut resolved:" << se->key().toString();
        QMetaObject::invokeMethod(candidates.first(), &QAction::trigger,
                                  Qt::QueuedConnection);
        return true;
      }
    }
  }

  return QObject::eventFilter(obj, event);
}

QString EditorCommand::unescapeAmpersand(QString text) noexcept {
  for (int i = text.length() - 1; i > 0; --i) {
    if ((text.at(i) == '&') && (text.at(i - 1) != '&')) {
      text.remove(i, 1);
    }
  }
  return text;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

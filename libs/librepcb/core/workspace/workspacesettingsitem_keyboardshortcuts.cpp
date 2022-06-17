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
#include "workspacesettingsitem_keyboardshortcuts.h"

#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsItem_KeyboardShortcuts::
    WorkspaceSettingsItem_KeyboardShortcuts(QObject* parent) noexcept
  : WorkspaceSettingsItem("keyboard_shortcuts", parent), mNodes() {
}

WorkspaceSettingsItem_KeyboardShortcuts::
    ~WorkspaceSettingsItem_KeyboardShortcuts() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceSettingsItem_KeyboardShortcuts::set(
    const QMap<QString, QList<QKeySequence>>& overrides) noexcept {
  bool modified = false;

  // Remove no longer overridden shortcuts.
  foreach (const QString& key, mOverrides.keys()) {
    if (!overrides.contains(key)) {
      mNodes.remove(key);
      mOverrides.remove(key);
      modified = true;
    }
  }

  // Update modified shortcuts.
  for (auto it = overrides.begin(); it != overrides.end(); it++) {
    if (!mOverrides.contains(it.key()) ||
        (mOverrides.value(it.key()) != it.value())) {
      SExpression node = SExpression::createList("shortcut");
      node.appendChild(SExpression::createToken(it.key()));
      foreach (const QKeySequence& sequence, it.value()) {
        node.appendChild(sequence.toString(QKeySequence::PortableText));
      }
      mNodes.insert(it.key(), node);
      mOverrides[it.key()] = it.value();
      modified = true;
    }
  }

  if (modified) {
    valueModified();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceSettingsItem_KeyboardShortcuts::restoreDefaultImpl() noexcept {
  if (!mNodes.isEmpty()) {
    mNodes.clear();
    mOverrides.clear();
    valueModified();
  }
}

void WorkspaceSettingsItem_KeyboardShortcuts::loadImpl(
    const SExpression& root, const Version& fileFormat) {
  Q_UNUSED(fileFormat);

  // Temporary objects to make this method atomic.
  QMap<QString, SExpression> nodes;
  QMap<QString, QList<QKeySequence>> overrides;
  foreach (const SExpression& child, root.getChildren("shortcut")) {
    QString identifier = child.getChild("@0").getValue();
    nodes.insert(identifier, child);
    QList<QKeySequence> sequences;
    foreach (const SExpression& node,
             child.getChildren(SExpression::Type::String)) {
      sequences.append(QKeySequence::fromString(node.getValue(),
                                                QKeySequence::PortableText));
    }
    overrides.insert(identifier, sequences);
  }

  mNodes = nodes;
  if (overrides != mOverrides) {
    mOverrides = overrides;
    valueModified();
  }
}

void WorkspaceSettingsItem_KeyboardShortcuts::serializeImpl(
    SExpression& root) const {
  foreach (const SExpression& node, mNodes) {
    root.ensureLineBreak();
    root.appendChild(node);
  }
  root.ensureLineBreakIfMultiLine();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

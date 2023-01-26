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
#include "workspacesettingsitem_themes.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsItem_Themes::WorkspaceSettingsItem_Themes(
    QObject* parent) noexcept
  : WorkspaceSettingsItem("themes", parent),
    mThemes(),
    mActiveUuid(Uuid::createRandom()),
    mActiveTheme() {
  restoreDefault();  // Add built-in default themes.
}

WorkspaceSettingsItem_Themes::~WorkspaceSettingsItem_Themes() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void WorkspaceSettingsItem_Themes::setAll(
    const QMap<Uuid, Theme>& themes) noexcept {
  if (themes != mThemes) {
    mThemes = themes;
    updateActiveTheme();
    valueModified();
  }
}

void WorkspaceSettingsItem_Themes::setActiveUuid(const Uuid& uuid) noexcept {
  if (uuid != mActiveUuid) {
    mActiveUuid = uuid;
    updateActiveTheme();
    valueModified();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceSettingsItem_Themes::restoreDefaultImpl() noexcept {
  mThemes.clear();
  mActiveUuid = Uuid::fromString("c1961d0f-51ec-4807-af5b-4d0ee26f1eaf");
  addTheme(Theme(mActiveUuid, "LibrePCB Default"));
  updateActiveTheme();
  valueModified();
}

void WorkspaceSettingsItem_Themes::loadImpl(const SExpression& root) {
  // Temporary objects to make this method atomic.
  QMap<Uuid, Theme> themes;
  foreach (const SExpression* child, root.getChildren("theme")) {
    Theme theme;
    theme.load(*child);  // can throw
    themes.insert(theme.getUuid(), theme);
  }
  const Uuid active = deserialize<Uuid>(root.getChild("active/@0"));

  if ((themes != mThemes) || (active != mActiveUuid)) {
    mThemes = themes;
    mActiveUuid = active;
    updateActiveTheme();
    valueModified();
  }
}

void WorkspaceSettingsItem_Themes::serializeImpl(SExpression& root) const {
  root.appendChild("active", mActiveUuid);
  foreach (const Theme& theme, mThemes) {
    root.ensureLineBreak();
    theme.serialize(root.appendList("theme"));
  }
  root.ensureLineBreak();
}

void WorkspaceSettingsItem_Themes::addTheme(const Theme& theme) noexcept {
  mThemes.insert(theme.getUuid(), theme);
}

void WorkspaceSettingsItem_Themes::updateActiveTheme() noexcept {
  if (mThemes.contains(mActiveUuid)) {
    mActiveTheme = mThemes[mActiveUuid];
  } else if (!mThemes.isEmpty()) {
    mActiveTheme = mThemes.first();
  } else {
    mActiveTheme = Theme();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

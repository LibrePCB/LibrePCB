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
#include "workspacesettings.h"

#include "../workspace.h"
#include "workspacesettingsdialog.h"

#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettings::WorkspaceSettings(const Workspace& workspace)
  : QObject(nullptr),
    mFilePath(workspace.getMetadataPath().getPathTo("settings.lp")) {
  qDebug("Load workspace settings...");

  // load settings if the settings file exists
  SExpression root;
  if (mFilePath.isExistingFile()) {
    root = SExpression::parse(FileUtils::readFile(mFilePath), mFilePath);
  } else {
    qInfo("Workspace settings file not found, default settings will be used.");
  }

  // load all settings
  loadSettingsItem(mUser, root);
  loadSettingsItem(mAppLocale, root);
  loadSettingsItem(mAppDefMeasUnits, root);
  loadSettingsItem(mProjectAutosaveInterval, root);
  loadSettingsItem(mAppearance, root);
  loadSettingsItem(mLibraryLocaleOrder, root);
  loadSettingsItem(mLibraryNormOrder, root);
  loadSettingsItem(mRepositories, root);
  loadSettingsItem(mDebugTools, root);

  // load the settings dialog
  mDialog.reset(new WorkspaceSettingsDialog(*this));

  qDebug("Workspace settings successfully loaded!");
}

WorkspaceSettings::~WorkspaceSettings() noexcept {
  mDialog.reset();  // the dialog must be deleted *before* any settings object!
  mItems.clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceSettings::restoreDefaults() noexcept {
  foreach (WSI_Base* item, mItems) { item->restoreDefault(); }
}

void WorkspaceSettings::applyAll() {
  foreach (WSI_Base* item, mItems) { item->apply(); }

  saveToFile();  // can throw
}

void WorkspaceSettings::revertAll() noexcept {
  foreach (WSI_Base* item, mItems) { item->revert(); }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void WorkspaceSettings::showSettingsDialog() noexcept {
  mDialog->exec();  // this is blocking
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename T>
void WorkspaceSettings::loadSettingsItem(QScopedPointer<T>& member,
                                         SExpression&       root) {
  member.reset(new T(root));  // can throw
  mItems.append(member.data());
}

void WorkspaceSettings::saveToFile() const {
  SExpression doc(serializeToDomElement("librepcb_workspace_settings"));
  FileUtils::writeFile(mFilePath, doc.toByteArray());  // can throw
}

void WorkspaceSettings::serialize(SExpression& root) const {
  foreach (WSI_Base* item, mItems) { item->serialize(root); }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

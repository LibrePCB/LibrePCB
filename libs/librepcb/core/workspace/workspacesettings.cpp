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

#include "../application.h"
#include "../fileio/fileutils.h"
#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettings::WorkspaceSettings(QObject* parent)
  : QObject(parent),
    mFileContent(),
    mUpgradeRequired(false),
    // Initialize settings items. Their constructor will register them as
    // child objects of this object, this way we will access them later.
    userName("user", "", this),
    applicationLocale("application_locale", "", this),
    defaultLengthUnit("default_length_unit", LengthUnit::millimeters(), this),
    projectAutosaveIntervalSeconds("project_autosave_interval", 600U, this),
    useOpenGl("use_opengl", false, this),
    libraryLocaleOrder("library_locale_order", "locale", QStringList(), this),
    libraryNormOrder("library_norm_order", "norm", QStringList(), this),
    repositoryUrls("repositories", "repository",
                   QList<QUrl>{QUrl("https://api.librepcb.org")}, this),
    externalWebBrowserCommands("external_web_browser", "command", QStringList(),
                               this),
    externalFileManagerCommands("external_file_manager", "command",
                                QStringList(), this),
    externalPdfReaderCommands("external_pdf_reader", "command", QStringList(),
                              this),
    keyboardShortcuts(this),
    dismissedMessages("dismissed_messages", "message", QSet<QString>(), this) {
}

WorkspaceSettings::~WorkspaceSettings() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void WorkspaceSettings::load(const SExpression& node,
                             const Version& fileFormat) {
  foreach (const SExpression* child,
           node.getChildren(SExpression::Type::List)) {
    mFileContent.insert(child->getName(), *child);
  }
  foreach (WorkspaceSettingsItem* item, getAllItems()) {
    try {
      if (mFileContent.contains(item->getKey())) {
        item->load(mFileContent[item->getKey()]);  // can throw
      }
    } catch (const Exception& e) {
      qCritical() << "Could not load workspace settings item:" << e.getMsg();
    }
  }
  if (fileFormat < qApp->getFileFormatVersion()) {
    mFileContent.clear();
    mUpgradeRequired = true;
  }
}

void WorkspaceSettings::restoreDefaults() noexcept {
  foreach (WorkspaceSettingsItem* item, getAllItems()) {
    item->restoreDefault();
  }
  mFileContent.clear();  // Remove even unknown settings!
}

SExpression WorkspaceSettings::serialize() {
  foreach (const WorkspaceSettingsItem* item, getAllItems()) {
    if (item->isEdited() || mUpgradeRequired) {
      if (item->isDefaultValue()) {
        mFileContent.remove(item->getKey());
      } else {
        SExpression node = SExpression::createList(item->getKey());
        item->serialize(node);  // can throw
        mFileContent.insert(item->getKey(), node);
      }
    }
  }
  SExpression root = SExpression::createList("librepcb_workspace_settings");
  foreach (const SExpression& child, mFileContent) {
    root.ensureLineBreak();
    root.appendChild(child);
  }
  root.ensureLineBreak();
  return root;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QList<WorkspaceSettingsItem*> WorkspaceSettings::getAllItems() const noexcept {
  return findChildren<WorkspaceSettingsItem*>();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

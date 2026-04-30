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
#include "eaglelibraryimportwizardcontext.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/eagleimport/eaglelibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using eagleimport::EagleLibraryImport;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryImportWizardContext::EagleLibraryImportWizardContext(
    const UiTheme& theme, Workspace& workspace, const FilePath& dstLibFp,
    QObject* parent) noexcept
  : QObject(parent),
    mTheme(theme),
    mWorkspace(workspace),
    mImport(new EagleLibraryImport(dstLibFp, &Uuid::createRandom, parent)),
    mLbrFilePath(),
    mAddNamePrefix(false),
    mAddToCategory(true),
    mComponentCategoryUuid(),
    mPackageCategoryUuid() {
  // Load settings.
  QSettings clientSettings;
  mLbrFilePath.setPath(
      clientSettings.value("library_editor/eagle_import_wizard/file")
          .toString());
  setAddNamePrefix(
      clientSettings
          .value("library_editor/eagle_import_wizard/add_name_prefix", false)
          .toBool());
  setAddToCategory(
      clientSettings
          .value("library_editor/eagle_import_wizard/add_category", true)
          .toBool());
  setComponentCategory(Uuid::tryFromString(
      clientSettings
          .value("library_editor/eagle_import_wizard/component_category")
          .toString()));
  setPackageCategory(Uuid::tryFromString(
      clientSettings
          .value("library_editor/eagle_import_wizard/package_category")
          .toString()));
}

EagleLibraryImportWizardContext::~EagleLibraryImportWizardContext() noexcept {
  // Save settings.
  QSettings clientSettings;
  clientSettings.setValue("library_editor/eagle_import_wizard/file",
                          mLbrFilePath.toStr());
  clientSettings.setValue("library_editor/eagle_import_wizard/add_name_prefix",
                          mAddNamePrefix);
  clientSettings.setValue("library_editor/eagle_import_wizard/add_category",
                          mAddToCategory);
  clientSettings.setValue(
      "library_editor/eagle_import_wizard/component_category",
      mComponentCategoryUuid ? mComponentCategoryUuid->toStr() : QString());
  clientSettings.setValue(
      "library_editor/eagle_import_wizard/package_category",
      mPackageCategoryUuid ? mPackageCategoryUuid->toStr() : QString());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EagleLibraryImportWizardContext::setLbrFilePath(
    const QString& filePath) noexcept {
  QStringList msg;
  try {
    FilePath fp(filePath);
    if (!fp.isValid()) {
      mImport->reset();
      msg.append(tr("No file selected."));
    } else if (fp != mImport->getLoadedFilePath()) {
      mLbrFilePath = fp;
      msg = mImport->open(fp);
      int count = mImport->getTotalElementsCount();
      msg.append(
          tr("Found %1 element(s) in the selected library.", nullptr, count)
              .arg(count));
    }
  } catch (const Exception& e) {
    msg = QStringList{e.getMsg()};
  }
  emit parseCompleted(msg.join("\n"));
}

void EagleLibraryImportWizardContext::setAddNamePrefix(bool add) noexcept {
  mAddNamePrefix = add;
  mImport->setNamePrefix(add ? NAME_PREFIX : QString());
}

void EagleLibraryImportWizardContext::setAddToCategory(bool add) noexcept {
  mAddToCategory = add;
  setComponentCategory(mComponentCategoryUuid);
  setPackageCategory(mPackageCategoryUuid);
}

void EagleLibraryImportWizardContext::setComponentCategory(
    const std::optional<Uuid>& uuid) noexcept {
  mComponentCategoryUuid = uuid;

  QSet<Uuid> categories;
  if (mAddToCategory) {
    categories.insert(EagleLibraryImport::getComponentCategory());
  }
  if (mComponentCategoryUuid) {
    categories.insert(*mComponentCategoryUuid);
  }
  mImport->setSymbolCategories(categories);
  mImport->setComponentCategories(categories);
  mImport->setDeviceCategories(categories);
}

void EagleLibraryImportWizardContext::setPackageCategory(
    const std::optional<Uuid>& uuid) noexcept {
  mPackageCategoryUuid = uuid;

  QSet<Uuid> categories;
  if (mAddToCategory) {
    categories.insert(EagleLibraryImport::getPackageCategory());
  }
  if (mPackageCategoryUuid) {
    categories.insert(*mPackageCategoryUuid);
  }
  mImport->setPackageCategories(categories);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

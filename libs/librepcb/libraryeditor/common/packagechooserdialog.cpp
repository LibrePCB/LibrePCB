/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "packagechooserdialog.h"

#include "ui_packagechooserdialog.h"

#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/library/pkg/footprintpreviewgraphicsitem.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageChooserDialog::PackageChooserDialog(
    const workspace::Workspace&     ws,
    const IF_GraphicsLayerProvider* layerProvider, QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mLayerProvider(layerProvider),
    mUi(new Ui::PackageChooserDialog) {
  mUi->setupUi(this);

  mGraphicsScene.reset(new GraphicsScene());
  mUi->graphicsView->setBackgroundBrush(Qt::black);
  mUi->graphicsView->setScene(mGraphicsScene.data());

  mCategoryTreeModel.reset(new workspace::PackageCategoryTreeModel(
      mWorkspace.getLibraryDb(), localeOrder()));
  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &PackageChooserDialog::treeCategories_currentItemChanged);
  connect(mUi->listPackages, &QListWidget::currentItemChanged, this,
          &PackageChooserDialog::listPackages_currentItemChanged);
  connect(mUi->listPackages, &QListWidget::itemDoubleClicked, this,
          &PackageChooserDialog::listPackages_itemDoubleClicked);

  setSelectedPackage(tl::nullopt);
}

PackageChooserDialog::~PackageChooserDialog() noexcept {
  setSelectedPackage(tl::nullopt);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageChooserDialog::treeCategories_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);
  setSelectedCategory(
      Uuid::tryFromString(current.data(Qt::UserRole).toString()));
}

void PackageChooserDialog::listPackages_currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  if (current) {
    setSelectedPackage(
        Uuid::tryFromString(current->data(Qt::UserRole).toString()));
  } else {
    setSelectedPackage(tl::nullopt);
  }
}

void PackageChooserDialog::listPackages_itemDoubleClicked(
    QListWidgetItem* item) noexcept {
  if (item) {
    setSelectedPackage(
        Uuid::tryFromString(item->data(Qt::UserRole).toString()));
    accept();
  }
}

void PackageChooserDialog::setSelectedCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  if (uuid && (uuid == mSelectedCategoryUuid)) return;

  setSelectedPackage(tl::nullopt);
  mUi->listPackages->clear();

  mSelectedCategoryUuid = uuid;

  try {
    QSet<Uuid> packages =
        mWorkspace.getLibraryDb().getPackagesByCategory(uuid);  // can throw
    foreach (const Uuid& pkgUuid, packages) {
      try {
        FilePath fp =
            mWorkspace.getLibraryDb().getLatestPackage(pkgUuid);  // can throw
        QString name;
        mWorkspace.getLibraryDb().getElementTranslations<Package>(
            fp, localeOrder(),
            &name);  // can throw
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, pkgUuid.toStr());
        mUi->listPackages->addItem(item);
      } catch (const Exception& e) {
        continue;  // should we do something here?
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not load packages"), e.getMsg());
  }
}

void PackageChooserDialog::setSelectedPackage(
    const tl::optional<Uuid>& uuid) noexcept {
  QString uuidStr = "00000000-0000-0000-0000-000000000000";
  QString name, desc;
  mSelectedPackageUuid = uuid;

  if (uuid) {
    try {
      uuidStr = uuid->toStr();
      mPackageFilePath =
          mWorkspace.getLibraryDb().getLatestPackage(*uuid);  // can throw
      mWorkspace.getLibraryDb().getElementTranslations<Package>(
          mPackageFilePath, localeOrder(), &name, &desc);  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Could not load package metadata"),
                            e.getMsg());
    }
  }

  mUi->lblPackageName->setText(name);
  mUi->lblPackageDescription->setText(desc);
  updatePreview();
}

void PackageChooserDialog::updatePreview() noexcept {
  mGraphicsItem.reset();
  mPackage.reset();

  if (mPackageFilePath.isValid() && mLayerProvider) {
    try {
      mPackage.reset(new Package(mPackageFilePath, true));  // can throw
      if (mPackage->getFootprints().count() > 0) {
        mGraphicsItem.reset(new FootprintPreviewGraphicsItem(
            *mLayerProvider, QStringList(), *mPackage->getFootprints().first(),
            mPackage.data()));
        mGraphicsScene->addItem(*mGraphicsItem);
        mUi->graphicsView->zoomAll();
      }
    } catch (const Exception& e) {
      // ignore errors...
    }
  }
}

void PackageChooserDialog::accept() noexcept {
  if (!mSelectedPackageUuid) {
    QMessageBox::information(this, tr("Invalid Selection"),
                             tr("Please select a package."));
    return;
  }
  QDialog::accept();
}

const QStringList& PackageChooserDialog::localeOrder() const noexcept {
  return mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

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
#include "packagechooserdialog.h"

#include "../../graphics/graphicsscene.h"
#include "../../widgets/waitingspinnerwidget.h"
#include "../../workspace/categorytreemodellegacy.h"
#include "footprintgraphicsitem.h"
#include "ui_packagechooserdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageChooserDialog::PackageChooserDialog(const Workspace& ws,
                                           const GraphicsLayerList* layers,
                                           QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mLayers(layers),
    mUi(new Ui::PackageChooserDialog),
    mCategorySelected(false),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);

  const Theme& theme = mWorkspace.getSettings().themes.getActive();
  mGraphicsScene->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->graphicsView->setScene(mGraphicsScene.data());

  mCategoryTreeModel.reset(new CategoryTreeModelLegacy(
      mWorkspace.getLibraryDb(), localeOrder(),
      CategoryTreeModelLegacy::Filter::PkgCatWithPackages));
  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &PackageChooserDialog::treeCategories_currentItemChanged);
  connect(mUi->listPackages, &QListWidget::currentItemChanged, this,
          &PackageChooserDialog::listPackages_currentItemChanged);
  connect(mUi->listPackages, &QListWidget::itemDoubleClicked, this,
          &PackageChooserDialog::listPackages_itemDoubleClicked);
  connect(mUi->edtSearch, &QLineEdit::textChanged, this,
          &PackageChooserDialog::searchEditTextChanged);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [&ws](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(ws.getLibraryDb().isScanInProgress());
  };
  addSpinner(mUi->treeCategories);
  addSpinner(mUi->listPackages);

  setSelectedPackage(std::nullopt);
}

PackageChooserDialog::~PackageChooserDialog() noexcept {
  setSelectedPackage(std::nullopt);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageChooserDialog::searchEditTextChanged(const QString& text) noexcept {
  try {
    QModelIndex catIndex = mUi->treeCategories->currentIndex();
    if (text.trimmed().isEmpty() && catIndex.isValid()) {
      setSelectedCategory(
          Uuid::tryFromString(catIndex.data(Qt::UserRole).toString()));
    } else {
      searchPackages(text.trimmed());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

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
    setSelectedPackage(std::nullopt);
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

void PackageChooserDialog::searchPackages(const QString& input) {
  setSelectedPackage(std::nullopt);
  mUi->listPackages->clear();
  mCategorySelected = false;

  // min. 2 chars to avoid freeze on entering first character due to huge result
  if (input.length() > 1) {
    QList<Uuid> packages = mWorkspace.getLibraryDb().find<Package>(input);
    foreach (const Uuid& uuid, packages) {
      FilePath fp =
          mWorkspace.getLibraryDb().getLatest<Package>(uuid);  // can throw
      QString name;
      mWorkspace.getLibraryDb().getTranslations<Package>(fp, localeOrder(),
                                                         &name);  // can throw
      bool deprecated = false;
      mWorkspace.getLibraryDb().getMetadata<Package>(fp, nullptr, nullptr,
                                                     &deprecated);  // can throw
      QListWidgetItem* item = new QListWidgetItem(name);
      item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
      item->setData(Qt::UserRole, uuid.toStr());
      mUi->listPackages->addItem(item);
    }
  }
}

void PackageChooserDialog::setSelectedCategory(
    const std::optional<Uuid>& uuid) noexcept {
  if ((mCategorySelected) && (uuid == mSelectedCategoryUuid)) return;

  setSelectedPackage(std::nullopt);
  mUi->listPackages->clear();
  mSelectedCategoryUuid = uuid;
  mCategorySelected = true;

  try {
    QSet<Uuid> packages =
        mWorkspace.getLibraryDb().getByCategory<Package>(uuid);  // can throw
    foreach (const Uuid& pkgUuid, packages) {
      try {
        FilePath fp =
            mWorkspace.getLibraryDb().getLatest<Package>(pkgUuid);  // can throw
        QString name;
        mWorkspace.getLibraryDb().getTranslations<Package>(fp, localeOrder(),
                                                           &name);  // can throw
        bool deprecated = false;
        mWorkspace.getLibraryDb().getMetadata<Package>(
            fp, nullptr, nullptr, &deprecated);  // can throw
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
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
    const std::optional<Uuid>& uuid) noexcept {
  FilePath fp;
  QString name = tr("No package selected");
  QString desc;
  mSelectedPackageUuid = uuid;

  if (uuid) {
    try {
      fp = mWorkspace.getLibraryDb().getLatest<Package>(*uuid);  // can throw
      mWorkspace.getLibraryDb().getTranslations<Package>(
          fp, localeOrder(), &name, &desc);  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Could not load package metadata"),
                            e.getMsg());
    }
  }

  mUi->lblPackageName->setText(name);
  mUi->lblPackageDescription->setText(desc);
  updatePreview(fp);
}

void PackageChooserDialog::updatePreview(const FilePath& fp) noexcept {
  mGraphicsItem.reset();
  mPackage.reset();

  if (fp.isValid() && mLayers) {
    try {
      mPackage = Package::open(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(fp))));  // can throw
      if (mPackage->getFootprints().count() > 0) {
        mGraphicsItem.reset(new FootprintGraphicsItem(
            mPackage->getFootprints().first(), *mLayers,
            Application::getDefaultStrokeFont(), &mPackage->getPads(), nullptr,
            localeOrder()));
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
  return mWorkspace.getSettings().libraryLocaleOrder.get();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

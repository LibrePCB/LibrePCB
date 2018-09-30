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
#include "libraryoverviewwidget.h"

#include "librarylisteditorwidget.h"
#include "ui_libraryoverviewwidget.h"

#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/library/elements.h>
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

LibraryOverviewWidget::LibraryOverviewWidget(const Context&          context,
                                             QSharedPointer<Library> lib,
                                             QWidget* parent) noexcept
  : EditorWidgetBase(context, lib->getFilePath(), parent),
    mLibrary(lib),
    mUi(new Ui::LibraryOverviewWidget) {
  mUi->setupUi(this);
  setWindowTitle(*mLibrary->getNames().value(getLibLocaleOrder()));
  updateIcon();

  // insert dependencies editor widget
  mDependenciesEditorWidget.reset(
      new LibraryListEditorWidget(mContext.workspace, this));
  int                   row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblDependencies, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mDependenciesEditorWidget.data());

  mUi->lblUuid->setText(mLibrary->getUuid().toStr());
  mUi->edtName->setText(*mLibrary->getNames().value(getLibLocaleOrder()));
  mUi->edtDescription->setPlainText(
      mLibrary->getDescriptions().value(getLibLocaleOrder()));
  mUi->edtKeywords->setText(mLibrary->getKeywords().value(getLibLocaleOrder()));
  mUi->edtAuthor->setText(mLibrary->getAuthor());
  mUi->edtVersion->setText(mLibrary->getVersion().toStr());
  mUi->edtUrl->setText(mLibrary->getUrl().toString());
  mUi->cbxDeprecated->setChecked(mLibrary->isDeprecated());
  mDependenciesEditorWidget->setUuids(mLibrary->getDependencies());

  connect(mUi->btnIcon, &QPushButton::clicked, this,
          &LibraryOverviewWidget::btnIconClicked);
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &QWidget::setWindowTitle);
  connect(mUi->edtName, &QLineEdit::textEdited, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->edtKeywords, &QLineEdit::textEdited, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->edtAuthor, &QLineEdit::textEdited, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->edtVersion, &QLineEdit::textEdited, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->edtUrl, &QLineEdit::textEdited, this,
          &LibraryOverviewWidget::setDirty);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &LibraryOverviewWidget::setDirty);
  connect(mDependenciesEditorWidget.data(),
          &LibraryListEditorWidget::libraryAdded, this,
          &LibraryOverviewWidget::setDirty);
  connect(mDependenciesEditorWidget.data(),
          &LibraryListEditorWidget::libraryRemoved, this,
          &LibraryOverviewWidget::setDirty);

  updateElementLists();
  connect(&mContext.workspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanSucceeded, this,
          &LibraryOverviewWidget::updateElementLists);

  connect(mUi->lstCmpCat, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstCmpCatDoubleClicked);
  connect(mUi->lstPkgCat, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstPkgCatDoubleClicked);
  connect(mUi->lstSym, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstSymDoubleClicked);
  connect(mUi->lstPkg, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstPkgDoubleClicked);
  connect(mUi->lstCmp, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstCmpDoubleClicked);
  connect(mUi->lstDev, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDevDoubleClicked);
}

LibraryOverviewWidget::~LibraryOverviewWidget() noexcept {
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool LibraryOverviewWidget::save() noexcept {
  try {
    ElementName name(mUi->edtName->text().trimmed());  // can throw
    Version     version =
        Version::fromString(mUi->edtVersion->text().trimmed());  // can throw

    mLibrary->setName("", name);
    mLibrary->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    mLibrary->setKeywords("", mUi->edtKeywords->text().trimmed());
    mLibrary->setAuthor(mUi->edtAuthor->text().trimmed());
    mLibrary->setVersion(version);
    mLibrary->setDeprecated(mUi->cbxDeprecated->isChecked());
    mLibrary->setDependencies(mDependenciesEditorWidget->getUuids());
    mLibrary->save();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryOverviewWidget::updateIcon() noexcept {
  mUi->btnIcon->setToolTip(
      tr("Click here to choose an icon (PNG, 256x256px)."));
  setWindowIcon(mLibrary->getIcon());
  mUi->btnIcon->setIcon(mLibrary->getIcon());
  if (mLibrary->getIcon().isNull()) {
    mUi->btnIcon->setText(mUi->btnIcon->toolTip());
  } else {
    mUi->btnIcon->setText(QString());
  }
}

void LibraryOverviewWidget::updateElementLists() noexcept {
  updateElementList<ComponentCategory>(*mUi->lstCmpCat,
                                       QIcon(":/img/places/folder.png"));
  updateElementList<PackageCategory>(*mUi->lstPkgCat,
                                     QIcon(":/img/places/folder_green.png"));
  updateElementList<Symbol>(*mUi->lstSym, QIcon(":/img/library/symbol.png"));
  updateElementList<Package>(*mUi->lstPkg, QIcon(":/img/library/package.png"));
  updateElementList<Component>(*mUi->lstCmp,
                               QIcon(":/img/library/component.png"));
  updateElementList<Device>(*mUi->lstDev, QIcon(":/img/library/device.png"));
}

template <typename ElementType>
void LibraryOverviewWidget::updateElementList(QListWidget& listWidget,
                                              const QIcon& icon) noexcept {
  QHash<FilePath, QString> elementNames;

  try {
    // get all library element names
    QList<FilePath> elements =
        mContext.workspace.getLibraryDb().getLibraryElements<ElementType>(
            mLibrary->getFilePath());  // can throw
    foreach (const FilePath& filepath, elements) {
      QString name;
      mContext.workspace.getLibraryDb().getElementTranslations<ElementType>(
          filepath, getLibLocaleOrder(), &name);  // can throw
      elementNames.insert(filepath, name);
    }
  } catch (const Exception& e) {
    listWidget.clear();
    QListWidgetItem* item = new QListWidgetItem(&listWidget);
    item->setText(e.getMsg());
    item->setToolTip(e.getMsg());
    item->setIcon(QIcon(":/img/status/dialog_error.png"));
    item->setBackground(Qt::red);
    item->setForeground(Qt::white);
    return;
  }

  // update/remove existing list widget items
  for (int i = listWidget.count() - 1; i >= 0; --i) {
    QListWidgetItem* item = listWidget.item(i);
    Q_ASSERT(item);
    FilePath filePath(item->data(Qt::UserRole).toString());
    if (elementNames.contains(filePath)) {
      item->setText(elementNames.take(filePath));
    } else {
      delete item;
    }
  }

  // add new list widget items
  foreach (const FilePath& fp, elementNames.keys()) {
    QString          name = elementNames.value(fp);
    QListWidgetItem* item = new QListWidgetItem(&listWidget);
    item->setText(name);
    item->setToolTip(name);
    item->setData(Qt::UserRole, fp.toStr());
    item->setIcon(icon);
  }
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

void LibraryOverviewWidget::btnIconClicked() noexcept {
  QString fp = FileDialog::getOpenFileName(
      this, tr("Choose library icon"), mLibrary->getIconFilePath().toNative(),
      tr("Portable Network Graphics (*.png)"));
  if (!fp.isEmpty()) {
    mLibrary->setIconFilePath(FilePath(fp));
    updateIcon();
  }
}

void LibraryOverviewWidget::lstCmpCatDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstCmpCat->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editComponentCategoryTriggered(fp);
  }
}

void LibraryOverviewWidget::lstPkgCatDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstPkgCat->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editPackageCategoryTriggered(fp);
  }
}

void LibraryOverviewWidget::lstSymDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstSym->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editSymbolTriggered(fp);
  }
}

void LibraryOverviewWidget::lstPkgDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstPkg->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editPackageTriggered(fp);
  }
}

void LibraryOverviewWidget::lstCmpDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstCmp->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editComponentTriggered(fp);
  }
}

void LibraryOverviewWidget::lstDevDoubleClicked(
    const QModelIndex& index) noexcept {
  QListWidgetItem* item = mUi->lstDev->item(index.row());
  FilePath         fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    emit editDeviceTriggered(fp);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

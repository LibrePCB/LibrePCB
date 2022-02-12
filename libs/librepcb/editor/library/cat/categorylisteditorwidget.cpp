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
#include "categorylisteditorwidget.h"

#include "categorychooserdialog.h"
#include "ui_categorylisteditorwidget.h"

#include <librepcb/core/workspace/workspace.h>
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

CategoryListEditorWidget::CategoryListEditorWidget(const Workspace& ws,
                                                   Categories categories,
                                                   QWidget* parent) noexcept
  : QWidget(parent),
    mWorkspace(ws),
    mCategories(categories),
    mUi(new Ui::CategoryListEditorWidget),
    mRequiresMinimumOneEntry(false) {
  mUi->setupUi(this);
  connect(mUi->btnAdd, &QPushButton::clicked, this,
          &CategoryListEditorWidget::btnAddClicked);
  connect(mUi->btnRemove, &QPushButton::clicked, this,
          &CategoryListEditorWidget::btnRemoveClicked);
}

CategoryListEditorWidget::~CategoryListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CategoryListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mUi->btnAdd->setHidden(readOnly);
  mUi->btnRemove->setHidden(readOnly);
}

void CategoryListEditorWidget::setRequiresMinimumOneEntry(bool v) noexcept {
  mRequiresMinimumOneEntry = v;
  updateColor();
}

void CategoryListEditorWidget::setUuids(const QSet<Uuid>& uuids) noexcept {
  mUuids = uuids;
  mUi->listWidget->clear();
  foreach (const Uuid& category, mUuids) { addItem(category); }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CategoryListEditorWidget::btnAddClicked() noexcept {
  tl::optional<Uuid> uuid = chooseCategoryWithDialog();
  if (uuid && !mUuids.contains(*uuid)) {
    mUuids.insert(*uuid);
    addItem(*uuid);
    emit categoryAdded(*uuid);
    emit edited();
  }
}

void CategoryListEditorWidget::btnRemoveClicked() noexcept {
  QListWidgetItem* item = mUi->listWidget->currentItem();
  tl::optional<Uuid> uuid = item
      ? Uuid::tryFromString(item->data(Qt::UserRole).toString())
      : tl::nullopt;
  if (item && uuid) {
    mUuids.remove(*uuid);
    delete item;
    updateColor();
    // Emit signals *after* removing the item to avoid critical issues if a
    // signal handler modifies the UUID list before removing was finished.
    emit categoryRemoved(*uuid);
    emit edited();
  }
}

void CategoryListEditorWidget::addItem(
    const tl::optional<Uuid>& category) noexcept {
  try {
    addItem(category, buildTree(category));  // can throw
  } catch (const Exception& e) {
    addItem(category, "ERROR: " % e.getMsg());
  }
}

void CategoryListEditorWidget::addItem(const tl::optional<Uuid>& category,
                                       const QStringList& lines) noexcept {
  QString text;
  for (int i = 0; i < lines.count(); ++i) {
    QString line = lines.value(i);
    if (i == 0) {
      text.append(line);
    } else {
      text.append(QString("\n%1⤷ %2").arg(QString(" ").repeated(i * 2), line));
    }
  }
  addItem(category, text);
}

void CategoryListEditorWidget::addItem(const tl::optional<Uuid>& category,
                                       const QString& text) noexcept {
  QString uuidStr = category ? category->toStr() : QString();
  QListWidgetItem* item = new QListWidgetItem(text, mUi->listWidget);
  item->setData(Qt::UserRole, uuidStr);
  item->setToolTip(uuidStr);
  updateColor();
}

void CategoryListEditorWidget::updateColor() noexcept {
  if (mRequiresMinimumOneEntry && (mUi->listWidget->count() == 0)) {
    mUi->listWidget->setStyleSheet("background-color: #FF5555;");
  } else {
    mUi->listWidget->setStyleSheet("");
  }
}

QStringList CategoryListEditorWidget::buildTree(
    const tl::optional<Uuid>& category) const {
  if (mCategories == Categories::Package) {
    CategoryTreeBuilder<PackageCategory> builder(
        mWorkspace.getLibraryDb(),
        mWorkspace.getSettings().libraryLocaleOrder.get(), false);
    return builder.buildTree(category);
  } else {
    CategoryTreeBuilder<ComponentCategory> builder(
        mWorkspace.getLibraryDb(),
        mWorkspace.getSettings().libraryLocaleOrder.get(), false);
    return builder.buildTree(category);
  }
}

tl::optional<Uuid>
    CategoryListEditorWidget::chooseCategoryWithDialog() noexcept {
  CategoryChooserDialog dialog(mWorkspace,
                               (mCategories == Categories::Package)
                                   ? CategoryChooserDialog::Filter::PkgCat
                                   : CategoryChooserDialog::Filter::CmpCat,
                               this);
  if (dialog.exec() == QDialog::Accepted) {
    return dialog.getSelectedCategoryUuid();
  } else {
    return tl::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

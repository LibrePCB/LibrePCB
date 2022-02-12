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

CategoryListEditorWidgetBase::CategoryListEditorWidgetBase(
    QWidget* parent) noexcept
  : QWidget(parent),
    mUi(new Ui::CategoryListEditorWidget),
    mRequiresMinimumOneEntry(false) {
  mUi->setupUi(this);
  connect(mUi->btnAdd, &QPushButton::clicked, this,
          &CategoryListEditorWidgetBase::btnAddClicked);
  connect(mUi->btnRemove, &QPushButton::clicked, this,
          &CategoryListEditorWidgetBase::btnRemoveClicked);
}

CategoryListEditorWidgetBase::~CategoryListEditorWidgetBase() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CategoryListEditorWidgetBase::setReadOnly(bool readOnly) noexcept {
  mUi->btnAdd->setHidden(readOnly);
  mUi->btnRemove->setHidden(readOnly);
}

void CategoryListEditorWidgetBase::setRequiresMinimumOneEntry(bool v) noexcept {
  mRequiresMinimumOneEntry = v;
  updateColor();
}

void CategoryListEditorWidgetBase::setUuids(const QSet<Uuid>& uuids) noexcept {
  mUuids = uuids;
  mUi->listWidget->clear();
  foreach (const Uuid& category, mUuids) { addItem(category); }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CategoryListEditorWidgetBase::btnAddClicked() noexcept {
  tl::optional<Uuid> uuid = chooseCategoryWithDialog();
  if (uuid && !mUuids.contains(*uuid)) {
    mUuids.insert(*uuid);
    addItem(*uuid);
    emit categoryAdded(*uuid);
    emit edited();
  }
}

void CategoryListEditorWidgetBase::btnRemoveClicked() noexcept {
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

void CategoryListEditorWidgetBase::addItem(
    const tl::optional<Uuid>& category) noexcept {
  try {
    addItem(category, buildTree(category));  // can throw
  } catch (const Exception& e) {
    addItem(category, "ERROR: " % e.getMsg());
  }
}

void CategoryListEditorWidgetBase::addItem(const tl::optional<Uuid>& category,
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

void CategoryListEditorWidgetBase::addItem(const tl::optional<Uuid>& category,
                                           const QString& text) noexcept {
  QString uuidStr = category ? category->toStr() : QString();
  QListWidgetItem* item = new QListWidgetItem(text, mUi->listWidget);
  item->setData(Qt::UserRole, uuidStr);
  item->setToolTip(uuidStr);
  updateColor();
}

void CategoryListEditorWidgetBase::updateColor() noexcept {
  if (mRequiresMinimumOneEntry && (mUi->listWidget->count() == 0)) {
    mUi->listWidget->setStyleSheet("background-color: #FF5555;");
  } else {
    mUi->listWidget->setStyleSheet("");
  }
}

/*******************************************************************************
 *  Class CategoryListEditorWidget
 ******************************************************************************/

template <typename ElementType>
CategoryListEditorWidget<ElementType>::CategoryListEditorWidget(
    const Workspace& ws, QWidget* parent) noexcept
  : CategoryListEditorWidgetBase(parent),
    mWorkspace(ws),
    mBuilder(ws.getLibraryDb(), ws.getSettings().libraryLocaleOrder.get(),
             false) {
}

template <typename ElementType>
CategoryListEditorWidget<ElementType>::~CategoryListEditorWidget() noexcept {
}

template <typename ElementType>
tl::optional<Uuid>
    CategoryListEditorWidget<ElementType>::chooseCategoryWithDialog() noexcept {
  CategoryChooserDialog<ElementType> dialog(mWorkspace, this);
  if (dialog.exec() == QDialog::Accepted) {
    return dialog.getSelectedCategoryUuid();
  } else {
    return tl::nullopt;
  }
}

template <typename ElementType>
QStringList CategoryListEditorWidget<ElementType>::buildTree(
    const tl::optional<Uuid>& category) const {
  return mBuilder.buildTree(category);
}

template class CategoryListEditorWidget<ComponentCategory>;
template class CategoryListEditorWidget<PackageCategory>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

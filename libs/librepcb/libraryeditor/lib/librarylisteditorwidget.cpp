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
#include "librarylisteditorwidget.h"

#include "ui_librarylisteditorwidget.h"

#include <librepcb/library/library.h>
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

LibraryListEditorWidget::LibraryListEditorWidget(const workspace::Workspace& ws,
                                                 QWidget* parent) noexcept
  : QWidget(parent), mWorkspace(ws), mUi(new Ui::LibraryListEditorWidget) {
  mUi->setupUi(this);
  connect(mUi->btnAdd, &QPushButton::clicked, this,
          &LibraryListEditorWidget::btnAddClicked);
  connect(mUi->btnRemove, &QPushButton::clicked, this,
          &LibraryListEditorWidget::btnRemoveClicked);

  const QStringList& localeOrder =
      mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
  QList<QSharedPointer<library::Library>> libs;
  libs.append(mWorkspace.getLocalLibraries().values());
  libs.append(mWorkspace.getRemoteLibraries().values());
  mUi->comboBox->addItem(tr("Choose library..."));
  foreach (const QSharedPointer<library::Library>& lib, libs) {
    mUi->comboBox->addItem(lib->getIcon(), *lib->getNames().value(localeOrder),
                           lib->getUuid().toStr());
  }
}

LibraryListEditorWidget::~LibraryListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LibraryListEditorWidget::setUuids(const QSet<Uuid>& uuids) noexcept {
  mUuids = uuids;
  mUi->listWidget->clear();
  foreach (const Uuid& category, mUuids) { addItem(category); }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryListEditorWidget::btnAddClicked() noexcept {
  tl::optional<Uuid> uuid =
      Uuid::tryFromString(mUi->comboBox->currentText().trimmed());
  if (!uuid) {
    uuid = Uuid::tryFromString(
        mUi->comboBox->currentData(Qt::UserRole).toString());
  }
  if (!uuid) {
    QMessageBox::warning(this, tr("Error"), tr("Invalid UUID"));
    return;
  }
  if (uuid && !mUuids.contains(*uuid)) {
    mUuids.insert(*uuid);
    addItem(*uuid);
    emit libraryAdded(*uuid);
  }
}

void LibraryListEditorWidget::btnRemoveClicked() noexcept {
  QListWidgetItem*   item = mUi->listWidget->currentItem();
  tl::optional<Uuid> uuid =
      item ? Uuid::tryFromString(item->data(Qt::UserRole).toString())
           : tl::nullopt;
  if (item && uuid) {
    mUuids.remove(*uuid);
    emit libraryRemoved(*uuid);
    delete item;
  }
}

void LibraryListEditorWidget::addItem(const Uuid& library) noexcept {
  QString name = library.toStr();

  const QStringList& localeOrder =
      mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
  QList<QSharedPointer<library::Library>> libs;
  libs.append(mWorkspace.getLocalLibraries().values());
  libs.append(mWorkspace.getRemoteLibraries().values());
  foreach (const QSharedPointer<library::Library>& lib, libs) {
    if (lib->getUuid() == library) {
      name = *lib->getNames().value(localeOrder);
      break;
    }
  }

  QListWidgetItem* item = new QListWidgetItem(name, mUi->listWidget);
  item->setData(Qt::UserRole, library.toStr());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

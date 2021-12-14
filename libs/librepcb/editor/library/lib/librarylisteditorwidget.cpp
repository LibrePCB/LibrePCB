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
#include "librarylisteditorwidget.h"

#include "../../modelview/comboboxdelegate.h"
#include "../../modelview/sortfilterproxymodel.h"
#include "ui_librarylisteditorwidget.h"

#include <librepcb/core/library/library.h>
#include <librepcb/core/utils/toolbox.h>
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

LibraryListEditorWidget::LibraryListEditorWidget(const Workspace& ws,
                                                 QWidget* parent) noexcept
  : QWidget(parent),
    mModel(new Model(this)),
    mProxyModel(new SortFilterProxyModel(this)),
    mUi(new Ui::LibraryListEditorWidget) {
  mUi->setupUi(this);
  mModel->setPlaceholderText(tr("Click here a add a new dependency"));
  mProxyModel->setKeepLastRowAtBottom(true);
  mProxyModel->setSourceModel(mModel.data());
  mUi->tableView->setModel(mProxyModel.data());
  mUi->tableView->setItemDelegateForColumn(Model::COLUMN_TEXT,
                                           new ComboBoxDelegate(false, this));
  mUi->tableView->horizontalHeader()->setSectionResizeMode(
      Model::COLUMN_TEXT, QHeaderView::Stretch);
  mUi->tableView->horizontalHeader()->setSectionResizeMode(
      Model::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
  mUi->tableView->sortByColumn(Model::COLUMN_TEXT, Qt::AscendingOrder);
  connect(mUi->tableView, &EditableTableWidget::btnAddClicked, mModel.data(),
          &Model::addItem);
  connect(mUi->tableView, &EditableTableWidget::btnRemoveClicked, mModel.data(),
          &Model::removeItem);
  connect(mModel.data(), &Model::rowsInserted, this,
          &LibraryListEditorWidget::edited);
  connect(mModel.data(), &Model::rowsRemoved, this,
          &LibraryListEditorWidget::edited);

  try {
    QList<Uuid> uuids;
    QMultiMap<Version, FilePath> libs =
        ws.getLibraryDb().getLibraries();  // can throw
    foreach (const FilePath& fp, libs) {
      Uuid uuid = Uuid::createRandom();
      ws.getLibraryDb().getElementMetadata<Library>(fp, &uuid);  // can throw
      QString name;
      ws.getLibraryDb().getElementTranslations<Library>(
          fp, ws.getSettings().libraryLocaleOrder.get(),
          &name);  // can throw
      mModel->setDisplayText(uuid, name);
      QPixmap icon;
      ws.getLibraryDb().getLibraryMetadata(fp, &icon);  // can throw
      uuids.append(uuid);
      mModel->setIcon(uuid, icon);
    }
    mModel->setChoices(uuids);
  } catch (const Exception& e) {
    qCritical() << "Could not load library list.";
  }
}

LibraryListEditorWidget::~LibraryListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<Uuid> LibraryListEditorWidget::getUuids() const noexcept {
  return Toolbox::toSet(mModel->getValues());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LibraryListEditorWidget::setReadOnly(bool readOnly) noexcept {
  mUi->tableView->setReadOnly(readOnly);
}

void LibraryListEditorWidget::setUuids(const QSet<Uuid>& uuids) noexcept {
  mModel->setValues(uuids.values());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

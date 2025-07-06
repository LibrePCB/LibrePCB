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
#include "categorychooserdialog.h"

#include "../../widgets/waitingspinnerwidget.h"
#include "ui_categorychooserdialog.h"

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

CategoryChooserDialog::CategoryChooserDialog(const Workspace& ws,
                                             Filters filters,
                                             QWidget* parent) noexcept
  : QDialog(parent), mUi(new Ui::CategoryChooserDialog) {
  mUi->setupUi(this);
  connect(mUi->treeView, &QTreeView::doubleClicked, this,
          &CategoryChooserDialog::accept);

  mModel.reset(new CategoryTreeModelLegacy(
      ws.getLibraryDb(), ws.getSettings().libraryLocaleOrder.get(), filters));
  mUi->treeView->setModel(mModel.data());
  mUi->treeView->setRootIndex(QModelIndex());

  // Add waiting spinner during workspace library scan.
  WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(mUi->treeView);
  connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanStarted, spinner,
          &WaitingSpinnerWidget::show);
  connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanFinished, spinner,
          &WaitingSpinnerWidget::hide);
  spinner->setVisible(ws.getLibraryDb().isScanInProgress());
}

CategoryChooserDialog::~CategoryChooserDialog() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::optional<Uuid> CategoryChooserDialog::getSelectedCategoryUuid()
    const noexcept {
  QModelIndex index = mUi->treeView->currentIndex();
  return Uuid::tryFromString(index.data(Qt::UserRole).toString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

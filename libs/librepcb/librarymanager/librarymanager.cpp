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
#include "librarymanager.h"

#include "addlibrarywidget.h"
#include "libraryinfowidget.h"
#include "librarylistwidgetitem.h"
#include "ui_librarymanager.h"

#include <librepcb/library/library.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace manager {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryManager::LibraryManager(workspace::Workspace& ws,
                               QWidget*              parent) noexcept
  : QMainWindow(parent),
    mWorkspace(ws),
    mUi(new Ui::LibraryManager),
    mCurrentWidget(nullptr),
    mSelectedLibrary() {
  mUi->setupUi(this);
  connect(mUi->btnClose, &QPushButton::clicked, this, &QMainWindow::close);
  connect(mUi->lstLibraries, &QListWidget::currentItemChanged, this,
          &LibraryManager::currentListItemChanged);

  mAddLibraryWidget.reset(new AddLibraryWidget(mWorkspace));
  mUi->verticalLayout->insertWidget(0, mAddLibraryWidget.data());
  connect(mAddLibraryWidget.data(), &AddLibraryWidget::libraryAdded, this,
          &LibraryManager::libraryAddedSlot);

  updateLibraryList();
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &LibraryManager::updateLibraryList);

  // Restore Window Geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("library_manager/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("library_manager/window_state").toByteArray());
}

LibraryManager::~LibraryManager() noexcept {
  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("library_manager/window_geometry", saveGeometry());
  clientSettings.setValue("library_manager/window_state", saveState());

  clearLibraryList();
  mAddLibraryWidget.reset();
  mUi.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LibraryManager::updateRepositoryLibraryList() noexcept {
  mAddLibraryWidget->updateRepositoryLibraryList();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryManager::closeEvent(QCloseEvent* event) noexcept {
  Q_UNUSED(event);
  mWorkspace.getLibraryDb().startLibraryRescan();
}

void LibraryManager::clearLibraryList() noexcept {
  for (int i = mUi->lstLibraries->count() - 1; i >= 0; i--) {
    QListWidgetItem* item = mUi->lstLibraries->item(i);
    Q_ASSERT(item);
    delete mUi->lstLibraries->itemWidget(item);
    delete item;
  }
  Q_ASSERT(mUi->lstLibraries->count() == 0);
}

void LibraryManager::updateLibraryList() noexcept {
  FilePath selectedLibrary = mSelectedLibrary;

  clearLibraryList();

  QList<LibraryListWidgetItem*> widgets;

  // add the "Add new library" item
  widgets.append(new LibraryListWidgetItem(mWorkspace, FilePath()));

  // add all existing libraries
  try {
    QMultiMap<Version, FilePath> libraries =
        mWorkspace.getLibraryDb().getLibraries();  // can throw

    foreach (const FilePath& libDir, libraries) {
      QString name, description, keywords;
      mWorkspace.getLibraryDb().getElementTranslations<Library>(
          libDir, mWorkspace.getSettings().libraryLocaleOrder.get(), &name,
          &description, &keywords);  // can throw
      QPixmap icon;
      mWorkspace.getLibraryDb().getLibraryMetadata(libDir, &icon);  // can throw

      LibraryListWidgetItem* widget = new LibraryListWidgetItem(
          mWorkspace, libDir, name, description, icon);
      connect(widget, &LibraryListWidgetItem::openLibraryEditorTriggered, this,
              &LibraryManager::openLibraryEditorTriggered);
      widgets.append(widget);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not load library list"), e.getMsg());
  }

  // sort all list widget items
  std::sort(widgets.begin(), widgets.end(), widgetsLessThan);

  // populate the list widget
  int selectedLibraryIndex = 0;
  for (int i = 0; i < widgets.count(); ++i) {
    LibraryListWidgetItem* widget = widgets.at(i);
    Q_ASSERT(widget);
    QListWidgetItem* item = new QListWidgetItem(mUi->lstLibraries);
    item->setSizeHint(widget->sizeHint());
    mUi->lstLibraries->setItemWidget(item, widget);
    if (widget->getLibraryFilePath() == selectedLibrary) {
      selectedLibraryIndex = i;
    }
  }

  // select the previously selected library
  mUi->lstLibraries->setCurrentRow(selectedLibraryIndex);
}

void LibraryManager::currentListItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);

  if (mCurrentWidget) {
    delete mCurrentWidget;
    mCurrentWidget = nullptr;
  }

  mSelectedLibrary = FilePath();

  if (current) {
    LibraryListWidgetItem* item = dynamic_cast<LibraryListWidgetItem*>(
        mUi->lstLibraries->itemWidget(current));
    if (item && item->getLibraryFilePath().isValid()) {
      try {
        LibraryInfoWidget* widget = new LibraryInfoWidget(
            mWorkspace, item->getLibraryFilePath());  // can throw
        connect(widget, &LibraryInfoWidget::openLibraryEditorTriggered, this,
                &LibraryManager::openLibraryEditorTriggered);
        mUi->verticalLayout->insertWidget(0, widget);
        mCurrentWidget   = widget;
        mSelectedLibrary = item->getLibraryFilePath();
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
      }
    }
  } else {
    mCurrentWidget = new QWidget();
    mUi->verticalLayout->insertWidget(0, mCurrentWidget);
  }

  mAddLibraryWidget->setVisible(mCurrentWidget ? false : true);
}

void LibraryManager::libraryAddedSlot(const FilePath& libDir) noexcept {
  // Update the selected library and start library scan - the library list will
  // be updated soon (triggered by the workspace library scanner), then the new
  // library will be selected as soon as it appears in the list.
  mSelectedLibrary = libDir;
  mWorkspace.getLibraryDb().startLibraryRescan();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool LibraryManager::widgetsLessThan(const LibraryListWidgetItem* a,
                                     const LibraryListWidgetItem* b) noexcept {
  Q_ASSERT(a && b);
  if (!a->isRemoteLibrary() && b->isRemoteLibrary()) {
    return true;
  } else if (a->isRemoteLibrary() && !b->isRemoteLibrary()) {
    return false;
  } else {
    if (!a->getLibraryFilePath().isValid()) {
      return true;
    } else if (!b->getLibraryFilePath().isValid()) {
      return false;
    } else {
      return a->getName().toLower() < b->getName().toLower();
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb

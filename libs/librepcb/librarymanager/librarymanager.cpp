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
    mCurrentWidget(nullptr) {
  mUi->setupUi(this);
  connect(mUi->btnClose, &QPushButton::clicked, this, &QMainWindow::close);
  connect(mUi->lstLibraries, &QListWidget::currentItemChanged, this,
          &LibraryManager::currentListItemChanged);

  mAddLibraryWidget.reset(new AddLibraryWidget(mWorkspace));
  mUi->verticalLayout->insertWidget(0, mAddLibraryWidget.data());
  connect(mAddLibraryWidget.data(), &AddLibraryWidget::libraryAdded, this,
          &LibraryManager::libraryAddedSlot);

  loadLibraryList();
}

LibraryManager::~LibraryManager() noexcept {
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

void LibraryManager::loadLibraryList() noexcept {
  QList<LibraryListWidgetItem*> widgets;

  // add the "Add new library" item
  widgets.append(new LibraryListWidgetItem(mWorkspace,
                                           QSharedPointer<library::Library>()));

  // add all existing libraries
  QList<QSharedPointer<library::Library>> libraries;
  libraries.append(mWorkspace.getLocalLibraries().values());
  libraries.append(mWorkspace.getRemoteLibraries().values());
  foreach (const QSharedPointer<library::Library>& lib, libraries) {
    LibraryListWidgetItem* widget = new LibraryListWidgetItem(mWorkspace, lib);
    connect(widget, &LibraryListWidgetItem::openLibraryEditorTriggered, this,
            &LibraryManager::openLibraryEditorTriggered);
    widgets.append(widget);
  }

  // sort all list widget items
  qSort(widgets.begin(), widgets.end(), widgetsLessThan);

  // populate the list widget
  foreach (LibraryListWidgetItem* widget, widgets) {
    Q_ASSERT(widget);
    QListWidgetItem* item = new QListWidgetItem(mUi->lstLibraries);
    item->setSizeHint(widget->sizeHint());
    mUi->lstLibraries->setItemWidget(item, widget);
  }

  // select the first item in the list
  mUi->lstLibraries->setCurrentRow(0);
}

void LibraryManager::currentListItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);

  if (mCurrentWidget) {
    delete mCurrentWidget;
    mCurrentWidget = nullptr;
  }

  if (current) {
    LibraryListWidgetItem* item = dynamic_cast<LibraryListWidgetItem*>(
        mUi->lstLibraries->itemWidget(current));
    if (item && (!item->getLibrary().isNull())) {
      QSharedPointer<library::Library> lib = item->getLibrary();
      LibraryInfoWidget* widget = new LibraryInfoWidget(mWorkspace, lib);
      connect(widget, &LibraryInfoWidget::openLibraryEditorTriggered, this,
              &LibraryManager::openLibraryEditorTriggered);
      connect(widget, &LibraryInfoWidget::libraryRemoved, this,
              &LibraryManager::libraryRemovedSlot);
      mUi->verticalLayout->insertWidget(0, widget);
      mCurrentWidget = widget;
    }
  } else {
    mCurrentWidget = new QWidget();
    mUi->verticalLayout->insertWidget(0, mCurrentWidget);
  }

  mAddLibraryWidget->setVisible(mCurrentWidget ? false : true);
}

void LibraryManager::libraryAddedSlot(const FilePath& libDir,
                                      bool            select) noexcept {
  clearLibraryList();
  loadLibraryList();
  mAddLibraryWidget->updateInstalledStatusOfRepositoryLibraries();

  if (select) {
    // select the new item
    for (int i = 0; i < mUi->lstLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstLibraries->item(i);
      Q_ASSERT(item);
      LibraryListWidgetItem* widget = dynamic_cast<LibraryListWidgetItem*>(
          mUi->lstLibraries->itemWidget(item));
      if (widget && widget->getLibrary() &&
          widget->getLibrary()->getFilePath() == libDir) {
        mUi->lstLibraries->setCurrentItem(item);
        break;
      }
    }
  }
}

void LibraryManager::libraryRemovedSlot(const FilePath& libDir) noexcept {
  Q_UNUSED(libDir);
  clearLibraryList();
  loadLibraryList();
  mAddLibraryWidget->updateInstalledStatusOfRepositoryLibraries();
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
    if (a->getLibrary().isNull()) {
      return true;
    } else if (b->getLibrary().isNull()) {
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

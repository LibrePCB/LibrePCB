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
#include "libraryeditor2.h"

#include "../guiapplication.h"
#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "../windowtab.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
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

LibraryEditor2::LibraryEditor2(GuiApplication& app,
                               std::unique_ptr<Library> lib, int uiIndex,
                               QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mApp(app),
    mWorkspace(app.getWorkspace()),
    mLibrary(std::move(lib)),
    mUiIndex(uiIndex),
    mUndoStack(new UndoStack()),
    mManualModificationsMade(false) {
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          [this]() { onUiDataChanged.notify(); });
}

LibraryEditor2::~LibraryEditor2() noexcept {
  // Force closing all tabs.
  emit aboutToBeDestroyed();
  Q_ASSERT(mRegisteredTabs.isEmpty());

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath LibraryEditor2::getFilePath() const noexcept {
  return mLibrary->getDirectory().getAbsPath();
}

bool LibraryEditor2::isWritable() const noexcept {
  return mLibrary->getDirectory().isWritable();
}

void LibraryEditor2::setUiIndex(int index) noexcept {
  if (index != mUiIndex) {
    mUiIndex = index;
    emit uiIndexChanged();
  }
}

ui::LibraryEditorData LibraryEditor2::getUiData() const noexcept {
  return ui::LibraryEditorData{
      true,  // Valid
      q2s(mLibrary->getDirectory().getAbsPath().toNative()),  // Path
      q2s(*mLibrary->getNames().getDefaultValue()),  // Name
      isWritable(),  // Writable
  };
}

void LibraryEditor2::setUiData(const ui::LibraryEditorData& data) noexcept {
  Q_UNUSED(data);
}

bool LibraryEditor2::requestClose() noexcept {
  // Check all opened tabs first.
  for (auto tab : mRegisteredTabs) {
    Q_ASSERT(tab);
    if (tab && (!tab->requestClose())) {
      return false;
    }
  }

  // Then check this library.
  if ((!hasUnsavedChanges()) || (!mLibrary->getDirectory().isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Library?"),
      tr("The library '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mLibrary->getNames().getDefaultValue()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);
  if (choice == QMessageBox::Yes) {
    return save();
  } else if (choice == QMessageBox::No) {
    return true;
  } else {
    return false;
  }

  return true;
}

bool LibraryEditor2::hasUnsavedChanges() const noexcept {
  return mManualModificationsMade || (!mUndoStack->isClean());
}

void LibraryEditor2::setManualModificationsMade() noexcept {
  if (!mManualModificationsMade) {
    mManualModificationsMade = true;
    emit manualModificationsMade();
  }
}

bool LibraryEditor2::save() noexcept {
  try {
    mLibrary->save();
    mLibrary->getDirectory().getFileSystem()->save();
    if (mManualModificationsMade) {
      mManualModificationsMade = false;
      emit manualModificationsMade();
    }
    mUndoStack->setClean();
    mWorkspace.getLibraryDb().startLibraryRescan();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void LibraryEditor2::registerTab(WindowTab& tab) noexcept {
  Q_ASSERT(!mRegisteredTabs.contains(&tab));
  mRegisteredTabs.append(&tab);
}

void LibraryEditor2::unregisterTab(WindowTab& tab) noexcept {
  Q_ASSERT(mRegisteredTabs.contains(&tab));
  mRegisteredTabs.removeAll(&tab);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

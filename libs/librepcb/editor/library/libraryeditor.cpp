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
#include "libraryeditor.h"

#include "../guiapplication.h"
#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "lib/librarytab.h"
#include "libraryeditortab.h"

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

LibraryEditor::LibraryEditor(GuiApplication& app, std::unique_ptr<Library> lib,
                             int uiIndex, QObject* parent) noexcept
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

LibraryEditor::~LibraryEditor() noexcept {
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

FilePath LibraryEditor::getFilePath() const noexcept {
  return mLibrary->getDirectory().getAbsPath();
}

bool LibraryEditor::isWritable() const noexcept {
  return mLibrary->getDirectory().isWritable();
}

void LibraryEditor::setUiIndex(int index) noexcept {
  if (index != mUiIndex) {
    mUiIndex = index;
    emit uiIndexChanged();
  }
}

ui::LibraryData LibraryEditor::getUiData() const noexcept {
  return ui::LibraryData{
      true,  // Valid
      q2s(mLibrary->getDirectory().getAbsPath().toNative()),  // Path
      q2s(*mLibrary->getNames().getDefaultValue()),  // Name
      isWritable(),  // Writable
  };
}

void LibraryEditor::setUiData(const ui::LibraryData& data) noexcept {
  Q_UNUSED(data);
}

bool LibraryEditor::requestCloseAllTabs() noexcept {
  for (auto tab : mRegisteredTabs) {
    Q_ASSERT(tab);
    if (tab && (!tab->requestClose())) {
      return false;
    }
  }
  // If multiple library tabs are open, requestCloseLibrary() was not called
  // by LibraryTab::requestClose(), so we have to do it now.
  if ((getNumberOfLibraryTabs() != 1) && (!requestCloseLibrary())) {
    return false;
  }
  return true;
}

bool LibraryEditor::requestCloseLibrary() noexcept {
  if ((!hasUnsavedChanges()) || (!mLibrary->getDirectory().isWritable())) {
    return true;  // No unsaved changes.
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
}

bool LibraryEditor::hasUnsavedChanges() const noexcept {
  return mManualModificationsMade || (!mUndoStack->isClean());
}

bool LibraryEditor::discardUnsavedChanges() noexcept {
  try {
    while ((!mUndoStack->isClean()) && mUndoStack->canUndo()) {
      mUndoStack->undo();  // can throw
    }
    Q_ASSERT(mUndoStack->isClean());
    mUndoStack->clear();
    if (mManualModificationsMade) {
      mManualModificationsMade = false;
      emit manualModificationsMade();
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void LibraryEditor::setManualModificationsMade() noexcept {
  if (!mManualModificationsMade) {
    mManualModificationsMade = true;
    emit manualModificationsMade();
  }
}

bool LibraryEditor::save() noexcept {
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

void LibraryEditor::registerTab(LibraryEditorTab& tab) noexcept {
  Q_ASSERT(!mRegisteredTabs.contains(&tab));
  mRegisteredTabs.append(&tab);
}

void LibraryEditor::unregisterTab(LibraryEditorTab& tab) noexcept {
  Q_ASSERT(mRegisteredTabs.contains(&tab));
  mRegisteredTabs.removeAll(&tab);

  // When closing the last tab of this library, automatically close the library
  // editor too, to avoid spamming the documents panel with lots of opened
  // libraries, required to be manually closed by the used.
  if (mRegisteredTabs.isEmpty() && (!hasUnsavedChanges())) {
    emit closeRequested();
  }
}

void LibraryEditor::forceClosingTabs(const QSet<FilePath>& fp) noexcept {
  for (auto tab : mRegisteredTabs) {
    Q_ASSERT(tab);
    if (tab && (fp.contains(tab->getDirectoryPath()))) {
      tab->closeEnforced();
    }
  }
}

int LibraryEditor::getNumberOfLibraryTabs() const noexcept {
  int count = 0;
  for (auto tab : mRegisteredTabs) {
    if (dynamic_cast<LibraryTab*>(tab.get())) {
      ++count;
    }
  }
  return count;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

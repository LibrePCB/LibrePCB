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
#include "../utils/slinthelpers.h"

#include <librepcb/core/library/library.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

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
    mUiIndex(uiIndex) {
}

LibraryEditor2::~LibraryEditor2() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

const FilePath& LibraryEditor2::getFilePath() const noexcept {
  return mLibrary->getDirectory().getAbsPath();
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
      mLibrary->getDirectory().isWritable(),  // Writable
  };
}

void LibraryEditor2::setUiData(const ui::LibraryEditorData& data) noexcept {
}

bool LibraryEditor2::requestClose() noexcept {
  /*if ((mUndoStack->isClean() && (!mManualModificationsMade)) ||
      (!mProject->getDirectory().isWritable())) {
    // No unsaved changes or opened in read-only mode or don't save.
    return true;
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Project?"),
      tr("The project '%1' contains unsaved changes.\n"
         "Do you want to save them before closing the project?")
          .arg(*mProject->getName()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);
  if (choice == QMessageBox::Yes) {
    return saveProject();
  } else if (choice == QMessageBox::No) {
    return true;
  } else {
    return false;
  }*/

  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

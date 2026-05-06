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
#include "projectlibrarytab.h"

#include "../../guiapplication.h"
#include "../../mainwindow.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../projecteditor.h"
#include "projectlibrarymodel.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLibraryTab::ProjectLibraryTab(GuiApplication& app, ProjectEditor& editor,
                                     QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mProjectEditor(editor),
    mProject(mProjectEditor.getProject()),
    mModel(new ProjectLibraryModel(mProjectEditor,
                                   mApp.getWorkspace().getLibraryDb())),
    mAllChecked(false),
    mViewportY(0) {
  // Connect project editor.
  connect(&mProjectEditor, &ProjectEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mProjectEditor, &ProjectEditor::aboutToBeDestroyed, this,
          &ProjectLibraryTab::closeEnforced);

  // Connect model.
  connect(mModel.get(), &ProjectLibraryModel::statisticsModified, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(mModel.get(), &ProjectLibraryModel::openTriggered, this,
          &ProjectLibraryTab::openTriggered, Qt::QueuedConnection);
}

ProjectLibraryTab::~ProjectLibraryTab() noexcept {
  deactivate();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int ProjectLibraryTab::getProjectIndex() const noexcept {
  return mProjectEditor.getUiIndex();
}

ui::TabData ProjectLibraryTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mProject.getDirectory().isWritable());
  features.undo = toFs(mProjectEditor.getUndoStack().canUndo());
  features.redo = toFs(mProjectEditor.getUndoStack().canRedo());

  return ui::TabData{
      ui::TabType::ProjectLibrary,  // Type
      q2s(*mProject.getName()),  // Title
      features,  // Features
      !mProject.getDirectory().isWritable(),  // Read-only
      mProjectEditor.hasUnsavedChanges(),  // Unsaved changes
      q2s(mProjectEditor.getUndoStack().getUndoCmdText()),  // Undo text
      q2s(mProjectEditor.getUndoStack().getRedoCmdText()),  // Redo text
      {},  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::ProjectLibraryTabData ProjectLibraryTab::getDerivedUiData() const noexcept {
  return ui::ProjectLibraryTabData{
      mProjectEditor.getUiIndex(),  // Project index
      mModel,  // Items
      mViewportY,  // Items viewport Y
      mModel->getDowngradedCount(),  // Downgraded items
      mModel->getCheckableCount(),  // Checkable items
      mModel->getCheckedCount(),  // Checked items
      mAllChecked,  // All checked
      {},  // Copy to library
  };
}

void ProjectLibraryTab::setDerivedUiData(
    const ui::ProjectLibraryTabData& data) noexcept {
  mViewportY = data.items_viewport_y;

  if (data.all_checked != mAllChecked) {
    mAllChecked = data.all_checked;
    mModel->setAllChecked(mAllChecked);
  }

  const FilePath libFp(s2q(data.copy_to_library));
  if (libFp.isValid() &&
      libFp.isLocatedInDir(mApp.getWorkspace().getLocalLibrariesPath())) {
    QStringList errors;
    if (mModel->copyToLibrary(libFp, errors)) {
      mApp.getWorkspace().getLibraryDb().startLibraryRescan();
    }
    if (!errors.isEmpty()) {
      if (errors.count() > 5) {
        errors = errors.mid(0, 4) +
            QStringList{QString("And %1 more").arg(errors.count() - 4)};
      }
      QMessageBox::critical(getWindow(), tr("Error"),
                            tr("Failed to copy library elements:") % "\n\n- " %
                                errors.join("\n- "));
    }
  }
}

void ProjectLibraryTab::activate() noexcept {
}

void ProjectLibraryTab::deactivate() noexcept {
}

void ProjectLibraryTab::trigger(ui::TabAction a) noexcept {
  mProjectEditor.setCurrentTab(this);

  switch (a) {
    case ui::TabAction::Save: {
      mProjectEditor.saveProject();
      break;
    }
    case ui::TabAction::Undo: {
      mProjectEditor.undo();
      break;
    }
    case ui::TabAction::Redo: {
      mProjectEditor.redo();
      break;
    }
    case ui::TabAction::Apply: {
      mApp.openProjectLibraryUpdater(mProject.getFilepath());
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectLibraryTab::openTriggered(ui::LibraryTreeViewItemType type,
                                      const FilePath& fp) {
  if (!mWindow) return;

  switch (type) {
    case ui::LibraryTreeViewItemType::Device: {
      mWindow->requestDeviceTab(fp);
      break;
    }
    case ui::LibraryTreeViewItemType::Component: {
      mWindow->requestComponentTab(fp);
      break;
    }
    case ui::LibraryTreeViewItemType::Symbol: {
      mWindow->requestSymbolTab(fp);
      break;
    }
    case ui::LibraryTreeViewItemType::Package: {
      mWindow->requestPackageTab(fp);
      break;
    }
    default: {
      break;
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

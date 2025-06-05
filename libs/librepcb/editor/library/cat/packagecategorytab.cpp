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
#include "packagecategorytab.h"

#include "../../undostack.h"
#include "../../workspace/categorytreemodel2.h"
#include "../cmd/cmdlibrarycategoryedit.h"
#include "../libraryeditor2.h"
#include "categorytreebuilder.h"
#include "utils/editortoolbox.h"
#include "utils/slinthelpers.h"
#include "utils/uihelpers.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/packagecategory.h>
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

PackageCategoryTab::PackageCategoryTab(LibraryEditor2& editor,
                                       std::unique_ptr<PackageCategory> cat,
                                       QObject* parent) noexcept
  : WindowTab(editor.getApp(), parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mCategory(std::move(cat)),
    mUndoStack(new UndoStack()),
    mNameParsed(mCategory->getNames().getDefaultValue()),
    mVersionParsed(mCategory->getVersion()),
    mDeprecated(false),
    mParents(new slint::VectorModel<slint::SharedString>()),
    mParentsModel(new CategoryTreeModel2(editor.getWorkspace().getLibraryDb(),
                                         editor.getWorkspace().getSettings(),
                                         CategoryTreeModel2::Filter::PkgCat)) {
  // Connect library editor.
  connect(&mEditor, &LibraryEditor2::aboutToBeDestroyed, this,
          &PackageCategoryTab::closeEnforced);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &PackageCategoryTab::refreshMetadata);

  // Connect library element.
  connect(mCategory.get(), &PackageCategory::namesChanged, this,
          [this]() { onUiDataChanged.notify(); });

  // Refresh content.
  refreshMetadata();
}

PackageCategoryTab::~PackageCategoryTab() noexcept {
  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath PackageCategoryTab::getDirectoryPath() const noexcept {
  return mCategory->getDirectory().getAbsPath();
}

ui::TabData PackageCategoryTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mCategory->getDirectory().isWritable());
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::PackageCategory,  // Type
      mName,  // Title
      features,  // Features
      !mCategory->getDirectory().isWritable(),  // Read-only
      q2s(mUndoStack->getUndoCmdText()),  // Undo text
      q2s(mUndoStack->getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::CategoryTabData PackageCategoryTab::getDerivedUiData() const noexcept {
  return ui::CategoryTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mCategory->getDirectory().getAbsPath().toStr()),  // Path
      mName,  // Name
      mNameError,  // Name error
      mDescription,  // Description
      mKeywords,  // Keywords
      mAuthor,  // Author
      mVersion,  // Version
      mVersionError,  // Version error
      mDeprecated,  // Deprecated
      mParents,  // Parents
      mParentsModel,  // Parents model
      slint::SharedString(),  // New parent
  };
}

void PackageCategoryTab::setDerivedUiData(
    const ui::CategoryTabData& data) noexcept {
  mName = data.name;
  if (auto value = validateElementName(s2q(mName), mNameError)) {
    mNameParsed = *value;
  }
  mDescription = data.description;
  mKeywords = data.keywords;
  mAuthor = data.author;
  mVersion = data.version;
  if (auto value = validateVersion(s2q(mVersion), mVersionError)) {
    mVersionParsed = *value;
  }
  mDeprecated = data.deprecated;

  const QString newParent = s2q(data.new_parent);
  if (!newParent.isEmpty()) {
    try {
      std::unique_ptr<CmdLibraryCategoryEdit> cmd(
          new CmdLibraryCategoryEdit(*mCategory));
      cmd->setParentUuid(Uuid::tryFromString(newParent));
      mUndoStack->execCmd(cmd.release());
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void PackageCategoryTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Apply: {
      commitMetadata();
      refreshMetadata();
      break;
    }
    case ui::TabAction::Save: {
      try {
        commitMetadata();
        mCategory->save();
        mCategory->getDirectory().getFileSystem()->save();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      refreshMetadata();
      break;
    }
    case ui::TabAction::Undo: {
      try {
        mUndoStack->undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        mUndoStack->redo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
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

void PackageCategoryTab::refreshMetadata() noexcept {
  mName = q2s(*mCategory->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mCategory->getNames().getDefaultValue();
  mDescription = q2s(mCategory->getDescriptions().getDefaultValue());
  mKeywords = q2s(mCategory->getKeywords().getDefaultValue());
  mAuthor = q2s(mCategory->getAuthor());
  mVersion = q2s(mCategory->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mCategory->getVersion();
  mDeprecated = mCategory->isDeprecated();

  std::vector<slint::SharedString> parents;
  try {
    CategoryTreeBuilder<PackageCategory> builder(
        mEditor.getWorkspace().getLibraryDb(),
        mEditor.getWorkspace().getSettings().libraryLocaleOrder.get(), true);
    for (auto item : builder.buildTree(mCategory->getParentUuid())) {
      parents.push_back(q2s(item));
    }
  } catch (const Exception& e) {
    parents.push_back(q2s(e.getMsg()));
  }
  mParents->set_vector(parents);

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void PackageCategoryTab::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryCategoryEdit> cmd(
        new CmdLibraryCategoryEdit(*mCategory));
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mCategory->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

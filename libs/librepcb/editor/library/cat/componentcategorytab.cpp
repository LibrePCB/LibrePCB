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
#include "componentcategorytab.h"

#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel.h"
#include "../cmd/cmdlibrarycategoryedit.h"
#include "../libraryeditor.h"
#include "categorytreebuilder.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
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

ComponentCategoryTab::ComponentCategoryTab(
    LibraryEditor& editor, std::unique_ptr<ComponentCategory> cat, Mode mode,
    QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mCategory(std::move(cat)),
    mChooseParent(false),
    mNameParsed(mCategory->getNames().getDefaultValue()),
    mVersionParsed(mCategory->getVersion()),
    mDeprecated(false),
    mParents(new slint::VectorModel<slint::SharedString>()),
    mParentsModel(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                        editor.getWorkspace().getSettings(),
                                        CategoryTreeModel::Filter::CmpCat,
                                        mCategory->getUuid())) {
  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentCategoryTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentCategoryTab::refreshUiData);

  // Refresh content.
  refreshUiData();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (mode == Mode::New) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }

  // Make save button primary if it's a new element.
  if (mode != Mode::Open) {
    mManualModificationsMade = true;
  }
}

ComponentCategoryTab::~ComponentCategoryTab() noexcept {
  deactivate();

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath ComponentCategoryTab::getDirectoryPath() const noexcept {
  return mCategory->getDirectory().getAbsPath();
}

ui::TabData ComponentCategoryTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::ComponentCategory,  // Type
      q2s(*mCategory->getNames().getDefaultValue()),  // Title
      features,  // Features
      !writable,  // Read-only
      hasUnsavedChanges(),  // Unsaved changes
      q2s(mUndoStack->getUndoCmdText()),  // Undo text
      q2s(mUndoStack->getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::CategoryTabData ComponentCategoryTab::getDerivedUiData() const noexcept {
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
      mParentsModel,  // Parents tree
      mChooseParent,  // Choose parent
      ui::RuleCheckData{
          ui::RuleCheckType::ComponentCategoryCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Check execution error
          !isWritable(),  // Check read-only
      },
      slint::SharedString(),  // New parent
  };
}

void ComponentCategoryTab::setDerivedUiData(
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

  mChooseParent = data.choose_parent;
  const QString newParent = s2q(data.new_parent);
  if (!newParent.isEmpty()) {
    mParent = Uuid::tryFromString(newParent);
    commitUiData();
    refreshUiData();
  }

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void ComponentCategoryTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Apply: {
      commitUiData();
      refreshUiData();
      break;
    }
    case ui::TabAction::Save: {
      commitUiData();
      save();
      break;
    }
    case ui::TabAction::Undo: {
      try {
        commitUiData();
        mUndoStack->undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitUiData();
        mUndoStack->redo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Close: {
      if (requestClose()) {
        WindowTab::trigger(a);
      }
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

bool ComponentCategoryTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The component category '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mCategory->getNames().getDefaultValue()),
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

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
    ComponentCategoryTab::runChecksImpl() {
  return std::make_pair(mCategory->runChecks(),
                        mCategory->getMessageApprovals());
}

bool ComponentCategoryTab::autoFixImpl(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool ComponentCategoryTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

void ComponentCategoryTab::messageApprovalChanged(const SExpression& approval,
                                                  bool approved) noexcept {
  if (mCategory->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void ComponentCategoryTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
void ComponentCategoryTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitUiData();
}

template <>
void ComponentCategoryTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ComponentCategoryTab::isWritable() const noexcept {
  return isPathOutsideLibDir() || mCategory->getDirectory().isWritable();
}

void ComponentCategoryTab::refreshUiData() noexcept {
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
  mParent = mCategory->getParentUuid();

  std::vector<slint::SharedString> parents;
  try {
    CategoryTreeBuilder<ComponentCategory> builder(
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

void ComponentCategoryTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdLibraryCategoryEdit> cmd(
        new CmdLibraryCategoryEdit(*mCategory));
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mCategory->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mCategory->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mCategory->getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setParentUuid(mParent);
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool ComponentCategoryTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
    mCategory->setMessageApprovals(mCategory->getMessageApprovals() -
                                   mDisappearedApprovals);

    mCategory->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<ComponentCategory>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mCategory->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mCategory->saveTo(dir);
    }
    mCategory->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mManualModificationsMade = false;
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    refreshUiData();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshUiData();
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

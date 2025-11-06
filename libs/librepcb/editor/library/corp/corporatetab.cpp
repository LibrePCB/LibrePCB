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
#include "corporatetab.h"

#include "../../dialogs/filedialog.h"
#include "../../guiapplication.h"
#include "../../project/board/boardsetupdialog.h"
#include "../../project/outputjobsdialog/outputjobsdialog.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcorporateedit.h"
#include "../cmd/cmdlibrarybaseelementedit.h"
#include "../libraryeditor.h"
#include "corporatepcbproductmodel.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/corp/corporate.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
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

CorporateTab::CorporateTab(LibraryEditor& editor,
                           std::unique_ptr<Corporate> cat, Mode mode,
                           QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mCorporate(std::move(cat)),
    mNameParsed(mCorporate->getNames().getDefaultValue()),
    mVersionParsed(mCorporate->getVersion()),
    mDeprecated(false),
    mPriority(0),
    mPcbProducts(new CorporatePcbProductModel()) {
  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &CorporateTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &CorporateTab::refreshUiData);

  // Connect models.
  mPcbProducts->setReferences(
      mCorporate.get(), mUndoStack.get(), [this](CorporatePcbProduct& product) {
        Project& prj = getTmpProject();
        if (Board* brd = prj.getBoardByIndex(0)) {
          brd->setDrcSettings(product.getDrcSettings(false));
          BoardSetupDialog dlg(mApp, *brd, *mUndoStack);
          dlg.openDrcSettingsTab();
          dlg.hideOtherTabs();
          dlg.exec();
          product.setDrcSettings(brd->getDrcSettings());
        }
      });

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

CorporateTab::~CorporateTab() noexcept {
  deactivate();

  // Reset references to avoid dangling pointers as the UI might still have
  // shared pointers to these models.
  mPcbProducts->setReferences(nullptr, nullptr, nullptr);

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath CorporateTab::getDirectoryPath() const noexcept {
  return mCorporate->getDirectory().getAbsPath();
}

ui::TabData CorporateTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::Corporate,  // Type
      q2s(*mCorporate->getNames().getDefaultValue()),  // Title
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

ui::CorporateTabData CorporateTab::getDerivedUiData() const noexcept {
  return ui::CorporateTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mCorporate->getDirectory().getAbsPath().toStr()),  // Path
      q2s(mCorporate->getLogoPixmap()),  // Logo
      mName,  // Name
      mNameError,  // Name error
      mDescription,  // Description
      mKeywords,  // Keywords
      mAuthor,  // Author
      mVersion,  // Version
      mVersionError,  // Version error
      mDeprecated,  // Deprecated
      mUrl,  // URL
      mUrlError,  // URL error
      mPriority,  // Priority
      mPcbProducts,  // PCB products
      mCorporate->getPcbOutputJobs().count(),  // PCB output jobs
      mCorporate->getAssemblyOutputJobs().count(),  // Assembly output jobs
      ui::RuleCheckData{
          ui::RuleCheckType::CorporateCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Check execution error
          !isWritable(),  // Check read-only
      },
  };
}

void CorporateTab::setDerivedUiData(const ui::CorporateTabData& data) noexcept {
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
  mUrl = data.url;
  validateUrl(s2q(mUrl), mUrlError, true);
  mPriority = data.priority;

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void CorporateTab::trigger(ui::TabAction a) noexcept {
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
    case ui::TabAction::LibraryChooseIcon: {
      const QString fp = FileDialog::getOpenFileName(
          qApp->activeWindow(), tr("Choose Corporate Logo"), QString(),
          tr("Portable Network Graphics (*.png)"));
      if (!fp.isEmpty()) {
        try {
          mLogo = FileUtils::readFile(FilePath(fp));  // can throw
          commitUiData();
        } catch (const Exception& e) {
          QMessageBox::critical(qApp->activeWindow(), tr("Could not open file"),
                                e.getMsg());
        }
      }
      break;
    }
    case ui::TabAction::CorporateAddPcbProduct: {
      mPcbProducts->addProduct();
      break;
    }
    case ui::TabAction::CorporateEditPcbOutputJobs: {
      execOutputJobsDialog(mCorporate->getPcbOutputJobs(),
                           &CmdCorporateEdit::setPcbOutputJobs);
      break;
    }
    case ui::TabAction::CorporateEditAssemblyOutputJobs: {
      execOutputJobsDialog(mCorporate->getAssemblyOutputJobs(),
                           &CmdCorporateEdit::setAssemblyOutputJobs);
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

bool CorporateTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The corporate '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mCorporate->getNames().getDefaultValue()),
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
    CorporateTab::runChecksImpl() {
  return std::make_pair(mCorporate->runChecks(),
                        mCorporate->getMessageApprovals());
}

bool CorporateTab::autoFixImpl(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool CorporateTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (checkOnly) {
        return true;
      } else {
        return autoFix(*m);  // can throw
      }
    }
  }
  return false;
}

void CorporateTab::messageApprovalChanged(const SExpression& approval,
                                          bool approved) noexcept {
  if (mCorporate->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void CorporateTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
bool CorporateTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitUiData();
  return true;
}

template <>
bool CorporateTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool CorporateTab::isWritable() const noexcept {
  return isPathOutsideLibDir() || mCorporate->getDirectory().isWritable();
}

void CorporateTab::refreshUiData() noexcept {
  mLogo = mCorporate->getLogoPng();
  mName = q2s(*mCorporate->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mCorporate->getNames().getDefaultValue();
  mDescription = q2s(mCorporate->getDescriptions().getDefaultValue());
  mKeywords = q2s(mCorporate->getKeywords().getDefaultValue());
  mAuthor = q2s(mCorporate->getAuthor());
  mVersion = q2s(mCorporate->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mCorporate->getVersion();
  mDeprecated = mCorporate->isDeprecated();
  mUrl = q2s(mCorporate->getUrl().toString());
  mUrlError = slint::SharedString();
  mPriority = mCorporate->getPriority();

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void CorporateTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdCorporateEdit> cmd(new CmdCorporateEdit(*mCorporate));
    cmd->setLogoPng(mLogo);
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mCorporate->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mCorporate->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mCorporate->getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    const QString urlStr = s2q(mUrl);
    if (urlStr != mCorporate->getUrl().toString()) {
      cmd->setUrl(QUrl(urlStr.trimmed(), QUrl::TolerantMode));
    }
    cmd->setPriority(mPriority);
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool CorporateTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
    mCorporate->setMessageApprovals(mCorporate->getMessageApprovals() -
                                    mDisappearedApprovals);

    mCorporate->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Corporate>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mCorporate->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mCorporate->saveTo(dir);
    }
    mCorporate->getDirectory().getFileSystem()->save();
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

void CorporateTab::execOutputJobsDialog(
    const OutputJobList& jobs,
    void (CmdCorporateEdit::*setter)(const OutputJobList& jobs)) noexcept {
  try {
    Project& prj = getTmpProject();
    prj.getOutputJobs() = jobs;
    UndoStack undoStack;
    OutputJobsDialog dlg(mApp.getWorkspace(), mApp.getLibraryElementCache(),
                         prj, undoStack);
    dlg.exec();
    std::unique_ptr<CmdCorporateEdit> cmd(new CmdCorporateEdit(*mCorporate));
    ((*cmd).*setter)(prj.getOutputJobs());
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

Project& CorporateTab::getTmpProject() {
  if (!mTmpProject) {
    auto fs = TransactionalFileSystem::openRO(FilePath::getRandomTempPath());
    mTmpProject = Project::create(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
        "tmp.lpp");
  }
  if (mTmpProject->getBoards().isEmpty()) {
    Board* brd = new Board(
        *mTmpProject,
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            mTmpProject->getDirectory(), "boards/board")),
        "board", Uuid::createRandom(), ElementName("board"));
    mTmpProject->addBoard(*brd);
  }
  return *mTmpProject.get();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

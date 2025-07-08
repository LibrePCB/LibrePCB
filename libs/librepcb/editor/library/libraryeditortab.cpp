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
#include "libraryeditortab.h"

#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "libraryeditor.h"
#include "rulecheck/rulecheckmessagesmodel.h"

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

LibraryEditorTab::LibraryEditorTab(LibraryEditor& editor,
                                   QObject* parent) noexcept
  : WindowTab(editor.getApp(), parent),
    mEditor(editor),
    mUndoStack(new UndoStack()),
    mManualModificationsMade(false),
    mCheckMessages(new RuleCheckMessagesModel()) {
  // Connect library editor.
  mEditor.registerTab(*this);
  connect(&mEditor, &LibraryEditor::uiIndexChanged, this,
          &LibraryEditorTab::notifyDerivedUiDataChanged);
  connect(&mEditor, &LibraryEditor::aboutToBeDestroyed, this,
          &LibraryEditorTab::closeEnforced);

  // Connect models.
  mCheckMessages->setAutofixHandler(std::bind(&LibraryEditorTab::autoFixHandler,
                                              this, std::placeholders::_1,
                                              std::placeholders::_2));
  connect(mCheckMessages.get(), &RuleCheckMessagesModel::unapprovedCountChanged,
          this, &LibraryEditorTab::notifyDerivedUiDataChanged);
  connect(mCheckMessages.get(), &RuleCheckMessagesModel::errorCountChanged,
          this, &LibraryEditorTab::notifyDerivedUiDataChanged);
  connect(mCheckMessages.get(), &RuleCheckMessagesModel::approvalChanged, this,
          &LibraryEditorTab::messageApprovalChanged);

  // Setup rule checks timer.
  mRuleCheckDelayTimer.setSingleShot(true);
  mRuleCheckDelayTimer.setInterval(100);
  connect(&mRuleCheckDelayTimer, &QTimer::timeout, this,
          &LibraryEditorTab::runChecks);
}

LibraryEditorTab::~LibraryEditorTab() noexcept {
  Q_ASSERT(!mUndoStack);  // Must be reset in derived class.
  mRuleCheckDelayTimer.stop();
  mCheckMessages->setAutofixHandler(nullptr);
  mEditor.unregisterTab(*this);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

bool LibraryEditorTab::isPathOutsideLibDir() const noexcept {
  const FilePath fp = getDirectoryPath();
  return (fp != mEditor.getFilePath()) &&
      (!fp.isLocatedInDir(mEditor.getFilePath()));
}

bool LibraryEditorTab::hasUnsavedChanges() const noexcept {
  return mManualModificationsMade || (!mUndoStack->isClean());
}

void LibraryEditorTab::scheduleChecks() noexcept {
  mRuleCheckDelayTimer.start();
}

void LibraryEditorTab::runChecks() noexcept {
  slint::SharedString err = mCheckError;

  try {
    if (auto result = runChecksImpl()) {  // can throw
      const auto approvals = RuleCheckMessage::getAllApprovals(result->first);
      mSupportedApprovals |= approvals;
      mDisappearedApprovals = mSupportedApprovals - approvals;
      mCheckMessages->setMessages(result->first, result->second);
      mCheckError = slint::SharedString();
    }
  } catch (const Exception& e) {
    mCheckMessages->clear();
    err = q2s(e.getMsg());
  }

  if (err != mCheckError) {
    mCheckError = err;
    notifyDerivedUiDataChanged();
  }
}

QString LibraryEditorTab::getWorkspaceSettingsUserName() const noexcept {
  QString u = mEditor.getWorkspace().getSettings().userName.get();
  if (u.isEmpty()) {
    QMessageBox::warning(
        qApp->activeWindow(), tr("User name not set"),
        tr("No user name is defined in the workspace settings. Please open "
           "the workspace settings to set a default user name."));
  }
  return u;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool LibraryEditorTab::autoFixHandler(
    const std::shared_ptr<const RuleCheckMessage>& msg,
    bool checkOnly) noexcept {
  try {
    return autoFixImpl(msg, checkOnly);  // can throw
  } catch (const Exception& e) {
    if (!checkOnly) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

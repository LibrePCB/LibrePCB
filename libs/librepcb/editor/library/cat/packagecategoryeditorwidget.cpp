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
#include "packagecategoryeditorwidget.h"

#include "../cmd/cmdlibrarycategoryedit.h"
#include "categorychooserdialog.h"
#include "categorytreelabeltextbuilder.h"
#include "ui_packagecategoryeditorwidget.h"

#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/workspace/workspace.h>

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

PackageCategoryEditorWidget::PackageCategoryEditorWidget(const Context& context,
                                                         const FilePath& fp,
                                                         QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::PackageCategoryEditorWidget) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setReadOnly(mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->btnChooseParentCategory->setEnabled(!mContext.readOnly);
  mUi->btnResetParentCategory->setEnabled(!mContext.readOnly);
  setWindowIcon(QIcon(":/img/places/folder_green.png"));
  connect(mUi->btnChooseParentCategory, &QToolButton::clicked, this,
          &PackageCategoryEditorWidget::btnChooseParentCategoryClicked);
  connect(mUi->btnResetParentCategory, &QToolButton::clicked, this,
          &PackageCategoryEditorWidget::btnResetParentCategoryClicked);

  // Load element.
  mCategory = PackageCategory::open(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem)));  // can throw
  updateMetadata();

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &PackageCategoryEditorWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &PackageCategoryEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &PackageCategoryEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &PackageCategoryEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &PackageCategoryEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &PackageCategoryEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &PackageCategoryEditorWidget::commitMetadata);
}

PackageCategoryEditorWidget::~PackageCategoryEditorWidget() noexcept {
  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature>
    PackageCategoryEditorWidget::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Close,
  };
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool PackageCategoryEditorWidget::save() noexcept {
  // Remove obsolete message approvals (bypassing the undo stack).
  mCategory->setMessageApprovals(mCategory->getMessageApprovals() -
                                 mDisappearedApprovals);

  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mCategory->save();  // can throw
    mFileSystem->save();  // can throw
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageCategoryEditorWidget::updateMetadata() noexcept {
  setWindowTitle(*mCategory->getNames().getDefaultValue());
  mUi->edtName->setText(*mCategory->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mCategory->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mCategory->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mCategory->getAuthor());
  mUi->edtVersion->setText(mCategory->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mCategory->isDeprecated());
  mUi->lstMessages->setApprovals(mCategory->getMessageApprovals());
  mParentUuid = mCategory->getParentUuid();
  updateCategoryLabel();
}

QString PackageCategoryEditorWidget::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryCategoryEdit> cmd(
        new CmdLibraryCategoryEdit(*mCategory));
    try {
      // throws on invalid name
      cmd->setName("", ElementName(mUi->edtName->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    cmd->setKeywords("", mUi->edtKeywords->text().trimmed());
    try {
      // throws on invalid version
      cmd->setVersion(Version::fromString(mUi->edtVersion->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setDeprecated(mUi->cbxDeprecated->isChecked());
    cmd->setParentUuid(mParentUuid);

    // Commit all changes.
    mUndoStack->execCmd(cmd.release());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool PackageCategoryEditorWidget::runChecks(RuleCheckMessageList& msgs) const {
  msgs = mCategory->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void PackageCategoryEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void PackageCategoryEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <typename MessageType>
bool PackageCategoryEditorWidget::fixMsgHelper(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool PackageCategoryEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  return false;
}

void PackageCategoryEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  setMessageApproved(*mCategory, msg, approve);
  updateMetadata();
}

void PackageCategoryEditorWidget::btnChooseParentCategoryClicked() noexcept {
  CategoryChooserDialog dialog(mContext.workspace,
                               CategoryChooserDialog::Filter::PkgCat);
  if (dialog.exec()) {
    mParentUuid = dialog.getSelectedCategoryUuid();
    commitMetadata();
  }
}

void PackageCategoryEditorWidget::btnResetParentCategoryClicked() noexcept {
  mParentUuid = std::nullopt;
  commitMetadata();
}

void PackageCategoryEditorWidget::updateCategoryLabel() noexcept {
  const WorkspaceLibraryDb& db = mContext.workspace.getLibraryDb();
  PackageCategoryTreeLabelTextBuilder textBuilder(db, getLibLocaleOrder(), true,
                                                  *mUi->lblParentCategories);
  textBuilder.updateText(mParentUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

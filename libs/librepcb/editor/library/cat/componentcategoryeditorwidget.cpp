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
#include "componentcategoryeditorwidget.h"

#include "../cmd/cmdlibrarybaseelementedit.h"
#include "../cmd/cmdlibrarycategoryedit.h"
#include "categorychooserdialog.h"
#include "categorytreelabeltextbuilder.h"
#include "ui_componentcategoryeditorwidget.h"

#include <librepcb/core/library/cat/componentcategory.h>
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

ComponentCategoryEditorWidget::ComponentCategoryEditorWidget(
    const Context& context, const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::ComponentCategoryEditorWidget) {
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
  setWindowIcon(QIcon(":/img/places/folder.png"));
  connect(mUi->btnChooseParentCategory, &QToolButton::clicked, this,
          &ComponentCategoryEditorWidget::btnChooseParentCategoryClicked);
  connect(mUi->btnResetParentCategory, &QToolButton::clicked, this,
          &ComponentCategoryEditorWidget::btnResetParentCategoryClicked);

  // Load element.
  mCategory = ComponentCategory::open(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem)));  // can throw
  updateMetadata();

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &ComponentCategoryEditorWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &ComponentCategoryEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &ComponentCategoryEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &ComponentCategoryEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &ComponentCategoryEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &ComponentCategoryEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &ComponentCategoryEditorWidget::commitMetadata);
}

ComponentCategoryEditorWidget::~ComponentCategoryEditorWidget() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature>
    ComponentCategoryEditorWidget::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Close,
  };
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool ComponentCategoryEditorWidget::save() noexcept {
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

void ComponentCategoryEditorWidget::updateMetadata() noexcept {
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

QString ComponentCategoryEditorWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdLibraryCategoryEdit> cmd(
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
    mUndoStack->execCmd(cmd.take());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool ComponentCategoryEditorWidget::runChecks(
    RuleCheckMessageList& msgs) const {
  msgs = mCategory->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void ComponentCategoryEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void ComponentCategoryEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <typename MessageType>
bool ComponentCategoryEditorWidget::fixMsgHelper(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool ComponentCategoryEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  return false;
}

void ComponentCategoryEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  setMessageApproved(*mCategory, msg, approve);
  updateMetadata();
}

void ComponentCategoryEditorWidget::btnChooseParentCategoryClicked() noexcept {
  CategoryChooserDialog dialog(mContext.workspace,
                               CategoryChooserDialog::Filter::CmpCat);
  if (dialog.exec()) {
    mParentUuid = dialog.getSelectedCategoryUuid();
    commitMetadata();
  }
}

void ComponentCategoryEditorWidget::btnResetParentCategoryClicked() noexcept {
  mParentUuid = tl::nullopt;
  commitMetadata();
}

void ComponentCategoryEditorWidget::updateCategoryLabel() noexcept {
  const WorkspaceLibraryDb& db = mContext.workspace.getLibraryDb();
  ComponentCategoryTreeLabelTextBuilder textBuilder(
      db, getLibLocaleOrder(), true, *mUi->lblParentCategories);
  textBuilder.updateText(mParentUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

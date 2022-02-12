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
#include "componenteditorwidget.h"

#include "../../widgets/signalrolecombobox.h"
#include "../cmd/cmdcomponentedit.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "componentsymbolvarianteditdialog.h"
#include "ui_componenteditorwidget.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/msg/msgmissingcomponentdefaultvalue.h>
#include <librepcb/core/library/cmp/msg/msgmissingsymbolvariant.h>
#include <librepcb/core/library/msg/msgmissingauthor.h>
#include <librepcb/core/library/msg/msgmissingcategories.h>
#include <librepcb/core/library/msg/msgnamenottitlecase.h>

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

ComponentEditorWidget::ComponentEditorWidget(const Context& context,
                                             const FilePath& fp,
                                             QWidget* parent)
  : EditorWidgetBase(context, fp, parent), mUi(new Ui::ComponentEditorWidget) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setProvideFixes(!mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->cbxSchematicOnly->setCheckable(!mContext.readOnly);
  mUi->edtPrefix->setReadOnly(mContext.readOnly);
  mUi->edtDefaultValue->setReadOnly(mContext.readOnly);
  mUi->signalEditorWidget->setReadOnly(mContext.readOnly);
  mUi->symbolVariantsEditorWidget->setReadOnly(mContext.readOnly);
  mUi->attributesEditorWidget->setReadOnly(mContext.readOnly);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  setWindowIcon(QIcon(":/img/library/component.png"));

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(new CategoryListEditorWidget(
      mContext.workspace, CategoryListEditorWidget::Categories::Component,
      this));
  mCategoriesEditorWidget->setReadOnly(mContext.readOnly);
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mComponent.reset(new Component(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem))));  // can throw
  mUi->signalEditorWidget->setReferences(mUndoStack.data(),
                                         &mComponent->getSignals());
  mUi->symbolVariantsEditorWidget->setReferences(
      mUndoStack.data(), &mComponent->getSymbolVariants(), this);
  updateMetadata();

  // Load attribute editor.
  mUi->attributesEditorWidget->setReferences(mUndoStack.data(),
                                             &mComponent->getAttributes());

  // Show "interface broken" warning when related properties are modified.
  memorizeComponentInterface();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);
  connect(mUi->cbxSchematicOnly, &QCheckBox::toggled, this,
          &ComponentEditorWidget::undoStackStateModified);

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &ComponentEditorWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mCategoriesEditorWidget.data(), &CategoryListEditorWidget::edited,
          this, &ComponentEditorWidget::commitMetadata);
  connect(mUi->cbxSchematicOnly, &QCheckBox::clicked, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtPrefix, &QLineEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
  connect(mUi->edtDefaultValue, &PlainTextEdit::editingFinished, this,
          &ComponentEditorWidget::commitMetadata);
}

ComponentEditorWidget::~ComponentEditorWidget() noexcept {
  mUi->attributesEditorWidget->setReferences(nullptr, nullptr);
  mUi->signalEditorWidget->setReferences(nullptr, nullptr);
  mUi->symbolVariantsEditorWidget->setReferences(nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool ComponentEditorWidget::save() noexcept {
  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mComponent->save();  // can throw
    mFileSystem->save();  // can throw
    memorizeComponentInterface();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentEditorWidget::updateMetadata() noexcept {
  setWindowTitle(*mComponent->getNames().getDefaultValue());
  mUi->edtName->setText(*mComponent->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mComponent->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mComponent->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mComponent->getAuthor());
  mUi->edtVersion->setText(mComponent->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mComponent->isDeprecated());
  mCategoriesEditorWidget->setUuids(mComponent->getCategories());
  mUi->cbxSchematicOnly->setChecked(mComponent->isSchematicOnly());
  mUi->edtPrefix->setText(*mComponent->getPrefixes().getDefaultValue());
  mUi->edtDefaultValue->setPlainText(mComponent->getDefaultValue());
}

QString ComponentEditorWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdComponentEdit> cmd(new CmdComponentEdit(*mComponent));
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
    cmd->setCategories(mCategoriesEditorWidget->getUuids());
    cmd->setIsSchematicOnly(mUi->cbxSchematicOnly->isChecked());
    try {
      // throws on invalid prefix
      cmd->setPrefix("", ComponentPrefix(mUi->edtPrefix->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDefaultValue(mUi->edtDefaultValue->toPlainText().trimmed());

    // Commit all changes.
    mUndoStack->execCmd(cmd.take());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool ComponentEditorWidget::openComponentSymbolVariantEditor(
    ComponentSymbolVariant& variant) noexcept {
  ComponentSymbolVariantEditDialog dialog(mContext.workspace, *mComponent,
                                          variant);
  dialog.setReadOnly(mContext.readOnly);
  return (dialog.exec() == QDialog::Accepted);
}

void ComponentEditorWidget::memorizeComponentInterface() noexcept {
  mOriginalIsSchematicOnly = mComponent->isSchematicOnly();
  mOriginalSignalUuids = mComponent->getSignals().getUuidSet();
  mOriginalSymbolVariants = mComponent->getSymbolVariants();
}

bool ComponentEditorWidget::isInterfaceBroken() const noexcept {
  if (mUi->cbxSchematicOnly->isChecked() != mOriginalIsSchematicOnly)
    return true;
  if (mComponent->getSignals().getUuidSet() != mOriginalSignalUuids)
    return true;
  for (const ComponentSymbolVariant& original : mOriginalSymbolVariants) {
    const ComponentSymbolVariant* current =
        mComponent->getSymbolVariants().find(original.getUuid()).get();
    if (!current) return true;
    if (current->getSymbolItems().getUuidSet() !=
        original.getSymbolItems().getUuidSet())
      return true;
    for (const ComponentSymbolVariantItem& originalItem :
         original.getSymbolItems()) {
      const ComponentSymbolVariantItem* currentItem =
          current->getSymbolItems().find(originalItem.getUuid()).get();
      if (currentItem->getSymbolUuid() != originalItem.getSymbolUuid())
        return true;
      if (currentItem->getPinSignalMap().getUuidSet() !=
          originalItem.getPinSignalMap().getUuidSet())
        return true;
      for (const ComponentPinSignalMapItem& originalMap :
           originalItem.getPinSignalMap()) {
        const ComponentPinSignalMapItem* currentMap =
            currentItem->getPinSignalMap().find(originalMap.getUuid()).get();
        if (!currentMap) return true;
        if (currentMap->getSignalUuid() != originalMap.getSignalUuid())
          return true;
      }
    }
  }
  return false;
}

bool ComponentEditorWidget::runChecks(
    LibraryElementCheckMessageList& msgs) const {
  msgs = mComponent->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void ComponentEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void ComponentEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void ComponentEditorWidget::fixMsg(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCategoriesEditorWidget->openAddCategoryDialog();
}

template <>
void ComponentEditorWidget::fixMsg(const MsgMissingComponentDefaultValue& msg) {
  Q_UNUSED(msg);
  // User has to answer the one-million-dollar question :-)
  QString title = tr("Determine default value");
  QString question =
      tr("Is this rather a (manufacturer-)specific component than a generic "
         "component?");
  int answer = QMessageBox::question(this, title, question, QMessageBox::Cancel,
                                     QMessageBox::Yes, QMessageBox::No);
  if (answer == QMessageBox::Yes) {
    mUi->edtDefaultValue->setPlainText("{{PARTNUMBER or DEVICE or COMPONENT}}");
    commitMetadata();
  } else if (answer == QMessageBox::No) {
    mUi->edtDefaultValue->setPlainText("{{PARTNUMBER or DEVICE}}");
    commitMetadata();
  }
}

template <>
void ComponentEditorWidget::fixMsg(const MsgMissingSymbolVariant& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<ComponentSymbolVariant> symbVar =
      std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), "",
                                               ElementName("default"), "");
  mUndoStack->execCmd(new CmdComponentSymbolVariantInsert(
      mComponent->getSymbolVariants(), symbVar));
}

template <typename MessageType>
bool ComponentEditorWidget::fixMsgHelper(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool ComponentEditorWidget::processCheckMessage(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingComponentDefaultValue>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingSymbolVariant>(msg, applyFix)) return true;
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

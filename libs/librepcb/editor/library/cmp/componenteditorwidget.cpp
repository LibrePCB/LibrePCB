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
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdcomponentedit.h"
#include "../cmd/cmdcomponentsignaledit.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "componentsymbolvarianteditdialog.h"
#include "ui_componenteditorwidget.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentcheckmessages.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
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

ComponentEditorWidget::ComponentEditorWidget(const Context& context,
                                             const FilePath& fp,
                                             QWidget* parent)
  : EditorWidgetBase(context, fp, parent), mUi(new Ui::ComponentEditorWidget) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setReadOnly(mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->edtDatasheet->setReadOnly(mContext.readOnly);
  connect(mUi->btnDownloadDatasheet, &QToolButton::clicked, this, [this]() {
    if (auto dbRes = mComponent->getResources().value(0)) {
      DesktopServices::downloadAndOpenResourceAsync(
          mContext.workspace.getSettings(), *dbRes->getName(),
          dbRes->getMediaType(), dbRes->getUrl(), this);
    }
  });
  mUi->cbxSchematicOnly->setCheckable(!mContext.readOnly);
  mUi->edtPrefix->setReadOnly(mContext.readOnly);
  mUi->edtDefaultValue->setReadOnly(mContext.readOnly);
  mUi->signalEditorWidget->setFrameStyle(QFrame::NoFrame);
  mUi->signalEditorWidget->setReadOnly(mContext.readOnly);
  mUi->symbolVariantsEditorWidget->setFrameStyle(QFrame::NoFrame);
  mUi->symbolVariantsEditorWidget->setReadOnly(mContext.readOnly);
  mUi->attributesEditorWidget->setFrameStyle(QFrame::NoFrame);
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
  mComponent.reset(Component::open(std::unique_ptr<TransactionalDirectory>(
                                       new TransactionalDirectory(mFileSystem)))
                       .release());  // can throw
  mUi->signalEditorWidget->setReferences(mUndoStack.data(),
                                         &mComponent->getSignals());
  mUi->symbolVariantsEditorWidget->setReferences(
      mUndoStack.data(), &mComponent->getSymbolVariants(), this);

  // Load attribute editor.
  mUi->attributesEditorWidget->setReferences(mUndoStack.data(),
                                             &mComponent->getAttributes());

  // Show "interface broken" warning when related properties are modified.
  connect(mUi->cbxSchematicOnly, &QCheckBox::toggled, this,
          &ComponentEditorWidget::undoStackStateModified);
}

ComponentEditorWidget::~ComponentEditorWidget() noexcept {
  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();

  // Disconnect UI from library element to avoid dangling pointers.
  mUi->attributesEditorWidget->setReferences(nullptr, nullptr);
  mUi->signalEditorWidget->setReferences(nullptr, nullptr);
  mUi->symbolVariantsEditorWidget->setReferences(nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature> ComponentEditorWidget::getAvailableFeatures()
    const noexcept {
  return {
      EditorWidgetBase::Feature::Close,
  };
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool ComponentEditorWidget::save() noexcept {
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ComponentEditorWidget::openComponentSymbolVariantEditor(
    std::shared_ptr<ComponentSymbolVariant> variant) noexcept {
  ComponentSymbolVariantEditDialog dialog(mContext.workspace, mComponent,
                                          variant);
  dialog.setReadOnly(mContext.readOnly);
  return (dialog.exec() == QDialog::Accepted);
}

bool ComponentEditorWidget::isInterfaceBroken() const noexcept {
  return false;
}

bool ComponentEditorWidget::runChecks(RuleCheckMessageList& msgs) const {
  return true;
}

bool ComponentEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  return false;
}

void ComponentEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

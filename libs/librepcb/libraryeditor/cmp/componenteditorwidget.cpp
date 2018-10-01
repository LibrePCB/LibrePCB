/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "componentsymbolvarianteditdialog.h"
#include "ui_componenteditorwidget.h"

#include <librepcb/common/widgets/signalrolecombobox.h>
#include <librepcb/library/cmp/component.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentEditorWidget::ComponentEditorWidget(const Context&  context,
                                             const FilePath& fp,
                                             QWidget*        parent)
  : EditorWidgetBase(context, fp, parent), mUi(new Ui::ComponentEditorWidget) {
  mUi->setupUi(this);
  setWindowIcon(QIcon(":/img/library/component.png"));
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &QWidget::setWindowTitle);

  // insert category list editor widget
  mCategoriesEditorWidget.reset(
      new ComponentCategoryListEditorWidget(mContext.workspace, this));
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int                   row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // load component
  mComponent.reset(new Component(fp, false));  // can throw
  setWindowTitle(*mComponent->getNames().value(getLibLocaleOrder()));
  mUi->lblUuid->setText(QString("<a href=\"%1\">%2</a>")
                            .arg(mComponent->getFilePath().toQUrl().toString(),
                                 mComponent->getUuid().toStr()));
  mUi->lblUuid->setToolTip(mComponent->getFilePath().toNative());
  mUi->edtName->setText(*mComponent->getNames().value(getLibLocaleOrder()));
  mUi->edtDescription->setPlainText(
      mComponent->getDescriptions().value(getLibLocaleOrder()));
  mUi->edtKeywords->setText(
      mComponent->getKeywords().value(getLibLocaleOrder()));
  mUi->edtAuthor->setText(mComponent->getAuthor());
  mUi->edtVersion->setText(mComponent->getVersion().toStr());
  mCategoriesEditorWidget->setUuids(mComponent->getCategories());
  mUi->cbxDeprecated->setChecked(mComponent->isDeprecated());
  mUi->cbxSchematicOnly->setChecked(mComponent->isSchematicOnly());
  mUi->edtPrefix->setText(*mComponent->getPrefixes().getDefaultValue());
  mUi->edtDefaultValue->setPlainText(mComponent->getDefaultValue());
  mUi->signalEditorWidget->setReferences(mUndoStack.data(),
                                         &mComponent->getSignals());
  mUi->symbolVariantsEditorWidget->setReferences(
      mUndoStack.data(), &mComponent->getSymbolVariants(), this);

  // load attributes
  mUi->attributesEditorWidget->setAttributeList(mComponent->getAttributes());
  connect(mUi->attributesEditorWidget, &AttributeListEditorWidget::edited, this,
          &ComponentEditorWidget::setDirty);

  // show "interface broken" warning when related properties are modified
  memorizeComponentInterface();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);
  connect(mUi->cbxSchematicOnly, &QCheckBox::toggled, this,
          &ComponentEditorWidget::undoStackStateModified);

  // show "no categories selected" warning if no categories selected
  mUi->lblWarnAboutMissingCategory->setVisible(
      mComponent->getCategories().isEmpty());

  // set dirty state when properties are modified
  connect(mUi->edtName, &QLineEdit::textEdited, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtKeywords, &QLineEdit::textEdited, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtAuthor, &QLineEdit::textEdited, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtVersion, &QLineEdit::textEdited, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &ComponentEditorWidget::setDirty);
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::categoryAdded, this,
          &ComponentEditorWidget::categoriesUpdated);
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::categoryRemoved, this,
          &ComponentEditorWidget::categoriesUpdated);
  connect(mUi->cbxSchematicOnly, &QCheckBox::clicked, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtPrefix, &QLineEdit::textEdited, this,
          &ComponentEditorWidget::setDirty);
  connect(mUi->edtDefaultValue, &QPlainTextEdit::textChanged, this,
          &ComponentEditorWidget::setDirty);
}

ComponentEditorWidget::~ComponentEditorWidget() noexcept {
  mUi->signalEditorWidget->setReferences(nullptr, nullptr);
  mUi->symbolVariantsEditorWidget->setReferences(nullptr, nullptr, nullptr);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool ComponentEditorWidget::save() noexcept {
  try {
    ElementName name(mUi->edtName->text().trimmed());  // can throw
    Version     version =
        Version::fromString(mUi->edtVersion->text().trimmed());  // can throw
    ComponentPrefix prefix(mUi->edtPrefix->text().trimmed());    // can throw

    mComponent->setName("", name);
    mComponent->setDescription("",
                               mUi->edtDescription->toPlainText().trimmed());
    mComponent->setKeywords("", mUi->edtKeywords->text().trimmed());
    mComponent->setAuthor(mUi->edtAuthor->text().trimmed());
    mComponent->setVersion(version);
    mComponent->setCategories(mCategoriesEditorWidget->getUuids());
    mComponent->setDeprecated(mUi->cbxDeprecated->isChecked());
    mComponent->getPrefixes().setDefaultValue(prefix);
    mComponent->setDefaultValue(mUi->edtDefaultValue->toPlainText().trimmed());
    mComponent->getAttributes() =
        mUi->attributesEditorWidget->getAttributeList();
    mComponent->save();
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

bool ComponentEditorWidget::openComponentSymbolVariantEditor(
    ComponentSymbolVariant& variant) noexcept {
  ComponentSymbolVariantEditDialog dialog(
      mContext.workspace, mContext.layerProvider, *mComponent, variant);
  return (dialog.exec() == QDialog::Accepted);
}

void ComponentEditorWidget::memorizeComponentInterface() noexcept {
  mOriginalIsSchematicOnly = mComponent->isSchematicOnly();
  mOriginalSignalUuids     = mComponent->getSignals().getUuidSet();
  mOriginalSymbolVariants  = mComponent->getSymbolVariants();
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

void ComponentEditorWidget::categoriesUpdated() noexcept {
  mUi->lblWarnAboutMissingCategory->setVisible(
      mCategoriesEditorWidget->getUuids().isEmpty());
  setDirty();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

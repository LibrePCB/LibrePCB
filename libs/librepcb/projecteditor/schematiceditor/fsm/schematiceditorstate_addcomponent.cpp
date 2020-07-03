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
#include "schematiceditorstate_addcomponent.h"

#include "../../cmd/cmdaddcomponenttocircuit.h"
#include "../../cmd/cmdaddsymboltoschematic.h"
#include "../../dialogs/addcomponentdialog.h"
#include "ui_schematiceditor.h"

#include <librepcb/common/attributes/attributetype.h>
#include <librepcb/common/attributes/attributeunit.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/attributeunitcombobox.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcb/project/schematics/items/si_symbol.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorState_AddComponent::SchematicEditorState_AddComponent(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mAddComponentDialog(nullptr),
    mLastAngle(0),
    mCurrentComponent(nullptr),
    mCurrentSymbVarItemIndex(-1),
    mCurrentSymbolToPlace(nullptr),
    mCurrentSymbolEditCommand(nullptr),
    // command toolbar actions / widgets:
    mValueLabel(nullptr),
    mValueComboBox(nullptr),
    mAttributeValueEdit(nullptr),
    mAttributeValueEditAction(nullptr),
    mAttributeUnitComboBox(nullptr),
    mAttributeUnitComboBoxAction(nullptr) {
}

SchematicEditorState_AddComponent::
    ~SchematicEditorState_AddComponent() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddComponent::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);
  mLastAngle.setAngleMicroDeg(0);

  // add the "Value:" label to the toolbar
  mValueLabel = new QLabel(tr("Value:"));
  mValueLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mValueLabel);

  // add the value text edit to the toolbar
  mValueComboBox = new QComboBox();
  mValueComboBox->setEditable(true);
  mValueComboBox->setFixedHeight(QLineEdit().sizeHint().height());
  mValueComboBox->setMinimumWidth(200);
  mValueComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  mContext.editorUi.commandToolbar->addWidget(mValueComboBox);

  // add the attribute text edit to the toolbar
  mAttributeValueEdit = new QLineEdit();
  mAttributeValueEdit->setClearButtonEnabled(true);
  mAttributeValueEdit->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Fixed);
  mAttributeValueEditAction =
      mContext.editorUi.commandToolbar->addWidget(mAttributeValueEdit);

  // add the attribute unit combobox to the toolbar
  mAttributeUnitComboBox = new AttributeUnitComboBox();
  mAttributeUnitComboBox->setFixedHeight(QLineEdit().sizeHint().height());
  mAttributeUnitComboBoxAction =
      mContext.editorUi.commandToolbar->addWidget(mAttributeUnitComboBox);

  // Update attribute toolbar widgets and start watching for modifications
  updateValueToolbar();
  updateAttributeToolbar();
  setFocusToToolbar();
  connect(mValueComboBox, &QComboBox::currentTextChanged, this,
          &SchematicEditorState_AddComponent::valueChanged);
  connect(mAttributeValueEdit, &QLineEdit::textChanged, this,
          &SchematicEditorState_AddComponent::attributeChanged);
  connect(mAttributeUnitComboBox, &AttributeUnitComboBox::currentItemChanged,
          this, &SchematicEditorState_AddComponent::attributeChanged);

  return true;
}

bool SchematicEditorState_AddComponent::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;
  Q_ASSERT(mIsUndoCmdActive == false);

  // Delete the "Add Component" dialog
  mAddComponentDialog.reset();

  // Remove actions / widgets from the "command" toolbar
  mAttributeUnitComboBoxAction = nullptr;
  mAttributeValueEditAction    = nullptr;
  delete mAttributeUnitComboBox;
  mAttributeUnitComboBox = nullptr;
  delete mAttributeValueEdit;
  mAttributeValueEdit = nullptr;
  delete mValueComboBox;
  mValueComboBox = nullptr;
  delete mValueLabel;
  mValueLabel = nullptr;

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_AddComponent::processAddComponent() noexcept {
  try {
    // start adding (another) component
    if (!abortCommand(true)) return false;
    mLastAngle.setAngleMicroDeg(0);  // reset the angle
    startAddingComponent();
    return true;
  } catch (UserCanceled& exc) {
  } catch (Exception& exc) {
    QMessageBox::critical(parentWidget(), tr("Error"), exc.getMsg());
  }

  return false;
}

bool SchematicEditorState_AddComponent::processAddComponent(
    const Uuid& cmp, const Uuid& symbVar) noexcept {
  try {
    // start adding (another) component
    if (!abortCommand(true)) return false;
    mLastAngle.setAngleMicroDeg(0);  // reset the angle
    startAddingComponent(cmp, symbVar);
    return true;
  } catch (UserCanceled& exc) {
  } catch (Exception& exc) {
    QMessageBox::critical(parentWidget(), tr("Error"), exc.getMsg());
  }

  return false;
}

bool SchematicEditorState_AddComponent::processRotateCw() noexcept {
  mLastAngle -= Angle::deg90();
  mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
  return true;
}

bool SchematicEditorState_AddComponent::processRotateCcw() noexcept {
  mLastAngle += Angle::deg90();
  mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
  return true;
}

bool SchematicEditorState_AddComponent::processAbortCommand() noexcept {
  if (mAddComponentDialog) {
    try {
      if (!abortCommand(true)) return false;
      mLastAngle.setAngleMicroDeg(0);  // reset the angle
      startAddingComponent();
      return true;
    } catch (UserCanceled& exc) {
    } catch (Exception& exc) {
      QMessageBox::critical(parentWidget(), tr("Error"), exc.getMsg());
    }
  }

  return false;
}

bool SchematicEditorState_AddComponent::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mIsUndoCmdActive) {
    // set temporary position of the current symbol
    Q_ASSERT(mCurrentSymbolEditCommand);
    Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mCurrentSymbolEditCommand->setPosition(pos, true);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddComponent::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  // NOTE: This method is also called by the doubleclick event!
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;
  if (!mIsUndoCmdActive) return false;

  try {
    // place the current symbol finally
    Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mCurrentSymbolEditCommand->setPosition(pos, false);
    mContext.undoStack.appendToCmdGroup(mCurrentSymbolEditCommand);
    mCurrentSymbolEditCommand = nullptr;
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mContext.undoStack.beginCmdGroup(tr("Add Symbol to Schematic"));
    mIsUndoCmdActive = true;

    // check if there is a next symbol to add
    mCurrentSymbVarItemIndex++;
    const library::ComponentSymbolVariantItem* currentSymbVarItem =
        mCurrentComponent->getSymbolVariant()
            .getSymbolItems()
            .value(mCurrentSymbVarItemIndex)
            .get();

    if (currentSymbVarItem) {
      // create the next symbol instance and add it to the schematic
      CmdAddSymbolToSchematic* cmd = new CmdAddSymbolToSchematic(
          mContext.workspace, *schematic,
          mCurrentSymbolToPlace->getComponentInstance(),
          currentSymbVarItem->getUuid(), pos);
      mContext.undoStack.appendToCmdGroup(cmd);
      mCurrentSymbolToPlace = cmd->getSymbolInstance();
      Q_ASSERT(mCurrentSymbolToPlace);

      // add command to move the current symbol
      Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
      mCurrentSymbolEditCommand =
          new CmdSymbolInstanceEdit(*mCurrentSymbolToPlace);
      mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
    } else {
      // all symbols placed, start adding the next component
      Uuid componentUuid = mCurrentComponent->getLibComponent().getUuid();
      Uuid symbVarUuid   = mCurrentComponent->getSymbolVariant().getUuid();
      tl::optional<Uuid> defaultDeviceUuid =
          mCurrentComponent->getDefaultDeviceUuid();
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;
      abortCommand(false);  // reset attributes
      startAddingComponent(componentUuid, symbVarUuid, defaultDeviceUuid, true);
    }
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddComponent::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  // Handle the same way as single click
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_AddComponent::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  if ((mIsUndoCmdActive) &&
      (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton))) {
    // rotate symbol
    mLastAngle += Angle::deg90();
    mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
    return true;
  }

  return false;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicEditorState_AddComponent::startAddingComponent(
    const tl::optional<Uuid>& cmp, const tl::optional<Uuid>& symbVar,
    const tl::optional<Uuid>& dev, bool keepValue) {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) throw LogicError(__FILE__, __LINE__);

  try {
    // start a new command
    Q_ASSERT(!mIsUndoCmdActive);
    mContext.undoStack.beginCmdGroup(tr("Add Component to Schematic"));
    mIsUndoCmdActive = true;

    if (cmp && symbVar) {
      // add selected component to circuit
      auto* cmd = new CmdAddComponentToCircuit(
          mContext.workspace, mContext.project, *cmp, *symbVar, dev);
      mContext.undoStack.appendToCmdGroup(cmd);
      mCurrentComponent = cmd->getComponentInstance();
    } else {
      // show component chooser dialog
      if (!mAddComponentDialog)
        mAddComponentDialog.reset(new AddComponentDialog(
            mContext.workspace, mContext.project, parentWidget()));
      if (mAddComponentDialog->exec() != QDialog::Accepted)
        throw UserCanceled(__FILE__, __LINE__);  // abort
      if (!mAddComponentDialog->getSelectedComponentUuid())
        throw LogicError(__FILE__, __LINE__);
      if (!mAddComponentDialog->getSelectedSymbVarUuid())
        throw LogicError(__FILE__, __LINE__);

      // add selected component to circuit
      auto cmd = new CmdAddComponentToCircuit(
          mContext.workspace, mContext.project,
          *mAddComponentDialog->getSelectedComponentUuid(),
          *mAddComponentDialog->getSelectedSymbVarUuid(),
          mAddComponentDialog->getSelectedDeviceUuid());
      mContext.undoStack.appendToCmdGroup(cmd);
      mCurrentComponent = cmd->getComponentInstance();
    }

    // set value
    if (keepValue && mValueComboBox) {
      mCurrentComponent->setValue(toMultiLine(mValueComboBox->currentText()));
      attributeChanged();  // sets the attribute on the component
    } else if (mValueComboBox) {
      updateValueToolbar();
      updateAttributeToolbar();
    }

    // set focus to toolbar so the value can be changed by typing
    setFocusToToolbar();

    // create the first symbol instance and add it to the schematic
    mCurrentSymbVarItemIndex = 0;
    const library::ComponentSymbolVariantItem* currentSymbVarItem =
        mCurrentComponent->getSymbolVariant()
            .getSymbolItems()
            .value(mCurrentSymbVarItemIndex)
            .get();
    if (!currentSymbVarItem) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString(tr("The component with the UUID \"%1\" does "
                                    "not have any symbol."))
                             .arg(mCurrentComponent->getUuid().toStr()));
    }
    Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, true);
    CmdAddSymbolToSchematic* cmd2 = new CmdAddSymbolToSchematic(
        mContext.workspace, *schematic, *mCurrentComponent,
        currentSymbVarItem->getUuid(), pos);
    mContext.undoStack.appendToCmdGroup(cmd2);
    mCurrentSymbolToPlace = cmd2->getSymbolInstance();
    Q_ASSERT(mCurrentSymbolToPlace);

    // add command to move the current symbol
    Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
    mCurrentSymbolEditCommand =
        new CmdSymbolInstanceEdit(*mCurrentSymbolToPlace);
    mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
  } catch (Exception& e) {
    if (mIsUndoCmdActive) {
      try {
        mContext.undoStack.abortCmdGroup();
        mIsUndoCmdActive = false;
      } catch (...) {
      }
    }
    throw;
  }
}

bool SchematicEditorState_AddComponent::abortCommand(
    bool showErrMsgBox) noexcept {
  try {
    // delete the current move command
    delete mCurrentSymbolEditCommand;
    mCurrentSymbolEditCommand = nullptr;

    // abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // reset attributes, go back to idle state
    mCurrentComponent        = nullptr;
    mCurrentSymbVarItemIndex = -1;
    mCurrentSymbolToPlace    = nullptr;
    return true;
  } catch (Exception& e) {
    if (showErrMsgBox)
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

std::shared_ptr<const Attribute>
SchematicEditorState_AddComponent::getToolbarAttribute() const noexcept {
  if (mCurrentComponent) {
    QString value = mCurrentComponent->getValue();
    QString key   = QString(value).remove("{{").remove("}}").trimmed();
    auto    attr  = mCurrentComponent->getAttributes().find(key);
    if (attr && value.startsWith("{{") && value.endsWith("}}")) {
      return attr;
    }
  }
  return std::shared_ptr<Attribute>();
}

void SchematicEditorState_AddComponent::valueChanged(QString text) noexcept {
  if (!mCurrentComponent) return;

  mCurrentComponent->setValue(toMultiLine(text));
  updateAttributeToolbar();
}

void SchematicEditorState_AddComponent::attributeChanged() noexcept {
  if (!mCurrentComponent) return;

  std::shared_ptr<const Attribute> selected = getToolbarAttribute();
  if (!selected) return;
  AttributeList              attributes = mCurrentComponent->getAttributes();
  std::shared_ptr<Attribute> attribute  = attributes.find(*selected->getKey());
  if (!attribute) return;
  const AttributeType& type  = attribute->getType();
  QString              value = toMultiLine(mAttributeValueEdit->text());
  if (const AttributeUnit* unit = type.tryExtractUnitFromValue(value)) {
    // avoid recursion by blocking signals from combobox
    const bool wasBlocked = mAttributeUnitComboBox->blockSignals(true);
    mAttributeUnitComboBox->setCurrentItem(unit);
    mAttributeUnitComboBox->blockSignals(wasBlocked);
    mAttributeUnitComboBox->setEnabled(false);
  } else {
    mAttributeUnitComboBox->setEnabled(true);
  }
  const AttributeUnit* unit = mAttributeUnitComboBox->getCurrentItem();
  if (type.isValueValid(value) && type.isUnitAvailable(unit)) {
    attribute->setTypeValueUnit(attribute->getType(), value, unit);
    mCurrentComponent->setAttributes(attributes);
  }
}

void SchematicEditorState_AddComponent::updateValueToolbar() noexcept {
  if (!mCurrentComponent) return;

  mValueComboBox->blockSignals(true);
  mValueComboBox->clear();
  for (const Attribute& attribute : mCurrentComponent->getAttributes()) {
    mValueComboBox->addItem("{{" + *attribute.getKey() + "}}");
  }
  mValueComboBox->setCurrentText(toSingleLine(mCurrentComponent->getValue()));
  mValueComboBox->blockSignals(false);
}

void SchematicEditorState_AddComponent::updateAttributeToolbar() noexcept {
  if (!mCurrentComponent) return;

  std::shared_ptr<const Attribute> attribute = getToolbarAttribute();
  if (attribute) {
    mAttributeValueEdit->blockSignals(true);
    mAttributeUnitComboBox->blockSignals(true);
    mAttributeValueEdit->setText(toSingleLine(attribute->getValue()));
    mAttributeValueEdit->setPlaceholderText(*attribute->getKey());
    mAttributeValueEditAction->setVisible(true);
    mAttributeUnitComboBox->setAttributeType(attribute->getType());
    mAttributeUnitComboBox->setCurrentItem(attribute->getUnit());
    if (attribute->getType().getAvailableUnits().count() > 0) {
      mAttributeValueEdit->setMinimumWidth(50);
      mAttributeUnitComboBoxAction->setVisible(true);
    } else {
      mAttributeValueEdit->setMinimumWidth(200);
      mAttributeUnitComboBoxAction->setVisible(false);
    }
    mAttributeValueEdit->blockSignals(false);
    mAttributeUnitComboBox->blockSignals(false);
  } else {
    mAttributeValueEditAction->setVisible(false);
    mAttributeUnitComboBoxAction->setVisible(false);
  }
}

void SchematicEditorState_AddComponent::setFocusToToolbar() noexcept {
  QLineEdit* widget = nullptr;
  if (mAttributeValueEditAction && mAttributeValueEditAction->isVisible()) {
    widget = mAttributeValueEdit;
  } else if (mValueComboBox) {
    widget = mValueComboBox->lineEdit();
  }
  if (widget) {
    // Slightly delay it to make it working properly...
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    QTimer::singleShot(0, widget, &QLineEdit::selectAll);
    QTimer::singleShot(
        0, widget, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));
#else
    QTimer::singleShot(0, widget, SLOT(selectAll()));
    QTimer::singleShot(0, widget, SLOT(setFocus()));
#endif
  }
}

QString SchematicEditorState_AddComponent::toSingleLine(
    const QString& text) noexcept {
  return QString(text).replace("\n", "\\n");
}

QString SchematicEditorState_AddComponent::toMultiLine(
    const QString& text) noexcept {
  return text.trimmed().replace("\\n", "\n");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

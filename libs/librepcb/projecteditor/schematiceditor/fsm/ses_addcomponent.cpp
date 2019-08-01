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
#include "ses_addcomponent.h"

#include "../../cmd/cmdaddcomponenttocircuit.h"
#include "../../cmd/cmdaddsymboltoschematic.h"
#include "../../dialogs/addcomponentdialog.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

#include <librepcb/common/attributes/attributetype.h>
#include <librepcb/common/attributes/attributeunit.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/attributetypecombobox.h>
#include <librepcb/common/widgets/attributeunitcombobox.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SES_AddComponent::SES_AddComponent(SchematicEditor&     editor,
                                   Ui::SchematicEditor& editorUi,
                                   GraphicsView&        editorGraphicsView,
                                   UndoStack&           undoStack)
  : SES_Base(editor, editorUi, editorGraphicsView, undoStack),
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
    mAttributeUnitComboBoxAction(nullptr)

{
}

SES_AddComponent::~SES_AddComponent() {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SES_Base::ProcRetVal SES_AddComponent::process(SEE_Base* event) noexcept {
  switch (event->getType()) {
    case SEE_Base::AbortCommand: {
      if (mAddComponentDialog) {
        try {
          if (!abortCommand(true)) return PassToParentState;
          mLastAngle.setAngleMicroDeg(0);  // reset the angle
          startAddingComponent();
          return ForceStayInState;
        } catch (UserCanceled& exc) {
        } catch (Exception& exc) {
          QMessageBox::critical(&mEditor, tr("Error"), exc.getMsg());
        }
      }
      return PassToParentState;
    }
    case SEE_Base::StartAddComponent: {
      try {
        // start adding (another) component
        SEE_StartAddComponent* e = dynamic_cast<SEE_StartAddComponent*>(event);
        Q_ASSERT(e);
        if (!abortCommand(true)) return PassToParentState;
        mLastAngle.setAngleMicroDeg(0);  // reset the angle
        startAddingComponent(e->getComponentUuid(), e->getSymbVarUuid());
        return ForceStayInState;
      } catch (UserCanceled& exc) {
      } catch (Exception& exc) {
        QMessageBox::critical(&mEditor, tr("Error"), exc.getMsg());
      }
      return PassToParentState;
    }
    case SEE_Base::Edit_RotateCW:
      mLastAngle -= Angle::deg90();
      mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
      return ForceStayInState;
    case SEE_Base::Edit_RotateCCW:
      mLastAngle += Angle::deg90();
      mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
      return ForceStayInState;
    case SEE_Base::GraphicsViewEvent:
      return processSceneEvent(event);
    default:
      return PassToParentState;
  }
}

bool SES_AddComponent::entry(SEE_Base* event) noexcept {
  // only accept events of type SEE_StartAddComponent
  if (!event) return false;
  if (event->getType() != SEE_Base::StartAddComponent) return false;
  SEE_StartAddComponent* e = dynamic_cast<SEE_StartAddComponent*>(event);
  Q_ASSERT(e);
  if (!e) return false;
  Q_ASSERT(mIsUndoCmdActive == false);
  mLastAngle.setAngleMicroDeg(0);

  // start adding the specified component
  try {
    startAddingComponent(e->getComponentUuid(), e->getSymbVarUuid());
  } catch (UserCanceled& exc) {
    if (mIsUndoCmdActive) abortCommand(false);
    delete mAddComponentDialog;
    mAddComponentDialog = nullptr;
    return false;
  } catch (Exception& exc) {
    QMessageBox::critical(
        &mEditor, tr("Error"),
        QString(tr("Could not add component:\n\n%1")).arg(exc.getMsg()));
    if (mIsUndoCmdActive) abortCommand(false);
    delete mAddComponentDialog;
    mAddComponentDialog = nullptr;
    return false;
  }
  Q_ASSERT(mCurrentComponent);

  // add the "Value:" label to the toolbar
  mValueLabel = new QLabel(tr("Value:"));
  mValueLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mValueLabel);

  // add the value text edit to the toolbar
  mValueComboBox = new QComboBox();
  mValueComboBox->setEditable(true);
  mValueComboBox->setFixedHeight(QLineEdit().sizeHint().height());
  mValueComboBox->setMinimumWidth(200);
  mValueComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  mEditorUi.commandToolbar->addWidget(mValueComboBox);

  // add the attribute text edit to the toolbar
  mAttributeValueEdit = new QLineEdit();
  mAttributeValueEdit->setClearButtonEnabled(true);
  mAttributeValueEdit->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Fixed);
  mAttributeValueEditAction =
      mEditorUi.commandToolbar->addWidget(mAttributeValueEdit);

  // add the attribute unit combobox to the toolbar
  mAttributeUnitComboBox = new AttributeUnitComboBox();
  mAttributeUnitComboBox->setFixedHeight(QLineEdit().sizeHint().height());
  mAttributeUnitComboBoxAction =
      mEditorUi.commandToolbar->addWidget(mAttributeUnitComboBox);

  // Update attribute toolbar widgets and start watching for modifications
  updateValueToolbar();
  updateAttributeToolbar();
  setFocusToToolbar();
  connect(mValueComboBox, &QComboBox::currentTextChanged, this,
          &SES_AddComponent::valueChanged);
  connect(mAttributeValueEdit, &QLineEdit::textChanged, this,
          &SES_AddComponent::attributeChanged);
  connect(mAttributeUnitComboBox, &AttributeUnitComboBox::currentItemChanged,
          this, &SES_AddComponent::attributeChanged);

  return true;
}

bool SES_AddComponent::exit(SEE_Base* event) noexcept {
  Q_UNUSED(event);

  // Abort the currently active command
  if (!abortCommand(true)) return false;
  Q_ASSERT(mIsUndoCmdActive == false);

  // Delete the "Add Component" dialog
  delete mAddComponentDialog;
  mAddComponentDialog = nullptr;

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
 *  Private Methods
 ******************************************************************************/

SES_Base::ProcRetVal SES_AddComponent::processSceneEvent(
    SEE_Base* event) noexcept {
  QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Schematic* schematic = mEditor.getActiveSchematic();
  Q_ASSERT(schematic);
  if (!schematic) return PassToParentState;
  if (!mIsUndoCmdActive) return PassToParentState;  // temporary

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(mEditor.getGridProperties().getInterval());
      // set temporary position of the current symbol
      Q_ASSERT(mCurrentSymbolEditCommand);
      mCurrentSymbolEditCommand->setPosition(pos, true);
      break;
    }

    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(mEditor.getGridProperties().getInterval());
      switch (sceneEvent->button()) {
        case Qt::LeftButton: {
          try {
            // place the current symbol finally
            mCurrentSymbolEditCommand->setPosition(pos, false);
            mUndoStack.appendToCmdGroup(mCurrentSymbolEditCommand);
            mCurrentSymbolEditCommand = nullptr;
            mUndoStack.commitCmdGroup();
            mIsUndoCmdActive = false;
            mUndoStack.beginCmdGroup(tr("Add Symbol to Schematic"));
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
                  mWorkspace, *schematic,
                  mCurrentSymbolToPlace->getComponentInstance(),
                  currentSymbVarItem->getUuid(), pos);
              mUndoStack.appendToCmdGroup(cmd);
              mCurrentSymbolToPlace = cmd->getSymbolInstance();
              Q_ASSERT(mCurrentSymbolToPlace);

              // add command to move the current symbol
              Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
              mCurrentSymbolEditCommand =
                  new CmdSymbolInstanceEdit(*mCurrentSymbolToPlace);
              mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
              return ForceStayInState;
            } else {
              // all symbols placed, start adding the next component
              Uuid componentUuid =
                  mCurrentComponent->getLibComponent().getUuid();
              Uuid symbVarUuid =
                  mCurrentComponent->getSymbolVariant().getUuid();
              mUndoStack.commitCmdGroup();
              mIsUndoCmdActive = false;
              abortCommand(false);  // reset attributes
              startAddingComponent(componentUuid, symbVarUuid, true);
              return ForceStayInState;
            }
          } catch (Exception& e) {
            QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
            abortCommand(false);
            return ForceLeaveState;
          }
          break;
        }

        case Qt::RightButton:
          return ForceStayInState;

        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      switch (sceneEvent->button()) {
        case Qt::RightButton:
          if (sceneEvent->screenPos() ==
              sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
            // rotate symbol
            mLastAngle += Angle::deg90();
            mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
            return ForceStayInState;
          }
          break;

        default:
          break;
      }
      break;
    }

    default: {
      // Always accept graphics scene events, even if we do not react on some of
      // the events! This will give us the full control over the graphics scene.
      // Otherwise, the graphics scene can react on some events and disturb our
      // state machine. Only the wheel event is ignored because otherwise the
      // view will not allow to zoom with the mouse wheel.
      if (qevent->type() != QEvent::GraphicsSceneWheel)
        return ForceStayInState;
      else
        return PassToParentState;
    }
  }
  return PassToParentState;
}

void SES_AddComponent::startAddingComponent(const tl::optional<Uuid>& cmp,
                                            const tl::optional<Uuid>& symbVar,
                                            bool keepValue) {
  Schematic* schematic = mEditor.getActiveSchematic();
  Q_ASSERT(schematic);
  if (!schematic) throw LogicError(__FILE__, __LINE__);

  try {
    // start a new command
    Q_ASSERT(!mIsUndoCmdActive);
    mUndoStack.beginCmdGroup(tr("Add Component to Schematic"));
    mIsUndoCmdActive = true;

    if (cmp && symbVar) {
      // add selected component to circuit
      auto* cmd =
          new CmdAddComponentToCircuit(mWorkspace, mProject, *cmp, *symbVar);
      mUndoStack.appendToCmdGroup(cmd);
      mCurrentComponent = cmd->getComponentInstance();
    } else {
      // show component chooser dialog
      if (!mAddComponentDialog)
        mAddComponentDialog =
            new AddComponentDialog(mWorkspace, mProject, &mEditor);
      if (mAddComponentDialog->exec() != QDialog::Accepted)
        throw UserCanceled(__FILE__, __LINE__);  // abort
      if (!mAddComponentDialog->getSelectedComponentUuid())
        throw LogicError(__FILE__, __LINE__);
      if (!mAddComponentDialog->getSelectedSymbVarUuid())
        throw LogicError(__FILE__, __LINE__);

      // add selected component to circuit
      auto cmd = new CmdAddComponentToCircuit(
          mWorkspace, mProject,
          *mAddComponentDialog->getSelectedComponentUuid(),
          *mAddComponentDialog->getSelectedSymbVarUuid(),
          mAddComponentDialog->getSelectedDeviceUuid());
      mUndoStack.appendToCmdGroup(cmd);
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
    Point pos =
        mEditorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
    CmdAddSymbolToSchematic* cmd2 =
        new CmdAddSymbolToSchematic(mWorkspace, *schematic, *mCurrentComponent,
                                    currentSymbVarItem->getUuid(), pos);
    mUndoStack.appendToCmdGroup(cmd2);
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
        mUndoStack.abortCmdGroup();
        mIsUndoCmdActive = false;
      } catch (...) {
      }
    }
    throw;
  }
}

bool SES_AddComponent::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // delete the current move command
    delete mCurrentSymbolEditCommand;
    mCurrentSymbolEditCommand = nullptr;

    // abort the undo command
    if (mIsUndoCmdActive) {
      mUndoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // reset attributes, go back to idle state
    mCurrentComponent        = nullptr;
    mCurrentSymbVarItemIndex = -1;
    mCurrentSymbolToPlace    = nullptr;
    return true;
  } catch (Exception& e) {
    if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

std::shared_ptr<const Attribute> SES_AddComponent::getToolbarAttribute() const
    noexcept {
  Q_ASSERT(mCurrentComponent);
  QString value = mCurrentComponent->getValue();
  QString key   = QString(value).remove("{{").remove("}}").trimmed();
  auto    attr  = mCurrentComponent->getAttributes().find(key);
  if (attr && value.startsWith("{{") && value.endsWith("}}")) {
    return attr;
  } else {
    return std::shared_ptr<Attribute>();
  }
}

void SES_AddComponent::valueChanged(QString text) noexcept {
  Q_ASSERT(mCurrentComponent);
  mCurrentComponent->setValue(toMultiLine(text));
  updateAttributeToolbar();
}

void SES_AddComponent::attributeChanged() noexcept {
  Q_ASSERT(mCurrentComponent);
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

void SES_AddComponent::updateValueToolbar() noexcept {
  mValueComboBox->blockSignals(true);
  mValueComboBox->clear();
  for (const Attribute& attribute : mCurrentComponent->getAttributes()) {
    mValueComboBox->addItem("{{" + *attribute.getKey() + "}}");
  }
  mValueComboBox->setCurrentText(toSingleLine(mCurrentComponent->getValue()));
  mValueComboBox->blockSignals(false);
}

void SES_AddComponent::updateAttributeToolbar() noexcept {
  Q_ASSERT(mCurrentComponent);
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

void SES_AddComponent::setFocusToToolbar() noexcept {
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

QString SES_AddComponent::toSingleLine(const QString& text) noexcept {
  return QString(text).replace("\n", "\\n");
}

QString SES_AddComponent::toMultiLine(const QString& text) noexcept {
  return text.trimmed().replace("\\n", "\n");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

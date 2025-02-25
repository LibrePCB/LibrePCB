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

#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/attributeunitcombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../addcomponentdialog.h"
#include "../../cmd/cmdaddcomponenttocircuit.h"
#include "../../cmd/cmdaddsymboltoschematic.h"
#include "../../cmd/cmdsymbolinstanceeditall.h"
#include "../schematiceditor.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/dev/part.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
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

SchematicEditorState_AddComponent::SchematicEditorState_AddComponent(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mUseAddComponentDialog(true),
    mAddComponentDialog(nullptr),
    mCurrentAngle(0),
    mCurrentMirrored(false),
    mCurrentValue(),
    mCurrentValueSuggestions(),
    mCurrentComponent(nullptr),
    mCurrentSymbVarItemIndex(-1),
    mCurrentSymbolToPlace(nullptr),
    mCurrentSymbolEditCommand(nullptr) {
}

SchematicEditorState_AddComponent::
    ~SchematicEditorState_AddComponent() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddComponent::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmSetTool(SchematicEditorFsmAdapter::Tool::Component, this);
  mCurrentAngle = Angle::deg0();
  mCurrentMirrored = false;
  setCurrentValue(QString());

  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddComponent::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetTool(SchematicEditorFsmAdapter::Tool::None, this);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_AddComponent::processAddComponent(
    const QString& searchTerm) noexcept {
  try {
    // start adding (another) component
    if (!abortCommand(true)) return false;
    mCurrentAngle = Angle::deg0();
    mCurrentMirrored = false;
    setCurrentValue(QString());
    mUseAddComponentDialog = true;
    startAddingComponent(std::nullopt, std::nullopt, std::nullopt, searchTerm);
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
    mCurrentAngle = Angle::deg0();
    mCurrentMirrored = false;
    setCurrentValue(QString());
    mUseAddComponentDialog = false;
    startAddingComponent(cmp, symbVar, std::nullopt);
    return true;
  } catch (UserCanceled& exc) {
  } catch (Exception& exc) {
    QMessageBox::critical(parentWidget(), tr("Error"), exc.getMsg());
  }

  return false;
}

bool SchematicEditorState_AddComponent::processRotate(
    const Angle& rotation) noexcept {
  if (mIsUndoCmdActive && mCurrentSymbolToPlace && mCurrentSymbolEditCommand) {
    mCurrentSymbolEditCommand->rotate(
        rotation, mCurrentSymbolToPlace->getPosition(), true);
    mCurrentAngle = mCurrentSymbolToPlace->getRotation();
    return true;
  }

  return false;
}

bool SchematicEditorState_AddComponent::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mIsUndoCmdActive && mCurrentSymbolToPlace && mCurrentSymbolEditCommand) {
    mCurrentSymbolEditCommand->mirror(mCurrentSymbolToPlace->getPosition(),
                                      orientation, true);
    mCurrentAngle = mCurrentSymbolToPlace->getRotation();
    mCurrentMirrored = mCurrentSymbolToPlace->getMirrored();
    return true;
  }

  return false;
}

bool SchematicEditorState_AddComponent::processAbortCommand() noexcept {
  try {
    if (!abortCommand(true)) {
      return false;
    }
    if (mUseAddComponentDialog && mAddComponentDialog &&
        mAddComponentDialog->getAutoOpenAgain()) {
      mCurrentAngle = Angle::deg0();
      mCurrentMirrored = false;
      setCurrentValue(QString());
      startAddingComponent();
      return true;
    }
  } catch (UserCanceled& exc) {
  } catch (Exception& exc) {
    QMessageBox::critical(parentWidget(), tr("Error"), exc.getMsg());
  }

  return false;  // FSM will handle the event and exit this state.
}

bool SchematicEditorState_AddComponent::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mIsUndoCmdActive && mCurrentSymbolEditCommand) {
    // set temporary position of the current symbol
    const Point p = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mCurrentSymbolEditCommand->setPosition(p, true);
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
  if (!mCurrentComponent) return false;
  if (!mCurrentSymbolToPlace) return false;
  if (!mCurrentSymbolEditCommand) return false;

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
    const ComponentSymbolVariantItem* currentSymbVarItem =
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
          new CmdSymbolInstanceEditAll(*mCurrentSymbolToPlace);
      mCurrentSymbolEditCommand->setRotation(mCurrentAngle, true);
      mCurrentSymbolEditCommand->setMirrored(mCurrentMirrored, true);
    } else {
      // all symbols placed, start adding the next component
      Uuid componentUuid = mCurrentComponent->getLibComponent().getUuid();
      Uuid symbVarUuid = mCurrentComponent->getSymbolVariant().getUuid();
      ComponentAssemblyOptionList options =
          mCurrentComponent->getAssemblyOptions();
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;
      abortCommand(false);  // reset attributes
      startAddingComponent(componentUuid, symbVarUuid, options, QString(),
                           true);
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
  if (mIsUndoCmdActive && mCurrentSymbolEditCommand) {
    // Only rotate symbol if cursor was not moved during click
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      mCurrentSymbolEditCommand->rotate(
          Angle::deg90(), mCurrentSymbolToPlace->getPosition(), true);
      mCurrentAngle = mCurrentSymbolToPlace->getRotation();
    }

    // Always accept the event if we are placing a symbol! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

/*******************************************************************************
 *  UI Input
 ******************************************************************************/

void SchematicEditorState_AddComponent::setCurrentValue(
    const QString& value) noexcept {
  if (mCurrentComponent) {
    mCurrentComponent->setValue(value);
  }

  if (mCurrentValue != value) {
    mCurrentValue = value;
    emit currentValueChanged(mCurrentValue);
  }

  QStringList suggestions;
  if (mCurrentComponent) {
    for (const Attribute& attribute : mCurrentComponent->getAttributes()) {
      suggestions.append("{{" + *attribute.getKey() + "}}");
    }
  }
  if (mCurrentValueSuggestions != suggestions) {
    mCurrentValueSuggestions = suggestions;
    emit currentValueSuggestionsChanged(mCurrentValueSuggestions);
  }

  // updateAttributeToolbar();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicEditorState_AddComponent::startAddingComponent(
    const std::optional<Uuid>& cmp, const std::optional<Uuid>& symbVar,
    const std::optional<ComponentAssemblyOptionList>& options,
    const QString& searchTerm, bool keepValue) {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Schematic* schematic = getActiveSchematic();
  if (!schematic) return;

  try {
    // start a new command
    Q_ASSERT(!mIsUndoCmdActive);
    mContext.undoStack.beginCmdGroup(tr("Add Component to Schematic"));
    mIsUndoCmdActive = true;

    if (cmp && symbVar) {
      // add selected component to circuit
      auto* cmd = new CmdAddComponentToCircuit(
          mContext.workspace, mContext.project, *cmp, *symbVar, options);
      mContext.undoStack.appendToCmdGroup(cmd);
      mCurrentComponent = cmd->getComponentInstance();
    } else {
      // show component chooser dialog
      if (mAddComponentDialog) {
        mAddComponentDialog->setLocaleOrder(mContext.project.getLocaleOrder());
        mAddComponentDialog->setNormOrder(mContext.project.getNormOrder());
      } else {
        mAddComponentDialog.reset(new AddComponentDialog(
            mContext.workspace.getLibraryDb(), mContext.workspace.getSettings(),
            mContext.project.getLocaleOrder(), mContext.project.getNormOrder(),
            parentWidget()));
      }
      if (!searchTerm.isEmpty()) {
        mAddComponentDialog->selectComponentByKeyword(searchTerm);
      }
      if (mAddComponentDialog->exec() != QDialog::Accepted)
        throw UserCanceled(__FILE__, __LINE__);  // abort
      if (!mAddComponentDialog->getSelectedComponent())
        throw LogicError(__FILE__, __LINE__);
      if (!mAddComponentDialog->getSelectedSymbolVariant())
        throw LogicError(__FILE__, __LINE__);

      // Create assembly options.
      ComponentAssemblyOptionList assemblyOptions;
      if (auto libDev = mAddComponentDialog->getSelectedDevice()) {
        PartList parts;
        if (auto libPart = mAddComponentDialog->getSelectedPart()) {
          parts.append(std::make_shared<Part>(
              libPart->getMpn(), libPart->getManufacturer(),
              libPart->getAttributes() | libDev->getAttributes()));
        }
        QSet<Uuid> assemblyVariants;
        if (mAddComponentDialog->getSelectedPackageAssemblyType() !=
            Package::AssemblyType::None) {
          assemblyVariants =
              mContext.project.getCircuit().getAssemblyVariants().getUuidSet();
        }
        assemblyOptions.append(std::make_shared<ComponentAssemblyOption>(
            libDev->getUuid(), libDev->getAttributes(), assemblyVariants,
            parts));
      }

      // add selected component to circuit
      auto cmd = new CmdAddComponentToCircuit(
          mContext.workspace, mContext.project,
          mAddComponentDialog->getSelectedComponent()->getUuid(),
          mAddComponentDialog->getSelectedSymbolVariant()->getUuid(),
          assemblyOptions);
      mContext.undoStack.appendToCmdGroup(cmd);
      mCurrentComponent = cmd->getComponentInstance();
    }

    // set value
    if (keepValue) {
      mCurrentComponent->setValue(mCurrentValue);
      // attributeChanged();  // sets the attribute on the component
    } else {
      setCurrentValue(mCurrentComponent->getValue());
      // updateAttributeToolbar();
    }

    // create the first symbol instance and add it to the schematic
    mCurrentSymbVarItemIndex = 0;
    const ComponentSymbolVariantItem* currentSymbVarItem =

        mCurrentComponent->getSymbolVariant()
            .getSymbolItems()
            .value(mCurrentSymbVarItemIndex)
            .get();
    if (!currentSymbVarItem) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("The component with the UUID \"%1\" does "
                            "not have any symbol.")
                             .arg(mCurrentComponent->getUuid().toStr()));
    }
    Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos(), true, true);
    CmdAddSymbolToSchematic* cmd2 = new CmdAddSymbolToSchematic(
        mContext.workspace, *schematic, *mCurrentComponent,
        currentSymbVarItem->getUuid(), pos);
    mContext.undoStack.appendToCmdGroup(cmd2);
    mCurrentSymbolToPlace = cmd2->getSymbolInstance();
    Q_ASSERT(mCurrentSymbolToPlace);

    // add command to move the current symbol
    Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
    mCurrentSymbolEditCommand =
        new CmdSymbolInstanceEditAll(*mCurrentSymbolToPlace);
    mCurrentSymbolEditCommand->setRotation(mCurrentAngle, true);
    mCurrentSymbolEditCommand->setMirrored(mCurrentMirrored, true);
  } catch (const Exception& e) {
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
    mCurrentComponent = nullptr;
    mCurrentSymbVarItemIndex = -1;
    mCurrentSymbolToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox)
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

/*std::shared_ptr<const Attribute>
    SchematicEditorState_AddComponent::getToolbarAttribute() const noexcept {
  if (mCurrentComponent) {
    // Only take the first line into account to avoid the problem described at
    // https://github.com/LibrePCB-Libraries/LibrePCB_Base.lplib/pull/138.
    QString value = mCurrentComponent->getValue().split("\n").value(0);
    QString key = QString(value).remove("{{").remove("}}").trimmed();
    auto attr = mCurrentComponent->getAttributes().find(key);
    if (attr && value.startsWith("{{") && value.endsWith("}}")) {
      return attr;
    }
  }
  return std::shared_ptr<Attribute>();
}

void SchematicEditorState_AddComponent::attributeChanged() noexcept {
  if (!mCurrentComponent) return;

  std::shared_ptr<const Attribute> selected = getToolbarAttribute();
  if (!selected) return;
  AttributeList attributes = mCurrentComponent->getAttributes();
  std::shared_ptr<Attribute> attribute = attributes.find(*selected->getKey());
  if (!attribute) return;
  const AttributeType& type = attribute->getType();
  QString value = toMultiLine(mAttributeValueEdit->text());
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

QString SchematicEditorState_AddComponent::toSingleLine(
    const QString& text) noexcept {
  return QString(text).replace("\n", "\\n");
}

QString SchematicEditorState_AddComponent::toMultiLine(
    const QString& text) noexcept {
  return text.trimmed().replace("\\n", "\n");
}*/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

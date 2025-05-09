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
#include "../../addcomponentdialog.h"
#include "../../cmd/cmdaddcomponenttocircuit.h"
#include "../../cmd/cmdaddsymboltoschematic.h"
#include "../../cmd/cmdsymbolinstanceeditall.h"
#include "../schematicgraphicsscene.h"

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
#include <librepcb/core/project/schematic/schematic.h>
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
    mCurrentValueAttribute(),
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

  mCurrentAngle = Angle::deg0();
  mCurrentMirrored = false;
  setValue(QString());

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features(
      SchematicEditorFsmAdapter::Feature::Rotate |
      SchematicEditorFsmAdapter::Feature::Mirror));
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddComponent::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
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
    setValue(QString());
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
    setValue(QString());
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
      setValue(QString());
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
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mIsUndoCmdActive && mCurrentSymbolEditCommand) {
    // set temporary position of the current symbol
    const Point p = e.scenePos.mappedToGrid(getGridInterval());
    mCurrentSymbolEditCommand->setPosition(p, true);
    return true;
  }

  return false;
}

bool SchematicEditorState_AddComponent::
    processGraphicsSceneLeftMouseButtonPressed(
        const GraphicsSceneMouseEvent& e) noexcept {
  // NOTE: This method is also called by the doubleclick event!
  if (!mIsUndoCmdActive) return false;
  if (!mCurrentComponent) return false;
  if (!mCurrentSymbolToPlace) return false;
  if (!mCurrentSymbolEditCommand) return false;

  try {
    // place the current symbol finally
    const Point pos = e.scenePos.mappedToGrid(getGridInterval());
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
          mContext.workspace, mContext.schematic,
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
        const GraphicsSceneMouseEvent& e) noexcept {
  // Handle the same way as single click
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_AddComponent::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  if (mIsUndoCmdActive && mCurrentSymbolEditCommand) {
    mCurrentSymbolEditCommand->rotate(
        Angle::deg90(), mCurrentSymbolToPlace->getPosition(), true);
    mCurrentAngle = mCurrentSymbolToPlace->getRotation();

    // Always accept the event if we are placing a symbol! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void SchematicEditorState_AddComponent::setValue(
    const QString& value) noexcept {
  if (mCurrentValue != value) {
    mCurrentValue = value;
    emit valueChanged(mCurrentValue);
  }

  QStringList suggestions;
  if (mCurrentComponent) {
    for (const Attribute& attribute : mCurrentComponent->getAttributes()) {
      suggestions.append("{{" + *attribute.getKey() + "}}");
    }
  }
  if (mCurrentValueSuggestions != suggestions) {
    mCurrentValueSuggestions = suggestions;
    emit valueSuggestionsChanged(mCurrentValueSuggestions);
  }

  const std::optional<AttributeKey> oldAttrKey = getValueAttributeKey();
  const AttributeType* oldAttrType = getValueAttributeType();
  const std::optional<QString> oldAttrValue = getValueAttributeValue();
  const AttributeUnit* oldAttrUnit = getValueAttributeUnit();
  mCurrentValueAttribute = std::nullopt;
  if (mCurrentComponent) {
    // Only take the first line into account to avoid the problem described at
    // https://github.com/LibrePCB-Libraries/LibrePCB_Base.lplib/pull/138.
    const QString value = mCurrentValue.split("\n").value(0);
    const QString key = QString(value).remove("{{").remove("}}").trimmed();
    auto attr = mCurrentComponent->getAttributes().find(key);
    if (attr && value.startsWith("{{") && value.endsWith("}}")) {
      mCurrentValueAttribute = *attr;
    }
  }
  if (getValueAttributeKey() != oldAttrKey) {
    emit valueAttributeKeyChanged(getValueAttributeKey());
  }
  if (getValueAttributeType() != oldAttrType) {
    emit valueAttributeTypeChanged(getValueAttributeType());
  }
  if (getValueAttributeValue() != oldAttrValue) {
    emit valueAttributeValueChanged(getValueAttributeValue());
  }
  if (getValueAttributeUnit() != oldAttrUnit) {
    emit valueAttributeUnitChanged(getValueAttributeUnit());
  }

  applyValueAndAttributeToComponent();
}

void SchematicEditorState_AddComponent::setValueAttributeValue(
    const QString& value) noexcept {
  if (mCurrentValueAttribute && (mCurrentValueAttribute->getValue() != value) &&
      mCurrentValueAttribute->getType().isValueValid(value)) {
    mCurrentValueAttribute->setTypeValueUnit(mCurrentValueAttribute->getType(),
                                             value,
                                             mCurrentValueAttribute->getUnit());
    emit valueAttributeValueChanged(value);
  }

  applyValueAndAttributeToComponent();
}

void SchematicEditorState_AddComponent::setValueAttributeUnit(
    const AttributeUnit* unit) noexcept {
  if (mCurrentValueAttribute && (mCurrentValueAttribute->getUnit() != unit) &&
      mCurrentValueAttribute->getType().isUnitAvailable(unit)) {
    mCurrentValueAttribute->setTypeValueUnit(mCurrentValueAttribute->getType(),
                                             mCurrentValueAttribute->getValue(),
                                             unit);
    emit valueAttributeUnitChanged(unit);
  }

  applyValueAndAttributeToComponent();
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
      applyValueAndAttributeToComponent();
    } else {
      setValue(mCurrentComponent->getValue());
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
    const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                          .mappedToGrid(getGridInterval());
    CmdAddSymbolToSchematic* cmd2 = new CmdAddSymbolToSchematic(
        mContext.workspace, mContext.schematic, *mCurrentComponent,
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

    // If a schematic frame was added as the first symbol in the schematic,
    // place it at (0, 0) and exit this tool for convenience and to ensure
    // a consistent schematic coordinate system across all LibrePCB projects.
    const Component& libCmp = mCurrentComponent->getLibComponent();
    if ((mContext.schematic.getSymbols().count() == 1) &&
        libCmp.isSchematicOnly() &&
        (libCmp.getNames().getDefaultValue()->toLower().contains("frame")) &&
        (mCurrentComponent->getSymbolVariant().getSymbolItems().count() == 1)) {
      mCurrentSymbolEditCommand->setPosition(Point(0, 0), true);
      mCurrentSymbolEditCommand->setRotation(Angle::deg0(), true);
      mCurrentSymbolEditCommand->setMirrored(false, true);
      mContext.undoStack.appendToCmdGroup(mCurrentSymbolEditCommand);
      mCurrentSymbolEditCommand = nullptr;
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;
      abortCommand(false);  // reset attributes
      if (auto scene = getActiveSchematicScene()) {
        mAdapter.fsmZoomToSceneRect(scene->itemsBoundingRect());
      }
      emit requestLeavingState();
    }
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

void SchematicEditorState_AddComponent::
    applyValueAndAttributeToComponent() noexcept {
  if (mCurrentComponent) {
    mCurrentComponent->setValue(mCurrentValue);
    if (mCurrentValueAttribute) {
      AttributeList attrs = mCurrentComponent->getAttributes();
      if (auto attr = attrs.find(*mCurrentValueAttribute->getKey())) {
        *attr = *mCurrentValueAttribute;
        mCurrentComponent->setAttributes(attrs);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

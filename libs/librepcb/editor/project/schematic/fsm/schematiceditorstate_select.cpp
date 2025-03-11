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
#include "schematiceditorstate_select.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../dialogs/polygonpropertiesdialog.h"
#include "../../../dialogs/textpropertiesdialog.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../undostack.h"
#include "../../../utils/editortoolbox.h"
#include "../../../utils/menubuilder.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmddragselectedschematicitems.h"
#include "../../cmd/cmdpasteschematicitems.h"
#include "../../cmd/cmdremoveselectedschematicitems.h"
#include "../../projecteditor.h"
#include "../graphicsitems/sgi_netlabel.h"
#include "../graphicsitems/sgi_symbol.h"
#include "../graphicsitems/sgi_text.h"
#include "../renamenetsegmentdialog.h"
#include "../schematicclipboarddatabuilder.h"
#include "../schematiceditor.h"
#include "../schematicgraphicsscene.h"
#include "../schematicselectionquery.h"
#include "../symbolinstancepropertiesdialog.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>

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

SchematicEditorState_Select::SchematicEditorState_Select(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mSubState(SubState::IDLE),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

SchematicEditorState_Select::~SchematicEditorState_Select() noexcept {
  Q_ASSERT(!mSelectedItemsDragCommand);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_Select::entry() noexcept {
  Q_ASSERT(mSubState == SubState::IDLE);
  return true;
}

bool SchematicEditorState_Select::exit() noexcept {
  if (mSubState == SubState::PASTING) {
    try {
      mContext.undoStack.abortCmdGroup();
    } catch (...) {
      return false;
    }
  }

  mSelectedItemsDragCommand.reset();
  mCmdPolygonEdit.reset();
  mSubState = SubState::IDLE;

  // Avoid propagating the selection to other, non-selectable tools, thus
  // clearing the selection.
  if (SchematicGraphicsScene* scene = getActiveSchematicScene()) {
    scene->clearSelection();
  }

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_Select::processSelectAll() noexcept {
  if (mSubState == SubState::IDLE) {
    if (SchematicGraphicsScene* scene = getActiveSchematicScene()) {
      scene->selectAll();
      return true;
    }
  }
  return false;
}

bool SchematicEditorState_Select::processCut() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    return copySelectedItemsToClipboard() && removeSelectedItems();
  }
  return false;
}

bool SchematicEditorState_Select::processCopy() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    return copySelectedItemsToClipboard();
  }
  return false;
}

bool SchematicEditorState_Select::processPaste() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    return pasteFromClipboard();
  }
  return false;
}

bool SchematicEditorState_Select::processMove(const Point& delta) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    return moveSelectedItems(delta);
  }
  return false;
}

bool SchematicEditorState_Select::processRotate(
    const Angle& rotation) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (!mCmdPolygonEdit) {
    return rotateSelectedItems(rotation);
  }
  return false;
}

bool SchematicEditorState_Select::processMirror(
    Qt::Orientation orientation) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (!mCmdPolygonEdit) {
    return mirrorSelectedItems(orientation);
  }
  return false;
}

bool SchematicEditorState_Select::processSnapToGrid() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (!mCmdPolygonEdit) {
    return snapSelectedItemsToGrid();
  }
  return false;
}

bool SchematicEditorState_Select::processResetAllTexts() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    return resetAllTextsOfSelectedItems();
  }
  return false;
}

bool SchematicEditorState_Select::processRemove() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    removeSelectedItems();
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processEditProperties() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if ((!scene) || (mSubState != SubState::IDLE)) {
    return false;
  }

  SchematicSelectionQuery query(*scene);
  query.addSelectedSymbols();
  query.addSelectedNetLabels();
  query.addSelectedPolygons();
  query.addSelectedSchematicTexts();
  query.addSelectedSymbolTexts();
  foreach (auto ptr, query.getSymbols()) {
    openSymbolPropertiesDialog(*ptr);
    return true;
  }
  foreach (auto ptr, query.getNetLabels()) {
    openNetLabelPropertiesDialog(*ptr);
    return true;
  }
  foreach (auto ptr, query.getPolygons()) {
    openPolygonPropertiesDialog(ptr->getPolygon());
    return true;
  }
  foreach (auto ptr, query.getTexts()) {
    openTextPropertiesDialog(ptr->getTextObj());
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processAbortCommand() noexcept {
  try {
    switch (mSubState) {
      case SubState::IDLE: {
        if (SchematicGraphicsScene* scene = getActiveSchematicScene()) {
          scene->clearSelection();
        }
        return true;
      }
      case SubState::PASTING: {
        Q_ASSERT(mSelectedItemsDragCommand);
        mContext.undoStack.abortCmdGroup();
        mSelectedItemsDragCommand.reset();
        mSubState = SubState::IDLE;
        return true;
      }
      case SubState::MOVING_POLYGON_VERTICES: {
        mCmdPolygonEdit.reset();
        mSelectedPolygon = nullptr;
        mSelectedPolygonVertices.clear();
        mSubState = SubState::IDLE;
        return true;
      }
      default: {
        break;
      }
    }
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  switch (mSubState) {
    case SubState::SELECTING: {
      // Update selection rectangle.
      scene->selectItemsInRect(mStartPos, e.scenePos);
      return true;
    }

    case SubState::MOVING:
    case SubState::PASTING: {
      Q_ASSERT(mSelectedItemsDragCommand);
      mSelectedItemsDragCommand->setCurrentPosition(e.scenePos);
      return true;
    }

    case SubState::MOVING_POLYGON_VERTICES: {
      // Move polygon vertices
      QVector<Vertex> vertices =
          mSelectedPolygon->getPolygon().getPath().getVertices();
      foreach (int i, mSelectedPolygonVertices) {
        if ((i >= 0) && (i < vertices.count())) {
          vertices[i].setPos(e.scenePos.mappedToGrid(getGridInterval()));
        }
      }
      mCmdPolygonEdit->setPath(Path(vertices), true);
      return true;
    }

    default:
      break;
  }

  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  if (mSubState == SubState::IDLE) {
    if (findPolygonVerticesAtPosition(e.scenePos)) {
      // start moving polygon vertex
      mCmdPolygonEdit.reset(new CmdPolygonEdit(mSelectedPolygon->getPolygon()));
      mSubState = SubState::MOVING_POLYGON_VERTICES;
      return true;
    } else {
      // handle items selection
      const QList<std::shared_ptr<QGraphicsItem>> items =
          findItemsAtPos(e.scenePos, FindFlag::All | FindFlag::AcceptNearMatch);
      if (items.isEmpty()) {
        // no items under mouse --> start drawing a selection rectangle
        scene->clearSelection();
        mStartPos = e.scenePos;
        mSubState = SubState::SELECTING;
        return true;
      }

      // Check if there's already an item selected. If a symbol is selected,
      // make sure to ignore its texts because they have been be selected
      // automatically too.
      auto isTextOfSymbol = [](const std::shared_ptr<QGraphicsItem>& text,
                               const std::shared_ptr<QGraphicsItem>& symbol) {
        if (auto textItem = dynamic_cast<SGI_Text*>(text.get())) {
          return textItem->getSymbolGraphicsItem().lock() == symbol;
        }
        return false;
      };
      std::shared_ptr<QGraphicsItem> selectedItem;
      foreach (auto item, items) {
        if ((item->isSelected()) &&
            ((!selectedItem) || (!isTextOfSymbol(item, selectedItem)))) {
          selectedItem = item;
        }
      }
      if (e.modifiers & Qt::ControlModifier) {
        // Toggle selection when CTRL is pressed.
        auto item = selectedItem ? selectedItem : items.first();
        item->setSelected(!item->isSelected());
      } else if (e.modifiers & Qt::ShiftModifier) {
        // Cycle Selection, when holding shift.
        const int nextSelectionIndex =
            (items.indexOf(selectedItem) + 1) % items.count();
        Q_ASSERT((nextSelectionIndex >= 0) &&
                 (nextSelectionIndex < items.count()));
        scene->clearSelection();
        items[nextSelectionIndex]->setSelected(true);
      } else if (!selectedItem) {
        // Only select the topmost item when clicking an unselected item
        // without CTRL.
        scene->clearSelection();
        items.first()->setSelected(true);
      }

      if (startMovingSelectedItems(*scene, e.scenePos)) {
        return true;
      }
    }
  } else if (mSubState == SubState::PASTING) {
    // stop moving items (set position of all selected elements permanent)
    Q_ASSERT(mSelectedItemsDragCommand);
    mSelectedItemsDragCommand->setCurrentPosition(e.scenePos);
    try {
      mContext.undoStack.appendToCmdGroup(
          mSelectedItemsDragCommand.release());  // can throw
      mContext.undoStack.commitCmdGroup();
    } catch (Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedItemsDragCommand.reset();
    mSubState = SubState::IDLE;
  }

  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  if (mSubState == SubState::SELECTING) {
    // remove selection rectangle and keep the selection state of all items
    scene->clearSelectionRect();
    mSubState = SubState::IDLE;
    return true;
  } else if (mSubState == SubState::MOVING) {
    // stop moving items (set position of all selected elements permanent)
    Q_ASSERT(mSelectedItemsDragCommand);
    mSelectedItemsDragCommand->setCurrentPosition(e.scenePos);
    try {
      execCmd(mSelectedItemsDragCommand.release());  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedItemsDragCommand.reset();
    mSubState = SubState::IDLE;
  } else if (mSubState == SubState::MOVING_POLYGON_VERTICES) {
    // Stop moving polygon vertices
    try {
      mContext.undoStack.execCmd(mCmdPolygonEdit.release());
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedPolygon = nullptr;
    mSelectedPolygonVertices.clear();
    mSubState = SubState::IDLE;
  }

  return false;
}

bool SchematicEditorState_Select::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  // If SHIFT or CTRL is pressed, the user is modifying items selection, not
  // double-clicking.
  if (e.modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) {
    return processGraphicsSceneLeftMouseButtonPressed(e);
  }

  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSubState == SubState::IDLE) {
    // Open the properties editor dialog of the selected item, if any.
    const QList<std::shared_ptr<QGraphicsItem>> items =
        findItemsAtPos(e.scenePos, FindFlag::All | FindFlag::AcceptNearMatch);
    foreach (auto item, items) {
      if (item->isSelected() && openPropertiesDialog(item)) {
        return true;
      }
    }
  }

  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mSelectedItemsDragCommand) {
    return rotateSelectedItems(Angle::deg90());
  }

  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;
  if (mSubState != SubState::IDLE) return false;

  // handle item selection
  QList<std::shared_ptr<QGraphicsItem>> items =
      findItemsAtPos(e.scenePos, FindFlag::All | FindFlag::AcceptNearMatch);
  if (items.isEmpty()) return false;
  std::shared_ptr<QGraphicsItem> selectedItem;
  foreach (auto item, items) {
    if (item->isSelected()) {
      selectedItem = item;
    }
  }
  if (!selectedItem) {
    scene->clearSelection();
    selectedItem = items.first();
    selectedItem->setSelected(true);
  }
  Q_ASSERT(selectedItem);
  Q_ASSERT(selectedItem->isSelected());

  // build the context menu
  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  if (auto sym = std::dynamic_pointer_cast<SGI_Symbol>(selectedItem)) {
    mb.addAction(
        cmd.properties.createAction(
            &menu, this,
            [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
        MenuBuilder::Flag::DefaultAction);
    mb.addSeparator();
    mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
      copySelectedItemsToClipboard();
      removeSelectedItems();
    }));
    mb.addAction(cmd.clipboardCopy.createAction(
        &menu, this, [this]() { copySelectedItemsToClipboard(); }));
    mb.addAction(cmd.remove.createAction(&menu, this,
                                         [this]() { removeSelectedItems(); }));
    mb.addSeparator();
    mb.addAction(cmd.rotateCcw.createAction(
        &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
    mb.addAction(cmd.rotateCw.createAction(
        &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
    mb.addAction(cmd.mirrorHorizontal.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal); }));
    mb.addAction(cmd.mirrorVertical.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical); }));
    mb.addSeparator();
    QAction* aSnap = cmd.snapToGrid.createAction(
        &menu, this, [this]() { snapSelectedItemsToGrid(); });
    aSnap->setEnabled(
        !sym->getSymbol().getPosition().isOnGrid(getGridInterval()));
    mb.addAction(aSnap);
    mb.addAction(cmd.deviceResetTextAll.createAction(
        &menu, this,
        &SchematicEditorState_Select::resetAllTextsOfSelectedItems));
    EditorToolbox::addResourcesToMenu(mContext.workspace, mb,
                                      sym->getSymbol().getComponentInstance(),
                                      std::nullopt, &mContext.editor, menu);
  } else if (auto item =
                 std::dynamic_pointer_cast<SGI_NetLabel>(selectedItem)) {
    mb.addAction(
        cmd.properties.createAction(
            &menu, this,
            [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
        MenuBuilder::Flag::DefaultAction);
    mb.addSeparator();
    mb.addAction(cmd.remove.createAction(&menu, this,
                                         [this]() { removeSelectedItems(); }));
    mb.addSeparator();
    mb.addAction(cmd.rotateCcw.createAction(
        &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
    mb.addAction(cmd.rotateCw.createAction(
        &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
    mb.addAction(cmd.mirrorHorizontal.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal); }));
    mb.addAction(cmd.mirrorVertical.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical); }));
    QAction* aSnap = cmd.snapToGrid.createAction(
        &menu, this, [this]() { snapSelectedItemsToGrid(); });
    aSnap->setEnabled(
        !item->getNetLabel().getPosition().isOnGrid(getGridInterval()));
    mb.addAction(aSnap);
  } else if (auto item =
                 std::dynamic_pointer_cast<PolygonGraphicsItem>(selectedItem)) {
    SI_Polygon* polygon =
        scene->getSchematic().getPolygons().value(item->getObj().getUuid());
    if (!polygon) return false;

    int lineIndex = item->getLineIndexAtPosition(e.scenePos);
    QVector<int> vertices = item->getVertexIndicesAtPosition(e.scenePos);

    mb.addAction(
        cmd.properties.createAction(
            &menu, this,
            [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
        MenuBuilder::Flag::DefaultAction);
    mb.addSeparator();
    if (!vertices.isEmpty()) {
      QAction* action = cmd.vertexRemove.createAction(
          &menu, this, [this, polygon, vertices]() {
            removePolygonVertices(polygon->getPolygon(), vertices);
          });
      int remainingVertices =
          polygon->getPolygon().getPath().getVertices().count() -
          vertices.count();
      action->setEnabled(remainingVertices >= 2);
      mb.addAction(action);
    }
    if (lineIndex >= 0) {
      mb.addAction(cmd.vertexAdd.createAction(
          &menu, this, [this, polygon, lineIndex, e]() {
            startAddingPolygonVertex(*polygon, lineIndex, e.scenePos);
          }));
    }
    if ((lineIndex >= 0) || (!vertices.isEmpty())) {
      mb.addSeparator();
    }
    mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
      copySelectedItemsToClipboard();
      removeSelectedItems();
    }));
    mb.addAction(cmd.clipboardCopy.createAction(
        &menu, this, [this]() { copySelectedItemsToClipboard(); }));
    mb.addAction(cmd.remove.createAction(&menu, this,
                                         [this]() { removeSelectedItems(); }));
    mb.addSeparator();
    mb.addAction(cmd.rotateCcw.createAction(
        &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
    mb.addAction(cmd.rotateCw.createAction(
        &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
    mb.addAction(cmd.mirrorHorizontal.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal); }));
    mb.addAction(cmd.mirrorVertical.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical); }));
    QAction* aSnap = cmd.snapToGrid.createAction(
        &menu, this, [this]() { snapSelectedItemsToGrid(); });
    aSnap->setEnabled(
        !polygon->getPolygon().getPath().isOnGrid(getGridInterval()));
    mb.addAction(aSnap);
  } else if (auto item = std::dynamic_pointer_cast<SGI_Text>(selectedItem)) {
    mb.addAction(
        cmd.properties.createAction(
            &menu, this,
            [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
        MenuBuilder::Flag::DefaultAction);
    mb.addSeparator();
    mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
      copySelectedItemsToClipboard();
      removeSelectedItems();
    }));
    mb.addAction(cmd.clipboardCopy.createAction(
        &menu, this, [this]() { copySelectedItemsToClipboard(); }));
    mb.addAction(cmd.remove.createAction(&menu, this,
                                         [this]() { removeSelectedItems(); }));
    mb.addSeparator();
    mb.addAction(cmd.rotateCcw.createAction(
        &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
    mb.addAction(cmd.rotateCw.createAction(
        &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
    mb.addAction(cmd.mirrorHorizontal.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal); }));
    mb.addAction(cmd.mirrorVertical.createAction(
        &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical); }));
    QAction* aSnap = cmd.snapToGrid.createAction(
        &menu, this, [this]() { snapSelectedItemsToGrid(); });
    aSnap->setEnabled(
        !item->getText().getPosition().isOnGrid(getGridInterval()));
    mb.addAction(aSnap);
  } else {
    return false;
  }

  // execute the context menu
  menu.exec(QCursor::pos());
  return true;
}

bool SchematicEditorState_Select::processSwitchToSchematicPage(
    int index) noexcept {
  Q_UNUSED(index);
  return mSubState == SubState::IDLE;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_Select::startMovingSelectedItems(
    SchematicGraphicsScene& scene, const Point& startPos) noexcept {
  Q_ASSERT(!mSelectedItemsDragCommand);
  mSelectedItemsDragCommand.reset(
      new CmdDragSelectedSchematicItems(scene, startPos));
  mSubState = SubState::MOVING;
  return true;
}

bool SchematicEditorState_Select::moveSelectedItems(
    const Point& delta) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if ((!scene) || (mSelectedItemsDragCommand)) return false;

  try {
    std::unique_ptr<CmdDragSelectedSchematicItems> cmd(
        new CmdDragSelectedSchematicItems(*scene, Point(0, 0)));
    cmd->setCurrentPosition(delta);
    return execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->rotate(angle, true);
    } else {
      std::unique_ptr<CmdDragSelectedSchematicItems> cmd(
          new CmdDragSelectedSchematicItems(*scene));
      cmd->rotate(angle, false);
      execCmd(cmd.release());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::mirrorSelectedItems(
    Qt::Orientation orientation) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->mirror(orientation, true);
    } else {
      std::unique_ptr<CmdDragSelectedSchematicItems> cmd(
          new CmdDragSelectedSchematicItems(*scene));
      cmd->mirror(orientation, false);
      execCmd(cmd.release());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::snapSelectedItemsToGrid() noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->snapToGrid();
    } else {
      std::unique_ptr<CmdDragSelectedSchematicItems> cmd(
          new CmdDragSelectedSchematicItems(*scene));
      cmd->snapToGrid();
      execCmd(cmd.release());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::resetAllTextsOfSelectedItems() noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    std::unique_ptr<CmdDragSelectedSchematicItems> cmd(
        new CmdDragSelectedSchematicItems(*scene));
    cmd->resetAllTexts();
    mContext.undoStack.execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::removeSelectedItems() noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    CmdRemoveSelectedSchematicItems* cmd =
        new CmdRemoveSelectedSchematicItems(*scene);
    execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void SchematicEditorState_Select::removePolygonVertices(
    Polygon& polygon, const QVector<int> vertices) noexcept {
  try {
    Path path;
    for (int i = 0; i < polygon.getPath().getVertices().count(); ++i) {
      if (!vertices.contains(i)) {
        path.getVertices().append(polygon.getPath().getVertices()[i]);
      }
    }
    if (polygon.getPath().isClosed() && path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid polygons!
    }
    std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon));
    cmd->setPath(path, false);
    mContext.undoStack.execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void SchematicEditorState_Select::startAddingPolygonVertex(
    SI_Polygon& polygon, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = polygon.getPolygon().getPath();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));

    mSelectedPolygon = &polygon;
    mSelectedPolygonVertices = {vertex};
    mCmdPolygonEdit.reset(new CmdPolygonEdit(polygon.getPolygon()));
    mCmdPolygonEdit->setPath(path, true);
    mSubState = SubState::MOVING_POLYGON_VERTICES;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

bool SchematicEditorState_Select::copySelectedItemsToClipboard() noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    const Point cursorPos =
        mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos());
    SchematicClipboardDataBuilder builder(*scene);
    std::unique_ptr<SchematicClipboardData> data = builder.generate(cursorPos);
    qApp->clipboard()->setMimeData(data->toMimeData().release());
    emit statusBarMessageChanged(tr("Copied to clipboard!"), 2000);
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

bool SchematicEditorState_Select::pasteFromClipboard() noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) return false;

  try {
    // get symbol items and abort if there are no items
    std::unique_ptr<SchematicClipboardData> data =
        SchematicClipboardData::fromMimeData(
            qApp->clipboard()->mimeData());  // can throw
    if (!data) {
      return false;
    }

    // update cursor position
    mStartPos =
        mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos());

    // start undo command group
    scene->clearSelection();
    mContext.undoStack.beginCmdGroup(tr("Paste Schematic Elements"));
    mSubState = SubState::PASTING;

    // paste items from clipboard
    Point offset =
        (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
    std::unique_ptr<CmdPasteSchematicItems> cmd(
        new CmdPasteSchematicItems(*scene, std::move(data), offset));

    if (mContext.undoStack.appendToCmdGroup(cmd.release())) {  // can throw
      // start moving the selected items
      mSelectedItemsDragCommand.reset(
          new CmdDragSelectedSchematicItems(*scene, mStartPos));
      return true;
    } else {
      // no items pasted -> abort
      mContext.undoStack.abortCmdGroup();  // can throw
      mSubState = SubState::IDLE;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mSelectedItemsDragCommand.reset();
    if (mSubState == SubState::PASTING) {
      try {
        mContext.undoStack.abortCmdGroup();
      } catch (...) {
      }
      mSubState = SubState::IDLE;
    }
  }
  return false;
}

bool SchematicEditorState_Select::findPolygonVerticesAtPosition(
    const Point& pos) noexcept {
  if (SchematicGraphicsScene* scene = getActiveSchematicScene()) {
    for (auto it = scene->getPolygons().begin();
         it != scene->getPolygons().end(); it++) {
      if (it.value()->isSelected()) {
        mSelectedPolygonVertices = it.value()->getVertexIndicesAtPosition(pos);
        if (!mSelectedPolygonVertices.isEmpty()) {
          mSelectedPolygon = it.key();
          return true;
        }
      }
    }
  }

  mSelectedPolygon = nullptr;
  mSelectedPolygonVertices.clear();
  return false;
}

bool SchematicEditorState_Select::openPropertiesDialog(
    std::shared_ptr<QGraphicsItem> item) noexcept {
  if (auto symbol = std::dynamic_pointer_cast<SGI_Symbol>(item)) {
    openSymbolPropertiesDialog(symbol->getSymbol());
    return true;
  } else if (auto netLabel = std::dynamic_pointer_cast<SGI_NetLabel>(item)) {
    openNetLabelPropertiesDialog(netLabel->getNetLabel());
    return true;
  } else if (auto polygon =
                 std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    openPolygonPropertiesDialog(polygon->getObj());
    return true;
  } else if (auto text = std::dynamic_pointer_cast<SGI_Text>(item)) {
    openTextPropertiesDialog(text->getText().getTextObj());
    return true;
  }
  return false;
}

void SchematicEditorState_Select::openSymbolPropertiesDialog(
    SI_Symbol& symbol) noexcept {
  SymbolInstancePropertiesDialog dialog(
      mContext.workspace, mContext.project, symbol.getComponentInstance(),
      symbol, mContext.undoStack, getLengthUnit(),
      "schematic_editor/symbol_properties_dialog", parentWidget());
  dialog.exec();
}

void SchematicEditorState_Select::openNetLabelPropertiesDialog(
    SI_NetLabel& netlabel) noexcept {
  RenameNetSegmentDialog dialog(mContext.undoStack, netlabel.getNetSegment(),
                                parentWidget());
  dialog.exec();  // performs the rename, if needed
}

void SchematicEditorState_Select::openPolygonPropertiesDialog(
    Polygon& polygon) noexcept {
  PolygonPropertiesDialog dialog(
      polygon, mContext.undoStack, getAllowedGeometryLayers(), getLengthUnit(),
      "schematic_editor/polygon_properties_dialog", parentWidget());
  dialog.exec();
}

void SchematicEditorState_Select::openTextPropertiesDialog(
    Text& text) noexcept {
  TextPropertiesDialog dialog(
      text, mContext.undoStack, getAllowedGeometryLayers(), getLengthUnit(),
      "schematic_editor/text_properties_dialog", parentWidget());
  dialog.exec();  // performs the modifications
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

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

#include "../../cmd/cmdmirrorselectedschematicitems.h"
#include "../../cmd/cmdmoveselectedschematicitems.h"
#include "../../cmd/cmdpasteschematicitems.h"
#include "../../cmd/cmdremoveselectedschematicitems.h"
#include "../../cmd/cmdrotateselectedschematicitems.h"
#include "../renamenetsegmentdialog.h"
#include "../schematicclipboarddatabuilder.h"
#include "../symbolinstancepropertiesdialog.h"

#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/textpropertiesdialog.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_polygon.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_text.h>
#include <librepcb/project/schematics/schematiclayerprovider.h>

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

SchematicEditorState_Select::SchematicEditorState_Select(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mSubState(SubState::IDLE),
    mCurrentSelectionIndex(0),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

SchematicEditorState_Select::~SchematicEditorState_Select() noexcept {
  Q_ASSERT(mSelectedItemsMoveCommand.isNull());
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

  mSelectedItemsMoveCommand.reset();
  mCmdPolygonEdit.reset();
  mSubState = SubState::IDLE;
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_Select::processSelectAll() noexcept {
  if (mSubState == SubState::IDLE) {
    if (Schematic* schematic = getActiveSchematic()) {
      schematic->selectAll();
      return true;
    }
  }
  return false;
}

bool SchematicEditorState_Select::processCut() noexcept {
  if (mSubState == SubState::IDLE) {
    return copySelectedItemsToClipboard() && removeSelectedItems();
  }
  return false;
}

bool SchematicEditorState_Select::processCopy() noexcept {
  if (mSubState == SubState::IDLE) {
    return copySelectedItemsToClipboard();
  }
  return false;
}

bool SchematicEditorState_Select::processPaste() noexcept {
  if (mSubState == SubState::IDLE) {
    return pasteFromClipboard();
  }
  return false;
}

bool SchematicEditorState_Select::processRotateCw() noexcept {
  if (mSubState == SubState::IDLE) {
    rotateSelectedItems(-Angle::deg90());
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processRotateCcw() noexcept {
  if (mSubState == SubState::IDLE) {
    rotateSelectedItems(Angle::deg90());
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processMirror() noexcept {
  if (mSubState == SubState::IDLE) {
    mirrorSelectedItems();
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processRemove() noexcept {
  if (mSubState == SubState::IDLE) {
    removeSelectedItems();
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processAbortCommand() noexcept {
  if (mSubState == SubState::PASTING) {
    // abort pasting items
    Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
    try {
      mContext.undoStack.abortCmdGroup();
      mSelectedItemsMoveCommand.reset();
      mSubState = SubState::IDLE;
    } catch (Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return true;
  }
  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  switch (mSubState) {
    case SubState::SELECTING: {
      // update selection rectangle
      Point pos = Point::fromPx(e.scenePos());
      schematic->setSelectionRect(mStartPos, pos, true);
      return true;
    }

    case SubState::MOVING:
    case SubState::PASTING: {
      Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
      Point pos = Point::fromPx(e.scenePos());
      mSelectedItemsMoveCommand->setCurrentPosition(pos);
      return true;
    }

    case SubState::MOVING_POLYGON_VERTICES: {
      // Move polygon vertices
      QVector<Vertex> vertices =
          mSelectedPolygon->getPolygon().getPath().getVertices();
      foreach (int i, mSelectedPolygonVertices) {
        if ((i >= 0) && (i < vertices.count())) {
          vertices[i].setPos(
              Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval()));
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
    QGraphicsSceneMouseEvent& mouseEvent) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  if (mSubState == SubState::IDLE) {
    if (findPolygonVerticesAtPosition(Point::fromPx(mouseEvent.scenePos()))) {
      // start moving polygon vertex
      mCmdPolygonEdit.reset(new CmdPolygonEdit(mSelectedPolygon->getPolygon()));
      mSubState = SubState::MOVING_POLYGON_VERTICES;
      return true;
    } else {
      // handle items selection
      Point pos = Point::fromPx(mouseEvent.scenePos());
      QList<SI_Base*> items = schematic->getItemsAtScenePos(pos);
      if (items.isEmpty()) {
        // no items under mouse --> start drawing a selection rectangle
        schematic->clearSelection();
        mStartPos = pos;
        mSubState = SubState::SELECTING;
        return true;
      }

      bool itemAlreadySelected = items.first()->isSelected();

      if (mouseEvent.modifiers() & Qt::ControlModifier) {
        // Toggle selection when CTRL is pressed
        items.first()->setSelected(!itemAlreadySelected);
      } else if (mouseEvent.modifiers() & Qt::ShiftModifier) {
        // Cycle Selection, when holding shift
        mCurrentSelectionIndex += 1;
        mCurrentSelectionIndex %= items.count();
        schematic->clearSelection();
        items[mCurrentSelectionIndex]->setSelected(true);
      } else if (!itemAlreadySelected) {
        // Only select the topmost item when clicking an unselected item
        // without CTRL
        schematic->clearSelection();
        items.first()->setSelected(true);
      }

      if (startMovingSelectedItems(*schematic,
                                   Point::fromPx(mouseEvent.scenePos()))) {
        return true;
      }
    }
  } else if (mSubState == SubState::PASTING) {
    // stop moving items (set position of all selected elements permanent)
    Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
    Point pos = Point::fromPx(mouseEvent.scenePos());
    mSelectedItemsMoveCommand->setCurrentPosition(pos);
    try {
      mContext.undoStack.appendToCmdGroup(
          mSelectedItemsMoveCommand.take());  // can throw
      mContext.undoStack.commitCmdGroup();
    } catch (Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedItemsMoveCommand.reset();
    mSubState = SubState::IDLE;
  }

  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  if (mSubState == SubState::SELECTING) {
    // remove selection rectangle and keep the selection state of all items
    schematic->setSelectionRect(Point(), Point(), false);
    mSubState = SubState::IDLE;
    return true;
  } else if (mSubState == SubState::MOVING) {
    // stop moving items (set position of all selected elements permanent)
    Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
    Point pos = Point::fromPx(e.scenePos());
    mSelectedItemsMoveCommand->setCurrentPosition(pos);
    try {
      execCmd(mSelectedItemsMoveCommand.take());  // can throw
    } catch (Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedItemsMoveCommand.reset();
    mSubState = SubState::IDLE;
  } else if (mSubState == SubState::MOVING_POLYGON_VERTICES) {
    // Stop moving polygon vertices
    try {
      mContext.undoStack.execCmd(mCmdPolygonEdit.take());
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
        QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  // if in moving state, abort moving and handle double click (only needed
  // on older qt versions)
  if ((mSubState == SubState::MOVING) &&
      (QT_VERSION < QT_VERSION_CHECK(5, 3, 0))) {
    mSelectedItemsMoveCommand.reset();
    mSubState = SubState::IDLE;
  }

  if (mSubState == SubState::IDLE) {
    // check if there is an element under the mouse
    QList<SI_Base*> items =
        schematic->getItemsAtScenePos(Point::fromPx(e.scenePos()));
    if (items.isEmpty()) return false;

    // open the properties editor dialog of the top most item
    openPropertiesDialog(items.first());
  }

  return false;
}

bool SchematicEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;
  if (mSubState != SubState::IDLE) return false;

  QMenu menu;
  Point pos = Point::fromPx(e.scenePos());
  if (findPolygonVerticesAtPosition(pos)) {
    // special menu for polygon vertices
    addActionRemoveVertex(menu, *mSelectedPolygon, mSelectedPolygonVertices);
  } else {
    // handle item selection
    QList<SI_Base*> items =
        schematic->getItemsAtScenePos(Point::fromPx(e.scenePos()));
    if (items.isEmpty()) return false;
    SI_Base* selectedItem = nullptr;
    foreach (SI_Base* item, items) {
      if (item->isSelected()) {
        selectedItem = item;
      }
    }
    if (!selectedItem) {
      schematic->clearSelection();
      selectedItem = items.first();
      selectedItem->setSelected(true);
    }
    Q_ASSERT(selectedItem);
    Q_ASSERT(selectedItem->isSelected());

    // build the context menu
    switch (selectedItem->getType()) {
      case SI_Base::Type_t::Symbol: {
        SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(selectedItem);
        Q_ASSERT(symbol);

        addActionCut(menu);
        addActionCopy(menu);
        addActionRemove(menu, tr("Remove Symbol"));
        menu.addSeparator();
        addActionRotate(menu);
        addActionMirror(menu);
        menu.addSeparator();
        addActionOpenProperties(menu, selectedItem);
        break;
      }

      case SI_Base::Type_t::NetLabel: {
        SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(selectedItem);
        Q_ASSERT(netlabel);

        addActionRotate(menu);
        addActionRemove(menu, tr("Remove Net Label"));
        menu.addSeparator();
        addActionOpenProperties(menu, selectedItem, tr("Rename Net Segment"));
        break;
      }

      case SI_Base::Type_t::Polygon: {
        SI_Polygon* polygon = dynamic_cast<SI_Polygon*>(selectedItem);
        Q_ASSERT(polygon);

        if (addActionAddVertex(menu, *polygon, pos)) {
          menu.addSeparator();
        }
        addActionRotate(menu);
        addActionMirror(menu);
        addActionRemove(menu);
        menu.addSeparator();
        addActionOpenProperties(menu, selectedItem);
        break;
      }

      case SI_Base::Type_t::Text: {
        SI_Text* text = dynamic_cast<SI_Text*>(selectedItem);
        Q_ASSERT(text);

        addActionCut(menu);
        addActionCopy(menu);
        addActionRemove(menu);
        menu.addSeparator();
        addActionRotate(menu);
        addActionMirror(menu);
        menu.addSeparator();
        addActionOpenProperties(menu, selectedItem);
        break;
      }

      default:
        return false;
    }
  }

  // execute the context menu
  menu.exec(e.screenPos());
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
    Schematic& schematic, const Point& startPos) noexcept {
  Q_ASSERT(mSelectedItemsMoveCommand.isNull());
  mSelectedItemsMoveCommand.reset(
      new CmdMoveSelectedSchematicItems(schematic, startPos));
  mSubState = SubState::MOVING;
  return true;
}

bool SchematicEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  Schematic* schematic = getActiveSchematic();
  Q_ASSERT(schematic);
  if (!schematic) return false;

  try {
    CmdRotateSelectedSchematicItems* cmd =
        new CmdRotateSelectedSchematicItems(*schematic, angle);
    execCmd(cmd);
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::mirrorSelectedItems() noexcept {
  Schematic* schematic = getActiveSchematic();
  Q_ASSERT(schematic);
  if (!schematic) return false;

  try {
    CmdMirrorSelectedSchematicItems* cmd =
        new CmdMirrorSelectedSchematicItems(*schematic, Qt::Horizontal);
    execCmd(cmd);
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SchematicEditorState_Select::removeSelectedItems() noexcept {
  Schematic* schematic = getActiveSchematic();
  Q_ASSERT(schematic);
  if (!schematic) return false;

  try {
    CmdRemoveSelectedSchematicItems* cmd =
        new CmdRemoveSelectedSchematicItems(*schematic);
    execCmd(cmd);
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void SchematicEditorState_Select::removeSelectedPolygonVertices() noexcept {
  if ((!getActiveSchematic()) || (!mSelectedPolygon) ||
      mSelectedPolygonVertices.isEmpty()) {
    return;
  }

  try {
    Path path;
    Polygon& polygon = mSelectedPolygon->getPolygon();
    for (int i = 0; i < polygon.getPath().getVertices().count(); ++i) {
      if (!mSelectedPolygonVertices.contains(i)) {
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
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon));
    cmd->setPath(path, false);
    mContext.undoStack.execCmd(cmd.take());
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
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  try {
    Point cursorPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);
    SchematicClipboardDataBuilder builder(*schematic);
    std::unique_ptr<SchematicClipboardData> data = builder.generate(cursorPos);
    qApp->clipboard()->setMimeData(data->toMimeData().release());
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

bool SchematicEditorState_Select::pasteFromClipboard() noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  try {
    // get symbol items and abort if there are no items
    std::unique_ptr<SchematicClipboardData> data =
        SchematicClipboardData::fromMimeData(
            qApp->clipboard()->mimeData());  // can throw
    if (!data) {
      return false;
    }

    // update cursor position
    mStartPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);

    // start undo command group
    schematic->clearSelection();
    mContext.undoStack.beginCmdGroup(tr("Paste Schematic Elements"));
    mSubState = SubState::PASTING;

    // paste items from clipboard
    Point offset =
        (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
    QScopedPointer<CmdPasteSchematicItems> cmd(
        new CmdPasteSchematicItems(*schematic, std::move(data), offset));

    if (mContext.undoStack.appendToCmdGroup(cmd.take())) {  // can throw
      // start moving the selected items
      mSelectedItemsMoveCommand.reset(
          new CmdMoveSelectedSchematicItems(*schematic, mStartPos));
      return true;
    } else {
      // no items pasted -> abort
      mContext.undoStack.abortCmdGroup();  // can throw
      mSubState = SubState::IDLE;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mSelectedItemsMoveCommand.reset();
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
  if (Schematic* schematic = getActiveSchematic()) {
    foreach (SI_Polygon* polygon, schematic->getPolygons()) {
      if (polygon->isSelected()) {
        mSelectedPolygonVertices =
            polygon->getGraphicsItem().getVertexIndicesAtPosition(pos);
        if (!mSelectedPolygonVertices.isEmpty()) {
          mSelectedPolygon = polygon;
          return true;
        }
      }
    }
  }

  mSelectedPolygon = nullptr;
  mSelectedPolygonVertices.clear();
  return false;
}

void SchematicEditorState_Select::openPropertiesDialog(SI_Base* item) noexcept {
  if (!item) return;
  switch (item->getType()) {
    case SI_Base::Type_t::Symbol: {
      SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item);
      Q_ASSERT(symbol);
      openSymbolPropertiesDialog(*symbol);
      break;
    }

    case SI_Base::Type_t::NetLabel: {
      SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(item);
      Q_ASSERT(netlabel);
      openNetLabelPropertiesDialog(*netlabel);
      break;
    }

    case SI_Base::Type_t::Polygon: {
      SI_Polygon* polygon = dynamic_cast<SI_Polygon*>(item);
      Q_ASSERT(polygon);
      openPolygonPropertiesDialog(*polygon);
      break;
    }

    case SI_Base::Type_t::Text: {
      SI_Text* text = dynamic_cast<SI_Text*>(item);
      Q_ASSERT(text);
      openTextPropertiesDialog(*text);
      break;
    }

    default:
      break;
  }
}

void SchematicEditorState_Select::openSymbolPropertiesDialog(
    SI_Symbol& symbol) noexcept {
  SymbolInstancePropertiesDialog dialog(
      mContext.workspace, mContext.project, symbol.getComponentInstance(),
      symbol, mContext.undoStack, getDefaultLengthUnit(),
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
    SI_Polygon& polygon) noexcept {
  PolygonPropertiesDialog dialog(
      polygon.getPolygon(), mContext.undoStack,
      mContext.project.getLayers().getSchematicGeometryElementLayers(),
      getDefaultLengthUnit(), "schematic_editor/polygon_properties_dialog",
      parentWidget());
  dialog.exec();
}

void SchematicEditorState_Select::openTextPropertiesDialog(
    SI_Text& text) noexcept {
  TextPropertiesDialog dialog(
      text.getText(), mContext.undoStack,
      mContext.project.getLayers().getSchematicGeometryElementLayers(),
      getDefaultLengthUnit(), "schematic_editor/text_properties_dialog",
      parentWidget());
  dialog.exec();  // performs the modifications
}

QAction* SchematicEditorState_Select::addActionCut(
    QMenu& menu, const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/cut.png"), text);
  connect(action, &QAction::triggered, [this]() {
    copySelectedItemsToClipboard();
    removeSelectedItems();
  });
  return action;
}

QAction* SchematicEditorState_Select::addActionCopy(
    QMenu& menu, const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/copy.png"), text);
  connect(action, &QAction::triggered,
          [this]() { copySelectedItemsToClipboard(); });
  return action;
}

QAction* SchematicEditorState_Select::addActionRemove(
    QMenu& menu, const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/delete.png"), text);
  connect(action, &QAction::triggered, [this]() { removeSelectedItems(); });
  return action;
}

QAction* SchematicEditorState_Select::addActionMirror(
    QMenu& menu, const QString& text) noexcept {
  QAction* action =
      menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), text);
  connect(action, &QAction::triggered, [this]() { mirrorSelectedItems(); });
  return action;
}

QAction* SchematicEditorState_Select::addActionRotate(
    QMenu& menu, const QString& text) noexcept {
  QAction* action =
      menu.addAction(QIcon(":/img/actions/rotate_left.png"), text);
  connect(action, &QAction::triggered,
          [this]() { rotateSelectedItems(Angle::deg90()); });
  return action;
}

void SchematicEditorState_Select::addActionRemoveVertex(
    QMenu& menu, SI_Polygon& polygon, const QVector<int>& verticesToRemove,
    const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/delete.png"), text);
  connect(action, &QAction::triggered,
          [this]() { removeSelectedPolygonVertices(); });
  int remainingVertices = polygon.getPolygon().getPath().getVertices().count() -
      verticesToRemove.count();
  action->setEnabled(remainingVertices >= 2);
}

bool SchematicEditorState_Select::addActionAddVertex(
    QMenu& menu, SI_Polygon& polygon, const Point& pos,
    const QString& text) noexcept {
  int index = polygon.getGraphicsItem().getLineIndexAtPosition(pos);

  if (index >= 0) {
    QAction* action = menu.addAction(QIcon(":/img/actions/add.png"), text);
    connect(action, &QAction::triggered,
            [=, &polygon]() { startAddingPolygonVertex(polygon, index, pos); });
    return true;
  }

  return false;
}

QAction* SchematicEditorState_Select::addActionOpenProperties(
    QMenu& menu, SI_Base* item, const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/settings.png"), text);
  connect(action, &QAction::triggered,
          [this, item]() { openPropertiesDialog(item); });
  return action;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

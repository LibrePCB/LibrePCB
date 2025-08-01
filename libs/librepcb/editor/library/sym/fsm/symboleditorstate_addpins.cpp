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
#include "symboleditorstate_addpins.h"

#include "../../../dialogs/circuitidentifierimportdialog.h"
#include "../../../undostack.h"
#include "../../cmd/cmdsymbolpinedit.h"
#include "../symbolclipboarddata.h"
#include "../symbolgraphicsitem.h"
#include "../symbolpingraphicsitem.h"

#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_AddPins::SymbolEditorState_AddPins(
    const Context& context) noexcept
  : SymbolEditorState(context),
    mCurrentProperties(
        Uuid::createRandom(),  // Not relevant
        CircuitIdentifier("1"),  // Name
        Point(),  // Not relevant
        UnsignedLength(
            2540000),  // Default length according library conventions
        Angle::deg0(),  // Default rotation
        Point(),  // Will be set later
        Angle::deg0(),  // Default name rotation
        SymbolPin::getDefaultNameHeight(),
        SymbolPin::getDefaultNameAlignment()),
    mCurrentPin(nullptr),
    mCurrentGraphicsItem(nullptr) {
  mCurrentProperties.setNamePosition(
      SymbolPin::getDefaultNamePosition(mCurrentProperties.getLength()));
}

SymbolEditorState_AddPins::~SymbolEditorState_AddPins() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::entry() noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!addNextPin(pos)) {
    return false;
  }

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Feature::Rotate |
                          SymbolEditorFsmAdapter::Feature::Mirror);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_AddPins::exit() noexcept {
  // abort command
  try {
    mCurrentEditCmd.reset();
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    mContext.undoStack.abortCmdGroup();
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void SymbolEditorState_AddPins::setName(
    const CircuitIdentifier& name) noexcept {
  if (mCurrentProperties.setName(name)) {
    emit nameChanged(mCurrentProperties.getName());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setName(mCurrentProperties.getName(), true);
  }
}

void SymbolEditorState_AddPins::setLength(
    const UnsignedLength& length) noexcept {
  if (mCurrentProperties.setLength(length)) {
    emit lengthChanged(mCurrentProperties.getLength());
  }

  mCurrentProperties.setNamePosition(
      SymbolPin::getDefaultNamePosition(mCurrentProperties.getLength()));

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLength(mCurrentProperties.getLength(), true);
    mCurrentEditCmd->setNamePosition(mCurrentProperties.getNamePosition(),
                                     true);
  }
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_AddPins::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentEditCmd) {
    mCurrentEditCmd->setPosition(currentPos, true);
  }
  return true;
}

bool SymbolEditorState_AddPins::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
  try {
    if (mCurrentEditCmd) {
      mCurrentEditCmd->setPosition(currentPos, true);
      mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    return addNextPin(currentPos);
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool SymbolEditorState_AddPins::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool SymbolEditorState_AddPins::processRotate(const Angle& rotation) noexcept {
  if (mCurrentEditCmd) {
    mCurrentEditCmd->rotate(rotation, mCurrentPin->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentPin->getRotation());
  }
  return true;
}

bool SymbolEditorState_AddPins::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mCurrentEditCmd) {
    mCurrentEditCmd->mirror(orientation, mCurrentPin->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentPin->getRotation());
  }
  return true;
}

bool SymbolEditorState_AddPins::processImportPins() noexcept {
  try {
    CircuitIdentifierImportDialog dlg("symbol_editor/import_pins_dialog",
                                      parentWidget());
    if (dlg.exec() != QDialog::Accepted) {
      return true;
    }
    const QList<CircuitIdentifier> names = dlg.getValues();
    if (names.isEmpty()) {
      return true;
    }
    std::unique_ptr<SymbolClipboardData> data(
        new SymbolClipboardData(mContext.symbol.getUuid(), Point(0, 0)));
    Point pos(0, 0);
    foreach (const auto& name, names) {
      mCurrentProperties.setName(name);
      mCurrentProperties.setPosition(pos);
      data->getPins().append(std::make_shared<SymbolPin>(Uuid::createRandom(),
                                                         mCurrentProperties));
      pos.setY(pos.getY() - Length(2540000));
    }
    requestPaste(std::move(data));
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::addNextPin(const Point& pos) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add symbol pin"));
    setName(determineNextPinName());
    mCurrentProperties.setPosition(pos);
    mCurrentPin = std::make_shared<SymbolPin>(Uuid::createRandom(),
                                              mCurrentProperties);  // can throw
    mContext.undoStack.appendToCmdGroup(
        new CmdSymbolPinInsert(mContext.symbol.getPins(), mCurrentPin));
    mCurrentGraphicsItem = item->getGraphicsItem(mCurrentPin);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    mCurrentEditCmd.reset(new CmdSymbolPinEdit(mCurrentPin));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mCurrentEditCmd.reset();
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    return false;
  }
}

CircuitIdentifier SymbolEditorState_AddPins::determineNextPinName()
    const noexcept {
  int i = 1;
  while (hasPin(QString::number(i))) ++i;
  return CircuitIdentifier(QString::number(i));  // can throw, but should not
}

bool SymbolEditorState_AddPins::hasPin(const QString& name) const noexcept {
  return mContext.symbol.getPins().contains(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

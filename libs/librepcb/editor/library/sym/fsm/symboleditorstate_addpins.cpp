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

#include "../../../widgets/graphicsview.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdsymbolpinedit.h"
#include "../symboleditorwidget.h"
#include "../symbolgraphicsitem.h"
#include "../symbolpingraphicsitem.h"

#include <librepcb/core/graphics/graphicsscene.h>
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
    mCurrentPin(nullptr),
    mCurrentGraphicsItem(nullptr),
    mNameLineEdit(nullptr),
    mLastLength(2540000)  // Default length according library conventions
{
}

SymbolEditorState_AddPins::~SymbolEditorState_AddPins() noexcept {
  Q_ASSERT(mEditCmd.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection
  mContext.graphicsView.setCursor(Qt::CrossCursor);

  // populate command toolbar
  mContext.commandToolBar.addLabel(tr("Name:"));
  mNameLineEdit = new QLineEdit();
  std::unique_ptr<QLineEdit> nameLineEdit(mNameLineEdit);
  nameLineEdit->setMaxLength(20);
  nameLineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  connect(nameLineEdit.get(), &QLineEdit::textEdited, this,
          &SymbolEditorState_AddPins::nameLineEditTextChanged);
  mContext.commandToolBar.addWidget(std::move(nameLineEdit));

  mContext.commandToolBar.addLabel(tr("Length:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLength(new UnsignedLengthEdit());
  edtLength->configure(getDefaultLengthUnit(),
                       LengthEditBase::Steps::pinLength(),
                       "symbol_editor/add_pins/length");
  edtLength->setValue(mLastLength);
  connect(edtLength.get(), &UnsignedLengthEdit::valueChanged, this,
          &SymbolEditorState_AddPins::lengthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLength));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  return addNextPin(pos, Angle::deg0());
}

bool SymbolEditorState_AddPins::exit() noexcept {
  // abort command
  try {
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    mEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }

  // cleanup command toolbar
  mNameLineEdit = nullptr;
  mContext.commandToolBar.clear();

  mContext.graphicsView.setCursor(Qt::ArrowCursor);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_AddPins::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  mEditCmd->setPosition(currentPos, true);
  return true;
}

bool SymbolEditorState_AddPins::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  mEditCmd->setPosition(currentPos, true);
  Angle currentRot = mCurrentPin->getRotation();
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    return addNextPin(currentPos, currentRot);
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool SymbolEditorState_AddPins::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotateCcw();
}

bool SymbolEditorState_AddPins::processRotateCw() noexcept {
  mEditCmd->rotate(-Angle::deg90(), mCurrentPin->getPosition(), true);
  return true;
}

bool SymbolEditorState_AddPins::processRotateCcw() noexcept {
  mEditCmd->rotate(Angle::deg90(), mCurrentPin->getPosition(), true);
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::addNextPin(const Point& pos,
                                           const Angle& rot) noexcept {
  try {
    mNameLineEdit->setText(determineNextPinName());
    mContext.undoStack.beginCmdGroup(tr("Add symbol pin"));
    mCurrentPin = std::make_shared<SymbolPin>(
        Uuid::createRandom(), CircuitIdentifier(mNameLineEdit->text()), pos,
        mLastLength, rot);  // can throw

    mContext.undoStack.appendToCmdGroup(
        new CmdSymbolPinInsert(mContext.symbol.getPins(), mCurrentPin));
    mEditCmd.reset(new CmdSymbolPinEdit(*mCurrentPin));
    mCurrentGraphicsItem =
        mContext.symbolGraphicsItem.getGraphicsItem(mCurrentPin);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    mEditCmd.reset();
    return false;
  }
}

void SymbolEditorState_AddPins::nameLineEditTextChanged(
    const QString& text) noexcept {
  if (mEditCmd && (!text.trimmed().isEmpty())) {
    try {
      mEditCmd->setName(CircuitIdentifier(text.trimmed()), true);  // can throw
    } catch (const Exception&) {
      // invalid name
    }
  }
}

void SymbolEditorState_AddPins::lengthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastLength = value;
  if (mEditCmd) {
    mEditCmd->setLength(mLastLength, true);
  }
}

QString SymbolEditorState_AddPins::determineNextPinName() const noexcept {
  int i = 1;
  while (hasPin(QString::number(i))) ++i;
  return QString::number(i);
}

bool SymbolEditorState_AddPins::hasPin(const QString& name) const noexcept {
  return mContext.symbol.getPins().contains(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

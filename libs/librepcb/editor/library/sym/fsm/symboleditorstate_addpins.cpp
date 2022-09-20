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

#include "../../../editorcommandset.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdsymbolpinedit.h"
#include "../symboleditorwidget.h"
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
    mCurrentPin(nullptr),
    mCurrentGraphicsItem(nullptr),
    mNameLineEdit(nullptr),
    mLastRotation(0),
    mLastLength(2540000)  // Default length according library conventions
{
}

SymbolEditorState_AddPins::~SymbolEditorState_AddPins() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::entry() noexcept {
  EditorCommandSet& cmd = EditorCommandSet::instance();

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
  edtLength->addAction(cmd.sizeIncrease.createAction(
      edtLength.get(), edtLength.get(), &UnsignedLengthEdit::stepUp));
  edtLength->addAction(cmd.sizeDecrease.createAction(
      edtLength.get(), edtLength.get(), &UnsignedLengthEdit::stepDown));
  connect(edtLength.get(), &UnsignedLengthEdit::valueChanged, this,
          &SymbolEditorState_AddPins::lengthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtLength));

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!addNextPin(pos)) {
    return false;
  }
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_AddPins::exit() noexcept {
  // abort command
  try {
    mEditCmd.reset();
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    mContext.undoStack.abortCmdGroup();
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }

  // cleanup command toolbar
  mNameLineEdit = nullptr;
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    SymbolEditorState_AddPins::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Rotate,
      EditorWidgetBase::Feature::Mirror,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_AddPins::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mEditCmd) {
    mEditCmd->setPosition(currentPos, true);
  }
  return true;
}

bool SymbolEditorState_AddPins::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  try {
    if (mEditCmd) {
      mEditCmd->setPosition(currentPos, true);
      mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    }
    mContext.undoStack.commitCmdGroup();
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
    return addNextPin(currentPos);
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool SymbolEditorState_AddPins::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool SymbolEditorState_AddPins::processRotate(const Angle& rotation) noexcept {
  if (mEditCmd) {
    mEditCmd->rotate(rotation, mCurrentPin->getPosition(), true);
    mLastRotation = mCurrentPin->getRotation();
  }
  return true;
}

bool SymbolEditorState_AddPins::processMirror(
    Qt::Orientation orientation) noexcept {
  if (mEditCmd) {
    mEditCmd->mirror(orientation, mCurrentPin->getPosition(), true);
    mLastRotation = mCurrentPin->getRotation();
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_AddPins::addNextPin(const Point& pos) noexcept {
  try {
    mNameLineEdit->setText(determineNextPinName());
    mContext.undoStack.beginCmdGroup(tr("Add symbol pin"));
    mCurrentPin = std::make_shared<SymbolPin>(
        Uuid::createRandom(), CircuitIdentifier(mNameLineEdit->text()), pos,
        mLastLength, mLastRotation,
        SymbolPin::getDefaultNamePosition(mLastLength), Angle(0),
        SymbolPin::getDefaultNameHeight(),
        SymbolPin::getDefaultNameAlignment());  // can throw
    mContext.undoStack.appendToCmdGroup(
        new CmdSymbolPinInsert(mContext.symbol.getPins(), mCurrentPin));
    mCurrentGraphicsItem =
        mContext.symbolGraphicsItem.getGraphicsItem(mCurrentPin);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    mEditCmd.reset(new CmdSymbolPinEdit(mCurrentPin));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mEditCmd.reset();
    mCurrentGraphicsItem.reset();
    mCurrentPin.reset();
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
    mEditCmd->setNamePosition(SymbolPin::getDefaultNamePosition(mLastLength),
                              true);
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

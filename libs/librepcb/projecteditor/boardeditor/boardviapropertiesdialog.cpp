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
#include "boardviapropertiesdialog.h"

#include "ui_boardviapropertiesdialog.h"

#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/netsignal.h>

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

BoardViaPropertiesDialog::BoardViaPropertiesDialog(Project&   project,
                                                   BI_Via&    via,
                                                   UndoStack& undoStack,
                                                   QWidget*   parent) noexcept
  : QDialog(parent),
    mProject(project),
    mVia(via),
    mUi(new Ui::BoardViaPropertiesDialog),
    mUndoStack(undoStack) {
  mUi->setupUi(this);
  mUi->edtSize->setSingleStep(0.1);           // [mm]
  mUi->edtDrillDiameter->setSingleStep(0.1);  // [mm]

  // shape combobox
  mUi->cbxShape->addItem(tr("Round"), static_cast<int>(BI_Via::Shape::Round));
  mUi->cbxShape->addItem(tr("Square"), static_cast<int>(BI_Via::Shape::Square));
  mUi->cbxShape->addItem(tr("Octagon"),
                         static_cast<int>(BI_Via::Shape::Octagon));
  mUi->cbxShape->setCurrentIndex(
      mUi->cbxShape->findData(static_cast<int>(mVia.getShape())));

  // Position spinboxes
  mUi->edtPosX->setValue(mVia.getPosition().getX());
  mUi->edtPosY->setValue(mVia.getPosition().getY());

  // size spinbox
  mUi->edtSize->setValue(mVia.getSize());

  // drill diameter spinbox
  mUi->edtDrillDiameter->setValue(mVia.getDrillDiameter());

  // netsignal combobox
  mUi->lblNetSignal->setText(*mVia.getNetSignalOfNetSegment().getName());
}

BoardViaPropertiesDialog::~BoardViaPropertiesDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardViaPropertiesDialog::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Return:
      accept();
      break;
    case Qt::Key_Escape:
      reject();
      break;
    default:
      QDialog::keyPressEvent(e);
      break;
  }
}

void BoardViaPropertiesDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool BoardViaPropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(mVia));
    cmd->setShape(
        static_cast<BI_Via::Shape>(mUi->cbxShape->currentData().toInt()),
        false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setSize(mUi->edtSize->getValue(), false);
    cmd->setDrillDiameter(mUi->edtDrillDiameter->getValue(), false);
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

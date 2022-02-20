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

#include "../../project/cmd/cmdboardviaedit.h"
#include "../../undostack.h"
#include "ui_boardviapropertiesdialog.h"

#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/netsignal.h>

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

BoardViaPropertiesDialog::BoardViaPropertiesDialog(
    Project& project, BI_Via& via, UndoStack& undoStack,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mProject(project),
    mVia(via),
    mUi(new Ui::BoardViaPropertiesDialog),
    mUndoStack(undoStack) {
  mUi->setupUi(this);
  mUi->edtSize->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/size");
  mUi->edtDrillDiameter->configure(lengthUnit,
                                   LengthEditBase::Steps::drillDiameter(),
                                   settingsPrefix % "/drill_diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &BoardViaPropertiesDialog::buttonBoxClicked);

  // shape combobox
  mUi->cbxShape->addItem(tr("Round"), static_cast<int>(Via::Shape::Round));
  mUi->cbxShape->addItem(tr("Square"), static_cast<int>(Via::Shape::Square));
  mUi->cbxShape->addItem(tr("Octagon"), static_cast<int>(Via::Shape::Octagon));
  mUi->cbxShape->setCurrentIndex(
      mUi->cbxShape->findData(static_cast<int>(mVia.getShape())));

  // Position spinboxes
  mUi->edtPosX->setValue(mVia.getPosition().getX());
  mUi->edtPosY->setValue(mVia.getPosition().getY());

  // size spinbox
  mUi->edtSize->setValue(mVia.getSize());

  // drill diameter spinbox
  mUi->edtDrillDiameter->setValue(mVia.getDrillDiameter());

  // netsignal name
  mUi->lblNetSignal->setText(mVia.getNetSegment().getNetNameToDisplay(true));
}

BoardViaPropertiesDialog::~BoardViaPropertiesDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardViaPropertiesDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      if (applyChanges()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
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
    cmd->setShape(static_cast<Via::Shape>(mUi->cbxShape->currentData().toInt()),
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
}  // namespace librepcb

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
#include "footprintpadpropertiesdialog.h"

#include "../../undostack.h"
#include "../cmd/cmdfootprintpadedit.h"
#include "ui_footprintpadpropertiesdialog.h"

#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/package.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

FootprintPadPropertiesDialog::FootprintPadPropertiesDialog(
    const Package& pkg, const Footprint& fpt, FootprintPad& pad,
    UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mPad(pad),
    mUndoStack(undoStack),
    mUi(new Ui::FootprintPadPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                           settingsPrefix % "/width");
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::generic(),
                            settingsPrefix % "/height");
  mUi->edtDrillDiameter->configure(lengthUnit,
                                   LengthEditBase::Steps::drillDiameter(),
                                   settingsPrefix % "/drill_diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &FootprintPadPropertiesDialog::on_buttonBox_clicked);

  // Avoid creating pads with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtDrillDiameter->getValue()) {
              mUi->edtDrillDiameter->setValue(positiveToUnsigned(value));
            }
          });
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtDrillDiameter->getValue()) {
              mUi->edtDrillDiameter->setValue(positiveToUnsigned(value));
            }
          });
  connect(mUi->edtDrillDiameter, &UnsignedLengthEdit::valueChanged, this,
          [this](const UnsignedLength& value) {
            if (value > mUi->edtWidth->getValue()) {
              mUi->edtWidth->setValue(PositiveLength(*value));
            }
            if (value > mUi->edtHeight->getValue()) {
              mUi->edtHeight->setValue(PositiveLength(*value));
            }
          });

  // load pad attributes
  int currentPadIndex = -1;
  for (const PackagePad& p : pkg.getPads()) {
    if ((p.getUuid() == mPad.getUuid()) ||
        (!fpt.getPads().contains(p.getUuid()))) {
      mUi->cbxPackagePad->addItem(*p.getName(), p.getUuid().toStr());
      if (mPad.getPackagePadUuid() == p.getUuid()) {
        currentPadIndex = mUi->cbxPackagePad->count() - 1;
      }
    }
  }
  mUi->cbxPackagePad->setCurrentIndex(currentPadIndex);
  switch (mPad.getBoardSide()) {
    case FootprintPad::BoardSide::TOP:
      mUi->rbtnBoardSideTop->setChecked(true);
      break;
    case FootprintPad::BoardSide::BOTTOM:
      mUi->rbtnBoardSideBottom->setChecked(true);
      break;
    case FootprintPad::BoardSide::THT:
      mUi->rbtnBoardSideTht->setChecked(true);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  switch (mPad.getShape()) {
    case FootprintPad::Shape::ROUND:
      mUi->rbtnShapeRound->setChecked(true);
      break;
    case FootprintPad::Shape::RECT:
      mUi->rbtnShapeRect->setChecked(true);
      break;
    case FootprintPad::Shape::OCTAGON:
      mUi->rbtnShapeOctagon->setChecked(true);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  mUi->edtWidth->setValue(mPad.getWidth());
  mUi->edtHeight->setValue(mPad.getHeight());
  mUi->edtDrillDiameter->setValue(mPad.getDrillDiameter());
  mUi->edtPosX->setValue(mPad.getPosition().getX());
  mUi->edtPosY->setValue(mPad.getPosition().getY());
  mUi->edtRotation->setValue(mPad.getRotation());

  // disable drill diameter for SMT pads
  mUi->edtDrillDiameter->setEnabled(mUi->rbtnBoardSideTht->isChecked());
  connect(mUi->rbtnBoardSideTht, &QRadioButton::toggled, mUi->edtDrillDiameter,
          &LengthEdit::setEnabled);
}

FootprintPadPropertiesDialog::~FootprintPadPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintPadPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxPackagePad->setDisabled(readOnly);
  mUi->rbtnBoardSideTht->setDisabled(readOnly);
  mUi->rbtnBoardSideTop->setDisabled(readOnly);
  mUi->rbtnBoardSideBottom->setDisabled(readOnly);
  mUi->rbtnShapeRound->setDisabled(readOnly);
  mUi->rbtnShapeRect->setDisabled(readOnly);
  mUi->rbtnShapeOctagon->setDisabled(readOnly);
  mUi->edtDrillDiameter->setReadOnly(readOnly);
  mUi->edtWidth->setReadOnly(readOnly);
  mUi->edtHeight->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  if (readOnly) {
    mUi->buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Close);
  } else {
    mUi->buttonBox->setStandardButtons(
        QDialogButtonBox::StandardButton::Apply |
        QDialogButtonBox::StandardButton::Cancel |
        QDialogButtonBox::StandardButton::Ok);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintPadPropertiesDialog::on_buttonBox_clicked(
    QAbstractButton* button) {
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

bool FootprintPadPropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(mPad));
    Uuid pkgPad = Uuid::fromString(
        mUi->cbxPackagePad->currentData().toString());  // can throw
    cmd->setPackagePadUuid(pkgPad, false);
    if (mUi->rbtnBoardSideTop->isChecked()) {
      cmd->setBoardSide(FootprintPad::BoardSide::TOP, false);
    } else if (mUi->rbtnBoardSideBottom->isChecked()) {
      cmd->setBoardSide(FootprintPad::BoardSide::BOTTOM, false);
    } else if (mUi->rbtnBoardSideTht->isChecked()) {
      cmd->setBoardSide(FootprintPad::BoardSide::THT, false);
    } else {
      Q_ASSERT(false);
    }
    if (mUi->rbtnShapeRound->isChecked()) {
      cmd->setShape(FootprintPad::Shape::ROUND, false);
    } else if (mUi->rbtnShapeRect->isChecked()) {
      cmd->setShape(FootprintPad::Shape::RECT, false);
    } else if (mUi->rbtnShapeOctagon->isChecked()) {
      cmd->setShape(FootprintPad::Shape::OCTAGON, false);
    } else {
      Q_ASSERT(false);
    }
    cmd->setWidth(mUi->edtWidth->getValue(), false);
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setDrillDiameter(mUi->edtDrillDiameter->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
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

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
#include "circlepropertiesdialog.h"

#include "../geometry/circle.h"
#include "../geometry/cmd/cmdcircleedit.h"
#include "../graphics/graphicslayer.h"
#include "../undostack.h"
#include "ui_circlepropertiesdialog.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

CirclePropertiesDialog::CirclePropertiesDialog(Circle& circle,
                                               UndoStack& undoStack,
                                               QList<GraphicsLayer*> layers,
                                               const LengthUnit& lengthUnit,
                                               const QString& settingsPrefix,
                                               QWidget* parent) noexcept
  : QDialog(parent),
    mCircle(circle),
    mUndoStack(undoStack),
    mUi(new Ui::CirclePropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/line_width");
  mUi->edtDiameter->configure(lengthUnit, LengthEditBase::Steps::generic(),
                              settingsPrefix % "/diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");

  foreach (const GraphicsLayer* layer, layers) {
    mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
  }

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &CirclePropertiesDialog::buttonBoxClicked);

  // load circle attributes
  selectLayerNameInCombobox(*mCircle.getLayerName());
  mUi->edtLineWidth->setValue(mCircle.getLineWidth());
  mUi->cbxFillArea->setChecked(mCircle.isFilled());
  mUi->cbxIsGrabArea->setChecked(mCircle.isGrabArea());
  mUi->edtDiameter->setValue(mCircle.getDiameter());
  mUi->edtPosX->setValue(mCircle.getCenter().getX());
  mUi->edtPosY->setValue(mCircle.getCenter().getY());
}

CirclePropertiesDialog::~CirclePropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CirclePropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxLayer->setDisabled(readOnly);
  mUi->edtLineWidth->setReadOnly(readOnly);
  mUi->cbxFillArea->setCheckable(!readOnly);
  mUi->cbxIsGrabArea->setCheckable(!readOnly);
  mUi->edtDiameter->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
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

void CirclePropertiesDialog::buttonBoxClicked(
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

bool CirclePropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdCircleEdit> cmd(new CmdCircleEdit(mCircle));
    if (mUi->cbxLayer->currentIndex() >= 0 &&
        mUi->cbxLayer->currentData().isValid()) {
      cmd->setLayerName(
          GraphicsLayerName(mUi->cbxLayer->currentData().toString()),
          false);  // can throw
    }
    cmd->setIsFilled(mUi->cbxFillArea->isChecked(), false);
    cmd->setIsGrabArea(mUi->cbxIsGrabArea->isChecked(), false);
    cmd->setLineWidth(mUi->edtLineWidth->getValue(), false);
    cmd->setDiameter(mUi->edtDiameter->getValue(), false);
    cmd->setCenter(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                   false);
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

void CirclePropertiesDialog::selectLayerNameInCombobox(
    const QString& name) noexcept {
  mUi->cbxLayer->setCurrentIndex(mUi->cbxLayer->findData(name));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

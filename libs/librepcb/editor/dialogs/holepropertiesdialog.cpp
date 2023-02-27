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
#include "holepropertiesdialog.h"

#include "../cmd/cmdholeedit.h"
#include "../undostack.h"
#include "ui_holepropertiesdialog.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

HolePropertiesDialog::HolePropertiesDialog(Hole& hole, UndoStack& undoStack,
                                           const LengthUnit& lengthUnit,
                                           const QString& settingsPrefix,
                                           QWidget* parent) noexcept
  : QDialog(parent),
    mHole(hole),
    mUndoStack(undoStack),
    mUi(new Ui::HolePropertiesDialog) {
  mUi->setupUi(this);
  mUi->holeEditorWidget->configureClientSettings(lengthUnit, settingsPrefix);
  connect(mUi->rbtnStopMaskManual, &QRadioButton::toggled,
          mUi->edtStopMaskOffset, &LengthEdit::setEnabled);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &HolePropertiesDialog::on_buttonBox_clicked);

  // Set properties.
  mUi->holeEditorWidget->setDiameter(mHole.getDiameter());
  mUi->holeEditorWidget->setPath(mHole.getPath());
  if (!mHole.getStopMaskConfig().isEnabled()) {
    mUi->rbtnStopMaskOff->setChecked(true);
  } else if (tl::optional<Length> offset =
                 mHole.getStopMaskConfig().getOffset()) {
    mUi->rbtnStopMaskManual->setChecked(true);
    mUi->edtStopMaskOffset->setValue(*offset);
  } else {
    mUi->rbtnStopMaskAuto->setChecked(true);
  }

  // Set focus to diameter so the user can immediately start typing to change it
  mUi->tabWidget->setCurrentIndex(0);
  mUi->holeEditorWidget->setFocusToDiameterEdit();
}

HolePropertiesDialog::~HolePropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void HolePropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->holeEditorWidget->setReadOnly(readOnly);
  mUi->rbtnStopMaskOff->setEnabled(!readOnly);
  mUi->rbtnStopMaskAuto->setEnabled(!readOnly);
  mUi->rbtnStopMaskManual->setEnabled(!readOnly);
  mUi->edtStopMaskOffset->setReadOnly(readOnly);
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

void HolePropertiesDialog::on_buttonBox_clicked(QAbstractButton* button) {
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

bool HolePropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdHoleEdit> cmd(new CmdHoleEdit(mHole));
    cmd->setDiameter(mUi->holeEditorWidget->getDiameter(), false);
    cmd->setPath(mUi->holeEditorWidget->getPath(), false);
    if (mUi->rbtnStopMaskOff->isChecked()) {
      cmd->setStopMaskConfig(MaskConfig::off());
    } else if (mUi->rbtnStopMaskAuto->isChecked()) {
      cmd->setStopMaskConfig(MaskConfig::automatic());
    } else if (mUi->rbtnStopMaskManual->isChecked()) {
      cmd->setStopMaskConfig(
          MaskConfig::manual(mUi->edtStopMaskOffset->getValue()));
    } else {
      qCritical() << "Unknown UI configuration for hole stop mask.";
    }
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

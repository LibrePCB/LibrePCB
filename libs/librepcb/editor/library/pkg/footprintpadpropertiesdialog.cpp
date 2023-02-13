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
    const Package& pkg, FootprintPad& pad, UndoStack& undoStack,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mPad(pad),
    mUndoStack(undoStack),
    mHoles(mPad.getHoles()),
    mSelectedHoleIndex(-1),
    mUi(new Ui::FootprintPadPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                           settingsPrefix % "/width");
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::generic(),
                            settingsPrefix % "/height");
  mUi->edtHoleDiameter->configure(lengthUnit,
                                  LengthEditBase::Steps::drillDiameter(),
                                  settingsPrefix % "/hole_diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  mUi->customShapePathEditor->setLengthUnit(lengthUnit);
  mUi->holeEditorWidget->configureClientSettings(
      lengthUnit, settingsPrefix % "/hole_editor");
  connect(mUi->lblHoleDetails, &QLabel::linkActivated, this,
          [this]() { mUi->tabWidget->setCurrentWidget(mUi->tabHoles); });
  connect(mUi->btnConvertToSmt, &QToolButton::clicked, this,
          &FootprintPadPropertiesDialog::removeAllHoles);
  connect(mUi->btnConvertToTht, &QToolButton::clicked, this,
          &FootprintPadPropertiesDialog::addHole);
  connect(mUi->holeEditorWidget, &HoleEditorWidget::holeChanged, this,
          [this](const Hole& hole) {
            const int index = qBound(0, mSelectedHoleIndex, mHoles.count() - 1);
            if (const std::shared_ptr<Hole> holePtr = mHoles.value(index)) {
              *holePtr = hole;
              updateGeneralTabHoleWidgets();
            }
          });
  connect(mUi->btnPreviousHole, &QToolButton::clicked, this,
          [this]() { setSelectedHole(mSelectedHoleIndex - 1); });
  connect(mUi->btnNextHole, &QToolButton::clicked, this,
          [this]() { setSelectedHole(mSelectedHoleIndex + 1); });
  connect(mUi->btnRemoveHole, &QToolButton::clicked, this,
          &FootprintPadPropertiesDialog::removeSelectedHole);
  connect(mUi->btnAddHole, &QToolButton::clicked, this,
          &FootprintPadPropertiesDialog::addHole);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &FootprintPadPropertiesDialog::on_buttonBox_clicked);

  // Disable width/height inputs if custom shape is selected.
  connect(mUi->rbtnShapeCustom, &QRadioButton::toggled, mUi->edtWidth,
          &PositiveLengthEdit::setDisabled);
  connect(mUi->rbtnShapeCustom, &QRadioButton::toggled, mUi->edtHeight,
          &PositiveLengthEdit::setDisabled);

  // Automatically set/clear custom shape outline to improve user experience.
  mAutoCustomOutline = mPad.getCustomShapeOutline();
  if (mPad.getShape() != FootprintPad::Shape::Custom) {
    const QVector<Path> outlines = mPad.getGeometry().toOutlines();
    if (!outlines.isEmpty()) {
      mAutoCustomOutline = outlines.first().toOpenPath();
    }
  }
  connect(mUi->rbtnShapeCustom, &QRadioButton::toggled, this,
          [this](bool custom) {
            const Path path = mUi->customShapePathEditor->getPath();
            if (custom && (path.getVertices().isEmpty())) {
              mUi->customShapePathEditor->setPath(mAutoCustomOutline);
            } else if (!custom) {
              mAutoCustomOutline = path;
              mUi->customShapePathEditor->setPath(Path());
            }
          });

  // Avoid creating pads with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtHoleDiameter->getValue()) {
              mUi->edtHoleDiameter->setValue(value);
            }
          });
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtHoleDiameter->getValue()) {
              mUi->edtHoleDiameter->setValue(value);
            }
          });
  connect(mUi->edtHoleDiameter, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value > mUi->edtWidth->getValue()) {
              mUi->edtWidth->setValue(value);
            }
            if (value > mUi->edtHeight->getValue()) {
              mUi->edtHeight->setValue(value);
            }
            if (const std::shared_ptr<Hole> holePtr = mHoles.value(0)) {
              holePtr->setDiameter(value);
              mUi->holeEditorWidget->setHole(*holePtr);
            }
          });

  // load pad attributes
  int currentPadIndex = 0;
  mUi->cbxPackagePad->addItem(tr("(not connected)"), "");
  for (const PackagePad& p : pkg.getPads()) {
    mUi->cbxPackagePad->addItem(*p.getName(), p.getUuid().toStr());
    if (mPad.getPackagePadUuid() == p.getUuid()) {
      currentPadIndex = mUi->cbxPackagePad->count() - 1;
    }
  }
  mUi->cbxPackagePad->setCurrentIndex(currentPadIndex);
  if (mPad.getComponentSide() == FootprintPad::ComponentSide::Bottom) {
    mUi->rbtnComponentSideBottom->setChecked(true);
  } else {
    mUi->rbtnComponentSideTop->setChecked(true);
  }
  switch (mPad.getShape()) {
    case FootprintPad::Shape::Round:
      mUi->rbtnShapeRound->setChecked(true);
      break;
    case FootprintPad::Shape::Rect:
      mUi->rbtnShapeRect->setChecked(true);
      break;
    case FootprintPad::Shape::Octagon:
      mUi->rbtnShapeOctagon->setChecked(true);
      break;
    case FootprintPad::Shape::Custom:
      mUi->rbtnShapeCustom->setChecked(true);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  mUi->edtWidth->setValue(mPad.getWidth());
  mUi->edtHeight->setValue(mPad.getHeight());
  mUi->edtPosX->setValue(mPad.getPosition().getX());
  mUi->edtPosY->setValue(mPad.getPosition().getY());
  mUi->edtRotation->setValue(mPad.getRotation());
  mUi->customShapePathEditor->setPath(mPad.getCustomShapeOutline());
  updateGeneralTabHoleWidgets();
  setSelectedHole(0);

  // Always select first tab.
  mUi->tabWidget->setCurrentIndex(0);
}

FootprintPadPropertiesDialog::~FootprintPadPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void FootprintPadPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxPackagePad->setDisabled(readOnly);
  mUi->rbtnComponentSideTop->setDisabled(readOnly);
  mUi->rbtnComponentSideBottom->setDisabled(readOnly);
  mUi->rbtnShapeRound->setDisabled(readOnly);
  mUi->rbtnShapeRect->setDisabled(readOnly);
  mUi->rbtnShapeOctagon->setDisabled(readOnly);
  mUi->rbtnShapeCustom->setDisabled(readOnly);
  mUi->edtHoleDiameter->setReadOnly(readOnly);
  mUi->btnConvertToSmt->setEnabled(!readOnly);
  mUi->btnConvertToTht->setEnabled(!readOnly);
  mUi->edtWidth->setReadOnly(readOnly);
  mUi->edtHeight->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->btnRemoveHole->setVisible(!readOnly);
  mUi->btnAddHole->setVisible(!readOnly);
  mUi->customShapePathEditor->setReadOnly(readOnly);
  mUi->holeEditorWidget->setReadOnly(readOnly);
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

void FootprintPadPropertiesDialog::addHole() noexcept {
  mHoles.append(std::make_shared<Hole>(
      Uuid::createRandom(), PositiveLength(800000), makeNonEmptyPath(Point())));
  setSelectedHole(mHoles.count() - 1);
  updateGeneralTabHoleWidgets();
}

void FootprintPadPropertiesDialog::removeSelectedHole() noexcept {
  mHoles.remove(mSelectedHoleIndex);
  setSelectedHole(mSelectedHoleIndex);
  updateGeneralTabHoleWidgets();
}

void FootprintPadPropertiesDialog::removeAllHoles() noexcept {
  mHoles.clear();
  setSelectedHole(0);
  updateGeneralTabHoleWidgets();
}

void FootprintPadPropertiesDialog::updateGeneralTabHoleWidgets() noexcept {
  if (mHoles.isEmpty()) {
    mUi->lblHoleDetails->setVisible(false);
    mUi->edtHoleDiameter->setVisible(false);
    mUi->btnConvertToSmt->setVisible(false);
    mUi->btnConvertToTht->setVisible(true);
  } else {
    mUi->btnConvertToTht->setVisible(false);
    if (mHoles.count() == 1) {
      mUi->lblHoleDetails->setVisible(false);
      mUi->edtHoleDiameter->setVisible(true);
      mUi->edtHoleDiameter->setValue(mHoles.first()->getDiameter());
    } else {
      mUi->edtHoleDiameter->setVisible(false);
      mUi->lblHoleDetails->setVisible(true);
    }
    mUi->btnConvertToSmt->setVisible(true);
  }
}

void FootprintPadPropertiesDialog::setSelectedHole(int index) noexcept {
  mSelectedHoleIndex = qBound(0, index, mHoles.count() - 1);
  const std::shared_ptr<Hole> hole = mHoles.value(mSelectedHoleIndex);
  if (hole) {
    mUi->lblSelectedHole->setText(
        tr("Hole %1 of %2").arg(mSelectedHoleIndex + 1).arg(mHoles.count()));
    mUi->holeEditorWidget->setHole(*hole);
  } else {
    mUi->lblSelectedHole->setText(tr("Pad has no holes"));
  }
  mUi->btnPreviousHole->setEnabled(mSelectedHoleIndex > 0);
  mUi->btnNextHole->setEnabled(mSelectedHoleIndex < (mHoles.count() - 1));
  mUi->btnRemoveHole->setEnabled(!mHoles.isEmpty());
  mUi->holeEditorWidget->setVisible(hole ? true : false);
}

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
  // Clean and validate custom outline path.
  const Path customOutlinePath =
      mUi->customShapePathEditor->getPath().cleaned().toOpenPath();
  mUi->customShapePathEditor->setPath(customOutlinePath);
  if (mUi->rbtnShapeCustom->isChecked() &&
      (!PadGeometry::isValidCustomOutline(customOutlinePath))) {
    QMessageBox::critical(
        this, tr("Invalid outline"),
        tr("The custom pad outline does not represent a valid area."));
    return false;
  }

  try {
    QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(mPad));
    tl::optional<Uuid> pkgPad =
        Uuid::tryFromString(mUi->cbxPackagePad->currentData().toString());
    cmd->setPackagePadUuid(pkgPad, false);
    if (mUi->rbtnComponentSideTop->isChecked()) {
      cmd->setComponentSide(FootprintPad::ComponentSide::Top, false);
    } else if (mUi->rbtnComponentSideBottom->isChecked()) {
      cmd->setComponentSide(FootprintPad::ComponentSide::Bottom, false);
    } else {
      Q_ASSERT(false);
    }
    if (mUi->rbtnShapeRound->isChecked()) {
      cmd->setShape(FootprintPad::Shape::Round, false);
    } else if (mUi->rbtnShapeRect->isChecked()) {
      cmd->setShape(FootprintPad::Shape::Rect, false);
    } else if (mUi->rbtnShapeOctagon->isChecked()) {
      cmd->setShape(FootprintPad::Shape::Octagon, false);
    } else if (mUi->rbtnShapeCustom->isChecked()) {
      cmd->setShape(FootprintPad::Shape::Custom, false);
    } else {
      Q_ASSERT(false);
    }
    cmd->setWidth(mUi->edtWidth->getValue(), false);
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setCustomShapeOutline(customOutlinePath);
    cmd->setHoles(mHoles, false);
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

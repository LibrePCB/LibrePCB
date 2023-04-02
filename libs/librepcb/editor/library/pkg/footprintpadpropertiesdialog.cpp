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
  mUi->edtRadiusRatio->setSingleStep(1.0);  // [%]
  mUi->edtRadiusAbs->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/radius_abs");
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
          [this](const PositiveLength& diameter, const NonEmptyPath& path) {
            const int index = qBound(0, mSelectedHoleIndex, mHoles.count() - 1);
            if (const std::shared_ptr<PadHole> holePtr = mHoles.value(index)) {
              holePtr->setDiameter(diameter);
              holePtr->setPath(path);
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

  // Disable some widgets if not applicable for the selected shape.
  connect(mUi->btnShapeRound, &QRadioButton::toggled, this,
          &FootprintPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeRect, &QRadioButton::toggled, this,
          &FootprintPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeRoundedRect, &QRadioButton::toggled, this,
          &FootprintPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeOctagon, &QRadioButton::toggled, this,
          &FootprintPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeCustom, &QRadioButton::toggled, this,
          &FootprintPadPropertiesDialog::updateShapeDependentWidgets);

  // Automatically set/clear custom shape outline to improve user experience.
  mAutoCustomOutline = mPad.getCustomShapeOutline();
  if (mPad.getShape() != FootprintPad::Shape::Custom) {
    const QVector<Path> outlines = mPad.getGeometry().toOutlines();
    if (!outlines.isEmpty()) {
      mAutoCustomOutline = outlines.first().toOpenPath();
    }
  }
  connect(mUi->btnShapeCustom, &QRadioButton::toggled, this,
          [this](bool custom) {
            const Path path = mUi->customShapePathEditor->getPath();
            if (custom && (path.getVertices().isEmpty())) {
              mUi->customShapePathEditor->setPath(mAutoCustomOutline);
            } else if (!custom) {
              mAutoCustomOutline = path;
              mUi->customShapePathEditor->setPath(Path());
            }
          });

  // Auto-update relative and absolute radius input widgets.
  connect(mUi->edtRadiusAbs, &UnsignedLengthEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::updateRelativeRadius);
  connect(mUi->edtRadiusRatio, &UnsignedLimitedRatioEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::updateAbsoluteRadius);
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::updateAbsoluteRadius);
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::updateAbsoluteRadius);

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
            if (const std::shared_ptr<PadHole> holePtr = mHoles.value(0)) {
              holePtr->setDiameter(value);
              mUi->holeEditorWidget->setDiameter(value);
            }
          });

  // Enable custom mask offset only when allowed.
  connect(mUi->rbtnStopMaskManual, &QRadioButton::toggled,
          mUi->edtStopMaskOffset, &LengthEdit::setEnabled);
  connect(mUi->rbtnSolderPasteManual, &QRadioButton::toggled,
          mUi->edtSolderPasteOffset, &LengthEdit::setEnabled);

  // Populate functions.
  for (int i = 0; i < static_cast<int>(FootprintPad::Function::_COUNT); ++i) {
    const FootprintPad::Function value = static_cast<FootprintPad::Function>(i);
    mUi->cbxFunction->addItem(FootprintPad::getFunctionDescriptionTr(value),
                              QVariant::fromValue(value));
  }

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
  mUi->cbxFunction->setCurrentIndex(
      mUi->cbxFunction->findData(QVariant::fromValue(mPad.getFunction())));
  if (mPad.getComponentSide() == FootprintPad::ComponentSide::Bottom) {
    mUi->btnComponentSideBottom->setChecked(true);
  } else {
    mUi->btnComponentSideTop->setChecked(true);
  }
  switch (mPad.getShape()) {
    case FootprintPad::Shape::RoundedRect:
      mUi->btnShapeRound->setChecked(*mPad.getRadius() == Ratio::percent100());
      mUi->btnShapeRect->setChecked(*mPad.getRadius() == Ratio::percent0());
      mUi->btnShapeRoundedRect->setChecked(
          (*mPad.getRadius() != Ratio::percent0()) &&
          (*mPad.getRadius() != Ratio::percent100()));
      break;
    case FootprintPad::Shape::RoundedOctagon:
      mUi->btnShapeOctagon->setChecked(true);
      break;
    case FootprintPad::Shape::Custom:
      mUi->btnShapeCustom->setChecked(true);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  mUi->edtRadiusRatio->setValue(mPad.getRadius());
  mUi->edtWidth->setValue(mPad.getWidth());
  mUi->edtHeight->setValue(mPad.getHeight());
  mUi->edtPosX->setValue(mPad.getPosition().getX());
  mUi->edtPosY->setValue(mPad.getPosition().getY());
  mUi->edtRotation->setValue(mPad.getRotation());
  mUi->customShapePathEditor->setPath(mPad.getCustomShapeOutline());
  if (!mPad.getStopMaskConfig().isEnabled()) {
    mUi->rbtnStopMaskOff->setChecked(true);
  } else if (tl::optional<Length> offset =
                 mPad.getStopMaskConfig().getOffset()) {
    mUi->rbtnStopMaskManual->setChecked(true);
    mUi->edtStopMaskOffset->setValue(*offset);
  } else {
    mUi->rbtnStopMaskAuto->setChecked(true);
  }
  if (!mPad.getSolderPasteConfig().isEnabled()) {
    mUi->rbtnSolderPasteOff->setChecked(true);
  } else if (tl::optional<Length> offset =
                 mPad.getSolderPasteConfig().getOffset()) {
    mUi->rbtnSolderPasteManual->setChecked(true);
    mUi->edtSolderPasteOffset->setValue(*offset);
  } else {
    mUi->rbtnSolderPasteAuto->setChecked(true);
  }
  updateGeneralTabHoleWidgets();
  setSelectedHole(0);

  // Auto-update radius when manually(!) modifying the size.
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::applyRecommendedRadius);
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          &FootprintPadPropertiesDialog::applyRecommendedRadius);

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
  mUi->cbxFunction->setDisabled(readOnly);
  mUi->btnComponentSideTop->setDisabled(readOnly);
  mUi->btnComponentSideBottom->setDisabled(readOnly);
  mUi->btnShapeRound->setDisabled(readOnly);
  mUi->btnShapeRect->setDisabled(readOnly);
  mUi->btnShapeRoundedRect->setDisabled(readOnly);
  mUi->btnShapeOctagon->setDisabled(readOnly);
  mUi->btnShapeCustom->setDisabled(readOnly);
  mUi->edtRadiusRatio->setReadOnly(readOnly);
  mUi->edtRadiusAbs->setReadOnly(readOnly);
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
  mUi->rbtnStopMaskOff->setEnabled(!readOnly);
  mUi->rbtnStopMaskAuto->setEnabled(!readOnly);
  mUi->rbtnStopMaskManual->setEnabled(!readOnly);
  mUi->edtStopMaskOffset->setReadOnly(readOnly);
  mUi->rbtnSolderPasteOff->setEnabled(!readOnly);
  mUi->rbtnSolderPasteAuto->setEnabled(!readOnly);
  mUi->rbtnSolderPasteManual->setEnabled(!readOnly);
  mUi->edtSolderPasteOffset->setReadOnly(readOnly);
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

void FootprintPadPropertiesDialog::updateShapeDependentWidgets(
    bool checked) noexcept {
  if (checked) {
    const bool round = mUi->btnShapeRound->isChecked();
    const bool roundedRect = mUi->btnShapeRoundedRect->isChecked();
    const bool octagon = mUi->btnShapeOctagon->isChecked();
    const bool custom = mUi->btnShapeCustom->isChecked();
    mUi->edtRadiusRatio->setEnabled(roundedRect || octagon);
    mUi->edtRadiusAbs->setEnabled(roundedRect || octagon);
    mUi->edtWidth->setEnabled(!custom);
    mUi->edtHeight->setEnabled(!custom);
    if (round) {
      mUi->edtRadiusRatio->setValue(UnsignedLimitedRatio(Ratio::percent100()));
    } else if (roundedRect) {
      applyRecommendedRadius();
    } else {
      mUi->edtRadiusRatio->setValue(UnsignedLimitedRatio(Ratio::percent0()));
    }
  }
}

void FootprintPadPropertiesDialog::updateAbsoluteRadius() noexcept {
  QSignalBlocker blocker(mUi->edtRadiusAbs);  // Avoid endless loop.
  const UnsignedLimitedRatio ratio = mUi->edtRadiusRatio->getValue();
  const Length maxValue =
      std::min(mUi->edtWidth->getValue(), mUi->edtHeight->getValue()) / 2;
  mUi->edtRadiusAbs->setValue(UnsignedLength(
      qBound(Length(0), Length::fromMm(maxValue.toMm() * ratio->toNormalized()),
             maxValue)));
}

void FootprintPadPropertiesDialog::updateRelativeRadius() noexcept {
  QSignalBlocker blocker(mUi->edtRadiusRatio);  // Avoid endless loop.
  const UnsignedLength value = mUi->edtRadiusAbs->getValue();
  const Length maxValue =
      std::min(mUi->edtWidth->getValue(), mUi->edtHeight->getValue()) / 2;
  mUi->edtRadiusRatio->setValue(UnsignedLimitedRatio(qBound(
      Ratio::percent0(), Ratio::fromNormalized(value->toMm() / maxValue.toMm()),
      Ratio::percent100())));
}

void FootprintPadPropertiesDialog::applyRecommendedRadius() noexcept {
  if (mUi->btnShapeRoundedRect->isChecked()) {
    mUi->edtRadiusRatio->setValue(FootprintPad::getRecommendedRadius(
        mUi->edtWidth->getValue(), mUi->edtHeight->getValue()));
  }
}

void FootprintPadPropertiesDialog::addHole() noexcept {
  mHoles.append(std::make_shared<PadHole>(
      Uuid::createRandom(), PositiveLength(800000), makeNonEmptyPath(Point())));
  setSelectedHole(mHoles.count() - 1);
  if (mHoles.count() == 1) {
    applyTypicalThtProperties();
  }
  updateGeneralTabHoleWidgets();
}

void FootprintPadPropertiesDialog::removeSelectedHole() noexcept {
  mHoles.remove(mSelectedHoleIndex);
  setSelectedHole(mSelectedHoleIndex);
  if (mHoles.isEmpty()) {
    applyTypicalSmtProperties();
  }
  updateGeneralTabHoleWidgets();
}

void FootprintPadPropertiesDialog::removeAllHoles() noexcept {
  mHoles.clear();
  setSelectedHole(0);
  applyTypicalSmtProperties();
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
  const std::shared_ptr<PadHole> hole = mHoles.value(mSelectedHoleIndex);
  if (hole) {
    mUi->lblSelectedHole->setText(
        tr("Hole %1 of %2").arg(mSelectedHoleIndex + 1).arg(mHoles.count()));
    mUi->holeEditorWidget->setDiameter(hole->getDiameter());
    mUi->holeEditorWidget->setPath(hole->getPath());
  } else {
    mUi->lblSelectedHole->setText(tr("Pad has no holes"));
  }
  mUi->btnPreviousHole->setEnabled(mSelectedHoleIndex > 0);
  mUi->btnNextHole->setEnabled(mSelectedHoleIndex < (mHoles.count() - 1));
  mUi->btnRemoveHole->setEnabled(!mHoles.isEmpty());
  mUi->holeEditorWidget->setVisible(hole ? true : false);
}

void FootprintPadPropertiesDialog::applyTypicalThtProperties() noexcept {
  mUi->rbtnSolderPasteOff->setChecked(true);
}

void FootprintPadPropertiesDialog::applyTypicalSmtProperties() noexcept {
  mUi->rbtnSolderPasteAuto->setChecked(true);
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
  if (mUi->btnShapeCustom->isChecked() &&
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
    QVariant function = mUi->cbxFunction->currentData();
    if (function.isValid() && function.canConvert<FootprintPad::Function>()) {
      cmd->setFunction(function.value<FootprintPad::Function>(), false);
    }
    if (mUi->btnComponentSideTop->isChecked()) {
      cmd->setComponentSide(FootprintPad::ComponentSide::Top, false);
    } else if (mUi->btnComponentSideBottom->isChecked()) {
      cmd->setComponentSide(FootprintPad::ComponentSide::Bottom, false);
    } else {
      Q_ASSERT(false);
    }
    if (mUi->btnShapeOctagon->isChecked()) {
      cmd->setShape(FootprintPad::Shape::RoundedOctagon, false);
    } else if (mUi->btnShapeCustom->isChecked()) {
      cmd->setShape(FootprintPad::Shape::Custom, false);
    } else {
      cmd->setShape(FootprintPad::Shape::RoundedRect, false);
    }
    cmd->setRadius(mUi->edtRadiusRatio->getValue(), false);
    cmd->setWidth(mUi->edtWidth->getValue(), false);
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setCustomShapeOutline(customOutlinePath);
    if (mUi->rbtnStopMaskManual->isChecked()) {
      cmd->setStopMaskConfig(
          MaskConfig::manual(mUi->edtStopMaskOffset->getValue()));
    } else if (mUi->rbtnStopMaskAuto->isChecked()) {
      cmd->setStopMaskConfig(MaskConfig::automatic());
    } else {
      cmd->setStopMaskConfig(MaskConfig::off());
    }
    if (mUi->rbtnSolderPasteManual->isChecked()) {
      cmd->setSolderPasteConfig(
          MaskConfig::manual(mUi->edtSolderPasteOffset->getValue()));
    } else if (mUi->rbtnSolderPasteAuto->isChecked()) {
      cmd->setSolderPasteConfig(MaskConfig::automatic());
    } else {
      cmd->setSolderPasteConfig(MaskConfig::off());
    }
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

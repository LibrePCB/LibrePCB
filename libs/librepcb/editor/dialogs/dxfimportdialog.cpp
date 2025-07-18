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
#include "dxfimportdialog.h"

#include "../widgets/lengtheditbase.h"
#include "filedialog.h"
#include "ui_dxfimportdialog.h"

#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DxfImportDialog::DxfImportDialog(const QSet<const Layer*>& layers,
                                 const Layer& defaultLayer, bool supportHoles,
                                 const LengthUnit& lengthUnit,
                                 const QString& settingsPrefix,
                                 QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::DxfImportDialog),
    mSettingsPrefix(settingsPrefix),
    mDefaultLayer(defaultLayer) {
  mUi->setupUi(this);
  mUi->cbxCirclesAsDrills->setVisible(supportHoles);
  mUi->cbxLayer->setLayers(layers);
  mUi->edtLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/line_width");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  connect(mUi->cbxInteractivePlacement, &QCheckBox::toggled, mUi->edtPosX,
          &QCheckBox::setDisabled);
  connect(mUi->cbxInteractivePlacement, &QCheckBox::toggled, mUi->edtPosY,
          &QCheckBox::setDisabled);

  // Load initial values and window geometry.
  try {
    QSettings cs;
    mUi->cbxLayer->setCurrentLayer(Layer::get(
        cs.value(settingsPrefix % "/layer", defaultLayer.getId()).toString()));
    mUi->edtLineWidth->setValue(UnsignedLength(Length::fromMm(
        cs.value(settingsPrefix % "/line_width", "0").toString())));
    mUi->spbxScaleFactor->setValue(
        cs.value(settingsPrefix % "/scale_factor", "1").toDouble());
    mUi->cbxInteractivePlacement->setChecked(
        cs.value(settingsPrefix % "/interactive_placement", true).toBool());
    mUi->edtPosX->setValue(
        Length::fromMm(cs.value(settingsPrefix % "/pos_x", "0").toString()));
    mUi->edtPosY->setValue(
        Length::fromMm(cs.value(settingsPrefix % "/pos_y", "0").toString()));
    mUi->cbxJoinTangentPolylines->setChecked(
        cs.value(settingsPrefix % "/join_tangent_polylines", true).toBool());
    mUi->cbxCirclesAsDrills->setChecked(
        cs.value(settingsPrefix % "/circles_as_drills", false).toBool());
    const QSize windowSize = cs.value(settingsPrefix % "/window_size").toSize();
    if (!windowSize.isEmpty()) {
      resize(windowSize);
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to initialize DXF import dialog:" << e.getMsg();
  }
}

DxfImportDialog::~DxfImportDialog() noexcept {
  // Save client settings.
  QSettings cs;
  if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
    cs.setValue(mSettingsPrefix % "/layer", layer->getId());
  }
  cs.setValue(mSettingsPrefix % "/line_width",
              mUi->edtLineWidth->getValue()->toMmString());
  cs.setValue(mSettingsPrefix % "/scale_factor", mUi->spbxScaleFactor->value());
  cs.setValue(mSettingsPrefix % "/interactive_placement",
              mUi->cbxInteractivePlacement->isChecked());
  cs.setValue(mSettingsPrefix % "/pos_x",
              mUi->edtPosX->getValue().toMmString());
  cs.setValue(mSettingsPrefix % "/pos_y",
              mUi->edtPosY->getValue().toMmString());
  cs.setValue(mSettingsPrefix % "/join_tangent_polylines",
              mUi->cbxJoinTangentPolylines->isChecked());
  cs.setValue(mSettingsPrefix % "/circles_as_drills",
              mUi->cbxCirclesAsDrills->isChecked());
  cs.setValue(mSettingsPrefix % "/window_size", size());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Layer& DxfImportDialog::getLayer() const noexcept {
  if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
    return *layer;
  } else {
    return mDefaultLayer;
  }
}

UnsignedLength DxfImportDialog::getLineWidth() const noexcept {
  return mUi->edtLineWidth->getValue();
}

qreal DxfImportDialog::getScaleFactor() const noexcept {
  return mUi->spbxScaleFactor->value();
}

std::optional<Point> DxfImportDialog::getPlacementPosition() const noexcept {
  if (mUi->cbxInteractivePlacement->isChecked()) {
    return std::nullopt;
  } else {
    return Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue());
  }
}

bool DxfImportDialog::getJoinTangentPolylines() const noexcept {
  return mUi->cbxJoinTangentPolylines->isChecked();
}

bool DxfImportDialog::getImportCirclesAsDrills() const noexcept {
  return mUi->cbxCirclesAsDrills->isChecked();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath DxfImportDialog::chooseFile() const noexcept {
  QSettings cs;
  QString key = mSettingsPrefix % "/file";
  QString selectedFile = cs.value(key, QDir::homePath()).toString();
  FilePath fp(FileDialog::getOpenFileName(parentWidget(), tr("Choose file"),
                                          selectedFile, "*.dxf;;*"));
  if (fp.isValid()) {
    cs.setValue(key, fp.toStr());
  }
  return fp;
}

void DxfImportDialog::throwNoObjectsImportedError() {
  throw RuntimeError(
      __FILE__, __LINE__,
      tr("The selected file does not contain any objects to import."));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

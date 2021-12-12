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

#include <librepcb/core/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DxfImportDialog::DxfImportDialog(QList<GraphicsLayer*> layers,
                                 const GraphicsLayerName& defaultLayer,
                                 bool supportHoles,
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
    QSettings clientSettings;
    mUi->cbxLayer->setCurrentLayer(GraphicsLayerName(
        clientSettings.value(settingsPrefix % "/layer", *defaultLayer)
            .toString()));
    mUi->cbxCirclesAsDrills->setChecked(
        clientSettings.value(settingsPrefix % "/circles_as_drills", false)
            .toBool());
    mUi->edtLineWidth->setValue(UnsignedLength(Length::fromMm(
        clientSettings.value(settingsPrefix % "/line_width", "0").toString())));
    mUi->spbxScaleFactor->setValue(
        clientSettings.value(settingsPrefix % "/scale_factor", "1").toDouble());
    mUi->cbxInteractivePlacement->setChecked(
        clientSettings.value(settingsPrefix % "/interactive_placement", true)
            .toBool());
    mUi->edtPosX->setValue(Length::fromMm(
        clientSettings.value(settingsPrefix % "/pos_x", "0").toString()));
    mUi->edtPosY->setValue(Length::fromMm(
        clientSettings.value(settingsPrefix % "/pos_y", "0").toString()));
    restoreGeometry(clientSettings.value(settingsPrefix % "/window_geometry")
                        .toByteArray());
  } catch (const Exception& e) {
    qCritical() << "Error while initializing DXF import dialog:" << e.getMsg();
  }
}

DxfImportDialog::~DxfImportDialog() noexcept {
  // Save the values and window geometry.
  QSettings clientSettings;
  if (auto layerName = mUi->cbxLayer->getCurrentLayerName()) {
    clientSettings.setValue(mSettingsPrefix % "/layer", **layerName);
  }
  clientSettings.setValue(mSettingsPrefix % "/circles_as_drills",
                          mUi->cbxCirclesAsDrills->isChecked());
  clientSettings.setValue(mSettingsPrefix % "/line_width",
                          mUi->edtLineWidth->getValue()->toMmString());
  clientSettings.setValue(mSettingsPrefix % "/scale_factor",
                          mUi->spbxScaleFactor->value());
  clientSettings.setValue(mSettingsPrefix % "/interactive_placement",
                          mUi->cbxInteractivePlacement->isChecked());
  clientSettings.setValue(mSettingsPrefix % "/pos_x",
                          mUi->edtPosX->getValue().toMmString());
  clientSettings.setValue(mSettingsPrefix % "/pos_y",
                          mUi->edtPosY->getValue().toMmString());
  clientSettings.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

GraphicsLayerName DxfImportDialog::getLayerName() const noexcept {
  if (auto layer = mUi->cbxLayer->getCurrentLayerName()) {
    return *layer;
  } else {
    return mDefaultLayer;
  }
}

bool DxfImportDialog::getImportCirclesAsDrills() const noexcept {
  return mUi->cbxCirclesAsDrills->isChecked();
}

UnsignedLength DxfImportDialog::getLineWidth() const noexcept {
  return mUi->edtLineWidth->getValue();
}

qreal DxfImportDialog::getScaleFactor() const noexcept {
  return mUi->spbxScaleFactor->value();
}

tl::optional<Point> DxfImportDialog::getPlacementPosition() const noexcept {
  if (mUi->cbxInteractivePlacement->isChecked()) {
    return tl::nullopt;
  } else {
    return Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue());
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath DxfImportDialog::chooseFile() const noexcept {
  QSettings clientSettings;
  QString key = mSettingsPrefix % "/file";
  QString selectedFile = clientSettings.value(key, QDir::homePath()).toString();
  FilePath fp(FileDialog::getOpenFileName(parentWidget(), tr("Choose file"),
                                          selectedFile, "*.dxf;;*"));
  if (fp.isValid()) {
    clientSettings.setValue(key, fp.toStr());
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

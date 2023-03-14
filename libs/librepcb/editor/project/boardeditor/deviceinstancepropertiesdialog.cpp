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
#include "deviceinstancepropertiesdialog.h"

#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../project/cmd/cmddeviceinstanceeditall.h"
#include "../../undostack.h"
#include "../../workspace/desktopservices.h"
#include "ui_deviceinstancepropertiesdialog.h"

#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>

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

DeviceInstancePropertiesDialog::DeviceInstancePropertiesDialog(
    const WorkspaceSettings& settings, Project& project, BI_Device& device,
    UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mSettings(settings),
    mProject(project),
    mDevice(device),
    mUndoStack(undoStack),
    mAttributes(mDevice.getComponentInstance().getAttributes()),
    mUi(new Ui::DeviceInstancePropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &DeviceInstancePropertiesDialog::buttonBoxClicked);
  setWindowTitle(
      tr("Properties of %1").arg(*mDevice.getComponentInstance().getName()));

  // Component Instance Attributes
  ComponentInstance& cmp = mDevice.getComponentInstance();
  mUi->edtCompInstName->setText(*cmp.getName());
  mUi->edtCompInstValue->setText(cmp.getValue());
  mUi->attributeListEditorWidget->setReferences(nullptr, &mAttributes);

  const QStringList& localeOrder = mProject.getLocaleOrder();

  // Library Element Information
  QString htmlLink("<a href=\"%1\">%2<a>");
  mUi->lblLibDeviceName->setText(
      htmlLink.arg(mDevice.getLibDevice().getDirectory().getAbsPath().toStr(),
                   *mDevice.getLibDevice().getNames().value(localeOrder)));
  mUi->lblLibDeviceName->setToolTip(
      mDevice.getLibDevice().getDescriptions().value(localeOrder) + "<p>" +
      mDevice.getLibDevice().getDirectory().getAbsPath().toNative());
  connect(mUi->lblLibDeviceName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mSettings, this);
            ds.openLocalPath(FilePath(url));
          });

  mUi->lblLibPackageName->setText(
      htmlLink.arg(mDevice.getLibPackage().getDirectory().getAbsPath().toStr(),
                   *mDevice.getLibPackage().getNames().value(localeOrder)));
  mUi->lblLibPackageName->setToolTip(
      mDevice.getLibPackage().getDescriptions().value(localeOrder) + "<p>" +
      mDevice.getLibPackage().getDirectory().getAbsPath().toNative());
  connect(mUi->lblLibPackageName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mSettings, this);
            ds.openLocalPath(FilePath(url));
          });

  mUi->lblLibFootprintName->setText(
      *mDevice.getLibFootprint().getNames().value(localeOrder));
  mUi->lblLibFootprintName->setToolTip(
      mDevice.getLibFootprint().getDescriptions().value(localeOrder));

  // Device/Footprint Attributes
  mUi->edtPosX->setValue(mDevice.getPosition().getX());
  mUi->edtPosY->setValue(mDevice.getPosition().getY());
  mUi->edtRotation->setValue(mDevice.getRotation());
  mUi->cbxMirror->setChecked(mDevice.getMirrored());

  // set focus to component instance name
  mUi->edtCompInstName->selectAll();
  mUi->edtCompInstName->setFocus();
}

DeviceInstancePropertiesDialog::~DeviceInstancePropertiesDialog() noexcept {
  mUi->attributeListEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DeviceInstancePropertiesDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      accept();
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

void DeviceInstancePropertiesDialog::keyPressEvent(QKeyEvent* e) noexcept {
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

void DeviceInstancePropertiesDialog::accept() noexcept {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool DeviceInstancePropertiesDialog::applyChanges() noexcept {
  try {
    UndoStackTransaction transaction(
        mUndoStack,
        tr("Change properties of %1")
            .arg(*mDevice.getComponentInstance().getName()));

    // Component Instance
    QScopedPointer<CmdComponentInstanceEdit> cmdCmp(
        new CmdComponentInstanceEdit(mProject.getCircuit(),
                                     mDevice.getComponentInstance()));
    cmdCmp->setName(CircuitIdentifier(
        mUi->edtCompInstName->text().trimmed()));  // can throw
    cmdCmp->setValue(mUi->edtCompInstValue->toPlainText());
    cmdCmp->setAttributes(mAttributes);
    transaction.append(cmdCmp.take());  // can throw

    // Device Instance
    QScopedPointer<CmdDeviceInstanceEditAll> cmdDev(
        new CmdDeviceInstanceEditAll(mDevice));
    cmdDev->setPosition(
        Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()), false);
    cmdDev->setRotation(mUi->edtRotation->getValue(), false);
    cmdDev->setMirrored(mUi->cbxMirror->isChecked(), false);  // can throw
    transaction.append(cmdDev.take());  // can throw

    transaction.commit();  // can throw
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

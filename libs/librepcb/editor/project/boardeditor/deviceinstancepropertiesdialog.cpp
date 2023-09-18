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

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/workspace.h>

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
    const Workspace& ws, Project& project, BI_Device& device,
    UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mProject(project),
    mDevice(device),
    mUndoStack(undoStack),
    mAttributes(mDevice.getComponentInstance().getAttributes()),
    mUi(new Ui::DeviceInstancePropertiesDialog) {
  mUi->setupUi(this);
  mUi->attributeListEditorWidget->setFrameStyle(QFrame::NoFrame);
  mUi->assemblyOptionListEditorWidget->setFrameStyle(QFrame::NoFrame);
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
  mUi->assemblyOptionListEditorWidget->setReferences(&mWorkspace, &mProject,
                                                     &cmp);
  setSelectedPart(nullptr);
  connect(mUi->assemblyOptionListEditorWidget,
          &ComponentAssemblyOptionListEditorWidget::selectedPartChanged, this,
          &DeviceInstancePropertiesDialog::setSelectedPart);

  const QStringList& localeOrder = mProject.getLocaleOrder();

  // Library Element Information
  QString htmlLink("<a href=\"%1\">%2<a>");
  mUi->lblLibDeviceName->setText(
      htmlLink.arg(mDevice.getLibDevice().getDirectory().getAbsPath().toStr(),
                   *mDevice.getLibDevice().getNames().value(localeOrder)));
  mUi->lblLibDeviceName->setToolTip(
      (mDevice.getLibDevice().getDescriptions().value(localeOrder) + "\n\n" +
       mDevice.getLibDevice().getDirectory().getAbsPath().toNative())
          .trimmed());
  connect(mUi->lblLibDeviceName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openLocalPath(FilePath(url));
          });

  mUi->lblLibPackageName->setText(
      htmlLink.arg(mDevice.getLibPackage().getDirectory().getAbsPath().toStr(),
                   *mDevice.getLibPackage().getNames().value(localeOrder)) %
      QString(" (%1 \"%2\")")
          .arg(tr("Footprint"))
          .arg(*mDevice.getLibFootprint().getNames().value(localeOrder)));
  mUi->lblLibPackageName->setToolTip(
      (mDevice.getLibPackage().getDescriptions().value(localeOrder) + "\n\n" +
       mDevice.getLibPackage().getDirectory().getAbsPath().toNative())
          .trimmed());
  connect(mUi->lblLibPackageName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openLocalPath(FilePath(url));
          });

  // Device/Footprint Attributes
  mUi->edtPosX->setValue(mDevice.getPosition().getX());
  mUi->edtPosY->setValue(mDevice.getPosition().getY());
  mUi->edtRotation->setValue(mDevice.getRotation());
  mUi->cbxMirror->setChecked(mDevice.getMirrored());
  mUi->cbxLock->setChecked(mDevice.isLocked());

  // set focus to component instance name
  mUi->edtCompInstName->selectAll();
  mUi->edtCompInstName->setFocus();
}

DeviceInstancePropertiesDialog::~DeviceInstancePropertiesDialog() noexcept {
  mUi->assemblyOptionListEditorWidget->setReferences(nullptr, nullptr, nullptr);
  mUi->attributeListEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DeviceInstancePropertiesDialog::setSelectedPart(
    std::shared_ptr<Part> part) noexcept {
  if (part) {
    mUi->attributeListEditorWidget->setReferences(nullptr,
                                                  &part->getAttributes());
    mUi->gbxAttributes->setTitle(tr("Attributes of Selected Part"));
  } else {
    mUi->attributeListEditorWidget->setReferences(nullptr, &mAttributes);
    mUi->gbxAttributes->setTitle(tr("Attributes of Component"));
  }
  mSelectedPart = part;  // Keep attribute list in memory!!!
}

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

    // Component instance properties.
    QScopedPointer<CmdComponentInstanceEdit> cmdCmp(
        new CmdComponentInstanceEdit(mProject.getCircuit(),
                                     mDevice.getComponentInstance()));
    cmdCmp->setName(CircuitIdentifier(
        mUi->edtCompInstName->text().trimmed()));  // can throw
    cmdCmp->setValue(mUi->edtCompInstValue->toPlainText());
    cmdCmp->setAttributes(mAttributes);
    cmdCmp->setAssemblyOptions(
        mUi->assemblyOptionListEditorWidget->getOptions());
    transaction.append(cmdCmp.take());  // can throw

    // Device instance with associated elements.
    QScopedPointer<CmdDeviceInstanceEditAll> cmdDevAll(
        new CmdDeviceInstanceEditAll(mDevice));
    cmdDevAll->setPosition(
        Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()), false);
    cmdDevAll->setRotation(mUi->edtRotation->getValue(), false);
    cmdDevAll->setMirrored(mUi->cbxMirror->isChecked(),
                           mDevice.getBoard().getInnerLayerCount(),
                           false);  // can throw
    if (mUi->cbxLock->isChecked() != mDevice.isLocked()) {
      // Do not apply to all elements if not modified!
      cmdDevAll->setLocked(mUi->cbxLock->isChecked());
    }
    transaction.append(cmdDevAll.take());  // can throw

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

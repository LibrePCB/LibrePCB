/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "newelementwizardpage_deviceproperties.h"
#include "ui_newelementwizardpage_deviceproperties.h"
#include <librepcb/library/pkg/package.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include "../common/componentchooserdialog.h"
#include "../common/packagechooserdialog.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NewElementWizardPage_DeviceProperties::NewElementWizardPage_DeviceProperties(NewElementWizardContext& context, QWidget *parent) noexcept :
    QWizardPage(parent), mContext(context), mUi(new Ui::NewElementWizardPage_DeviceProperties)
{
    mUi->setupUi(this);
    connect(mUi->edtComponentUuid, &QLineEdit::textChanged,
            this, &NewElementWizardPage_DeviceProperties::edtComponentUuidTextChanged);
    connect(mUi->edtPackageUuid, &QLineEdit::textChanged,
            this, &NewElementWizardPage_DeviceProperties::edtPackageUuidTextChanged);
    connect(mUi->btnChooseComponent, &QToolButton::clicked,
            this, &NewElementWizardPage_DeviceProperties::btnChooseComponentClicked);
    connect(mUi->btnChoosePackage, &QToolButton::clicked,
            this, &NewElementWizardPage_DeviceProperties::btnChoosePackageClicked);
}

NewElementWizardPage_DeviceProperties::~NewElementWizardPage_DeviceProperties() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool NewElementWizardPage_DeviceProperties::isComplete() const noexcept
{
    if (mContext.mDeviceComponentUuid.isNull()) return false;
    if (mContext.mDevicePackageUuid.isNull()) return false;
    return true;
}

int NewElementWizardPage_DeviceProperties::nextId() const noexcept
{
    return NewElementWizardContext::ID_None;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void NewElementWizardPage_DeviceProperties::edtComponentUuidTextChanged(const QString& text) noexcept
{
    setComponent(Uuid(text.trimmed()));
}

void NewElementWizardPage_DeviceProperties::edtPackageUuidTextChanged(const QString& text) noexcept
{
    setPackage(Uuid(text.trimmed()));
}

void NewElementWizardPage_DeviceProperties::btnChooseComponentClicked() noexcept
{
    ComponentChooserDialog dialog(mContext.getWorkspace(), &mContext.getLayerProvider(), this);
    if (dialog.exec() == QDialog::Accepted) {
        mUi->edtComponentUuid->setText(dialog.getSelectedComponentUuid().toStr());
    }
}

void NewElementWizardPage_DeviceProperties::btnChoosePackageClicked() noexcept
{
    PackageChooserDialog dialog(mContext.getWorkspace(), &mContext.getLayerProvider(), this);
    if (dialog.exec() == QDialog::Accepted) {
        mUi->edtPackageUuid->setText(dialog.getSelectedPackageUuid().toStr());
    }
}

void NewElementWizardPage_DeviceProperties::setComponent(const Uuid& uuid) noexcept
{
    mContext.mDeviceComponentUuid = uuid;
    if (!uuid.isNull()) {
        try {
            FilePath fp = mContext.getWorkspace().getLibraryDb().getLatestComponent(uuid); // can throw
            QString name, desc;
            mContext.getWorkspace().getLibraryDb().getElementTranslations<Component>(
                fp, mContext.getLibLocaleOrder(), &name, &desc); // can throw
            mUi->lblComponentName->setText(name);
            mUi->lblComponentDescription->setText(desc);
        } catch (const Exception& e) {
            mUi->lblComponentName->setText(tr("ERROR:"));
            mUi->lblComponentDescription->setText(e.getMsg());
        }
    } else {
        mUi->lblComponentName->setText(tr("No component selected"));
        mUi->lblComponentDescription->clear();
    }
    completeChanged();
}

void NewElementWizardPage_DeviceProperties::setPackage(const Uuid& uuid) noexcept
{
    mContext.mDevicePackageUuid = uuid;
    if (!uuid.isNull()) {
        try {
            FilePath fp = mContext.getWorkspace().getLibraryDb().getLatestPackage(uuid); // can throw
            Package package(fp, true); // can throw
            DevicePadSignalMapHelpers::setPads(mContext.mDevicePadSignalMap,
                                               package.getPads().getUuidSet());
            mUi->lblPackageName->setText(package.getNames().value(mContext.getLibLocaleOrder()));
            mUi->lblPackageDescription->setText(package.getDescriptions().value(mContext.getLibLocaleOrder()));
        } catch (const Exception& e) {
            mUi->lblPackageName->setText(tr("ERROR:"));
            mUi->lblPackageDescription->setText(e.getMsg());
            mContext.mDevicePackageUuid = Uuid(); // invalid package!
        }
    } else {
        mUi->lblPackageName->setText(tr("No package selected"));
        mUi->lblPackageDescription->clear();
    }
    completeChanged();
}

void NewElementWizardPage_DeviceProperties::initializePage() noexcept
{
    QWizardPage::initializePage();
    mUi->edtComponentUuid->setText(mContext.mDeviceComponentUuid.toStr());
    mUi->edtPackageUuid->setText(mContext.mDevicePackageUuid.toStr());
    setComponent(mContext.mDeviceComponentUuid);
    setPackage(mContext.mDevicePackageUuid);
}

void NewElementWizardPage_DeviceProperties::cleanupPage() noexcept
{
    QWizardPage::cleanupPage();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

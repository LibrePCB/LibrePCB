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
#include <QtCore>
#include <QtWidgets>
#include "deviceeditorwidget.h"
#include "ui_deviceeditorwidget.h"
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolpreviewgraphicsitem.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/dev/cmd/cmddeviceedit.h>
#include <librepcb/library/dev/cmd/cmddevicepadsignalmapitemedit.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/pkg/footprintpreviewgraphicsitem.h>
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

DeviceEditorWidget::DeviceEditorWidget(const Context& context, const FilePath& fp,
                                       QWidget* parent) :
    EditorWidgetBase(context, fp, parent), mUi(new Ui::DeviceEditorWidget)
{
    mUi->setupUi(this);
    setWindowIcon(QIcon(":/img/library/device.png"));

    // show graphics scenes
    mComponentGraphicsScene.reset(new GraphicsScene());
    mPackageGraphicsScene.reset(new GraphicsScene());
    mUi->viewComponent->setScene(mComponentGraphicsScene.data());
    mUi->viewPackage->setScene(mPackageGraphicsScene.data());
    mUi->viewPackage->setBackgroundBrush(Qt::black);

    // insert category list editor widget
    mCategoriesEditorWidget.reset(new ComponentCategoryListEditorWidget(mContext.workspace, this));
    int row;
    QFormLayout::ItemRole role;
    mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
    mUi->formLayout->setWidget(row, QFormLayout::FieldRole, mCategoriesEditorWidget.data());

    mDevice.reset(new Device(fp, false)); // can throw
    setWindowTitle(mDevice->getNames().value(getLibLocaleOrder()));
    mUi->lblUuid->setText(QString("<a href=\"%1\">%2</a>").arg(
        mDevice->getFilePath().toQUrl().toString(), mDevice->getUuid().toStr()));
    mUi->lblUuid->setToolTip(mDevice->getFilePath().toNative());
    mUi->edtName->setText(mDevice->getNames().value(getLibLocaleOrder()));
    mUi->edtDescription->setPlainText(mDevice->getDescriptions().value(getLibLocaleOrder()));
    mUi->edtKeywords->setText(mDevice->getKeywords().value(getLibLocaleOrder()));
    mUi->edtAuthor->setText(mDevice->getAuthor());
    mUi->edtVersion->setText(mDevice->getVersion().toStr());
    mCategoriesEditorWidget->setUuids(mDevice->getCategories());
    mUi->cbxDeprecated->setChecked(mDevice->isDeprecated());
    mUi->padSignalMapEditorWidget->setReferences(mUndoStack.data(), &mDevice->getPadSignalMap());
    updateDeviceComponentUuid(mDevice->getComponentUuid());
    updateDevicePackageUuid(mDevice->getPackageUuid());

    // show "interface broken" warning when related properties are modified
    memorizeDeviceInterface();
    setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

    // set dirty state when properties are modified
    connect(mUi->edtName, &QLineEdit::textEdited, this, &DeviceEditorWidget::setDirty);
    connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this, &DeviceEditorWidget::setDirty);
    connect(mUi->edtKeywords, &QLineEdit::textEdited, this, &DeviceEditorWidget::setDirty);
    connect(mUi->edtAuthor, &QLineEdit::textEdited, this, &DeviceEditorWidget::setDirty);
    connect(mUi->edtVersion, &QLineEdit::textEdited, this, &DeviceEditorWidget::setDirty);
    connect(mUi->cbxDeprecated, &QCheckBox::clicked, this, &DeviceEditorWidget::setDirty);
    connect(mCategoriesEditorWidget.data(), &ComponentCategoryListEditorWidget::categoryAdded,
            this, &DeviceEditorWidget::setDirty);
    connect(mCategoriesEditorWidget.data(), &ComponentCategoryListEditorWidget::categoryRemoved,
            this, &DeviceEditorWidget::setDirty);

    connect(mDevice.data(), &Device::componentUuidChanged,
            this, &DeviceEditorWidget::updateDeviceComponentUuid);
    connect(mDevice.data(), &Device::packageUuidChanged,
            this, &DeviceEditorWidget::updateDevicePackageUuid);
    connect(mUi->btnChooseComponent, &QToolButton::clicked,
            this, &DeviceEditorWidget::btnChooseComponentClicked);
    connect(mUi->btnChoosePackage, &QToolButton::clicked,
            this, &DeviceEditorWidget::btnChoosePackageClicked);
}

DeviceEditorWidget::~DeviceEditorWidget() noexcept
{
    mUi->padSignalMapEditorWidget->setReferences(nullptr, nullptr);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

bool DeviceEditorWidget::save() noexcept
{
    try {
        QString name = mUi->edtName->text().trimmed();
        if (name.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__, tr("The name must not be empty."));
        }
        Version version(mUi->edtVersion->text().trimmed());
        if (!version.isValid()) {
            throw RuntimeError(__FILE__, __LINE__, tr("The version number is invalid."));
        }

        mDevice->setName("", name);
        mDevice->setDescription("", mUi->edtDescription->toPlainText().trimmed());
        mDevice->setKeywords("", mUi->edtKeywords->text().trimmed());
        mDevice->setAuthor(mUi->edtAuthor->text().trimmed());
        mDevice->setVersion(version);
        mDevice->setCategories(mCategoriesEditorWidget->getUuids());
        mDevice->setDeprecated(mUi->cbxDeprecated->isChecked());
        mDevice->save();
        memorizeDeviceInterface();
        return EditorWidgetBase::save();
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Save failed"), e.getMsg());
        return false;
    }
}

bool DeviceEditorWidget::zoomIn() noexcept
{
    mUi->viewComponent->zoomIn();
    mUi->viewPackage->zoomIn();
    return true;
}

bool DeviceEditorWidget::zoomOut() noexcept
{
    mUi->viewComponent->zoomOut();
    mUi->viewPackage->zoomOut();
    return true;
}

bool DeviceEditorWidget::zoomAll() noexcept
{
    mUi->viewComponent->zoomAll();
    mUi->viewPackage->zoomAll();
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void DeviceEditorWidget::btnChooseComponentClicked() noexcept
{
    ComponentChooserDialog dialog(mContext.workspace, &mContext.layerProvider, this);
    if (dialog.exec() == QDialog::Accepted) {
        Uuid cmpUuid = dialog.getSelectedComponentUuid();
        if (cmpUuid != mDevice->getComponentUuid()) {
            try {
                // load component
                FilePath fp = mContext.workspace.getLibraryDb().getLatestComponent(cmpUuid); // can throw
                if (!fp.isValid()) {
                    throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
                }
                Component component(fp, true); // can throw

                // edit device
                QScopedPointer<UndoCommandGroup> cmdGroup(new UndoCommandGroup(
                                                              tr("Change component")));
                QScopedPointer<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
                cmdDevEdit->setComponentUuid(cmpUuid);
                cmdGroup->appendChild(cmdDevEdit.take());
                for (DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
                    if (!component.getSignals().contains(item.getSignalUuid())) {
                        QScopedPointer<CmdDevicePadSignalMapItemEdit> cmdItem(
                            new CmdDevicePadSignalMapItemEdit(item));
                        cmdItem->setSignalUuid(Uuid());
                        cmdGroup->appendChild(cmdItem.take());
                    }
                }
                mUndoStack->execCmd(cmdGroup.take());
            } catch (const Exception& e) {
                QMessageBox::critical(this, tr("Could not set component"), e.getMsg());
            }
        }
    }
}

void DeviceEditorWidget::btnChoosePackageClicked() noexcept
{
    PackageChooserDialog dialog(mContext.workspace, &mContext.layerProvider, this);
    if (dialog.exec() == QDialog::Accepted) {
        Uuid pkgUuid = dialog.getSelectedPackageUuid();
        if (pkgUuid != mDevice->getPackageUuid()) {
            try {
                // load package
                FilePath fp = mContext.workspace.getLibraryDb().getLatestPackage(pkgUuid); // can throw
                if (!fp.isValid()) {
                    throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
                }
                Package package(fp, true); // can throw
                QSet<Uuid> pads = package.getPads().getUuidSet();

                // edit device
                QScopedPointer<UndoCommandGroup> cmdGroup(new UndoCommandGroup(
                                                              tr("Change package")));
                QScopedPointer<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
                cmdDevEdit->setPackageUuid(dialog.getSelectedPackageUuid());
                cmdGroup->appendChild(cmdDevEdit.take());
                for (const DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
                    if (!pads.contains(item.getPadUuid())) {
                        cmdGroup->appendChild(new CmdDevicePadSignalMapItemRemove(
                            mDevice->getPadSignalMap(), &item));
                    }
                }
                foreach (const Uuid& pad, pads - mDevice->getPadSignalMap().getUuidSet()) {
                    cmdGroup->appendChild(new CmdDevicePadSignalMapItemInsert(
                        mDevice->getPadSignalMap(),
                        std::make_shared<DevicePadSignalMapItem>(pad, Uuid())));
                }
                mUndoStack->execCmd(cmdGroup.take());
                Q_ASSERT(mDevice->getPadSignalMap().getUuidSet() == pads);
            } catch (const Exception& e) {
                QMessageBox::critical(this, tr("Could not set package"), e.getMsg());
            }
        }
    }
}

void DeviceEditorWidget::updateDeviceComponentUuid(const Uuid& uuid) noexcept
{
    mSymbolGraphicsItems.clear();
    mSymbols.clear();
    mUi->lblComponentUuid->setText(uuid.toStr());
    try {
        FilePath fp = mContext.workspace.getLibraryDb().getLatestComponent(uuid); // can throw
        if (!fp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
        }
        mComponent.reset(new Component(fp, true)); // can throw
        mUi->padSignalMapEditorWidget->setSignalList(mComponent->getSignals());
        mUi->lblComponentName->setText(mComponent->getNames().value(getLibLocaleOrder()));
        mUi->lblComponentName->setStyleSheet("");
        updateComponentPreview();
    } catch (const Exception& e) {
        mUi->padSignalMapEditorWidget->setSignalList(ComponentSignalList());
        mUi->lblComponentName->setText(e.getMsg());
        mUi->lblComponentName->setStyleSheet("color: red;");
    }
}

void DeviceEditorWidget::updateComponentPreview() noexcept
{
    if (mComponent && mComponent->getSymbolVariants().count() > 0) {
        const ComponentSymbolVariant& symbVar = *mComponent->getSymbolVariants().first();
        for (const ComponentSymbolVariantItem& item : symbVar.getSymbolItems()) {
            try {
                FilePath fp = mContext.workspace.getLibraryDb().getLatestSymbol(item.getSymbolUuid()); // can throw
                std::shared_ptr<Symbol> sym = std::make_shared<Symbol>(fp, true); // can throw
                mSymbols.append(sym);
                std::shared_ptr<SymbolPreviewGraphicsItem> graphicsItem =
                    std::make_shared<SymbolPreviewGraphicsItem>(
                            mContext.layerProvider, QStringList(), *sym,
                            mComponent.data(), symbVar.getUuid(), item.getUuid());
                graphicsItem->setPos(item.getSymbolPosition().toPxQPointF());
                graphicsItem->setRotation(-item.getSymbolRotation().toDeg());
                mComponentGraphicsScene->addItem(*graphicsItem);
                mSymbolGraphicsItems.append(graphicsItem);
            } catch (const Exception& e) {
                // what could we do here? ;)
            }
        }
        mUi->viewComponent->zoomAll();
    }
}

void DeviceEditorWidget::updateDevicePackageUuid(const Uuid& uuid) noexcept
{
    mFootprintGraphicsItem.reset();
    mUi->lblPackageUuid->setText(uuid.toStr());
    try {
        FilePath fp = mContext.workspace.getLibraryDb().getLatestPackage(uuid); // can throw
        if (!fp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
        }
        mPackage.reset(new Package(fp, true)); // can throw
        mUi->padSignalMapEditorWidget->setPadList(mPackage->getPads());
        mUi->lblPackageName->setText(mPackage->getNames().value(getLibLocaleOrder()));
        mUi->lblPackageName->setStyleSheet("");
        updatePackagePreview();
    } catch (const Exception& e) {
        mUi->padSignalMapEditorWidget->setPadList(PackagePadList());
        mUi->lblPackageName->setText(e.getMsg());
        mUi->lblPackageName->setStyleSheet("color: red;");
    }
}

void DeviceEditorWidget::updatePackagePreview() noexcept
{
    if (mPackage && mPackage->getFootprints().count() > 0) {
        mFootprintGraphicsItem.reset(new FootprintPreviewGraphicsItem(
            mContext.layerProvider, QStringList(), *mPackage->getFootprints().first(),
            mPackage.data(), mComponent.data()));
        mPackageGraphicsScene->addItem(*mFootprintGraphicsItem);
        mUi->viewPackage->zoomAll();
    }
}

void DeviceEditorWidget::memorizeDeviceInterface() noexcept
{
    mOriginalComponentUuid = mDevice->getComponentUuid();
    mOriginalPackageUuid = mDevice->getPackageUuid();
    mOriginalPadSignalMap = mDevice->getPadSignalMap();
}

bool DeviceEditorWidget::isInterfaceBroken() const noexcept
{
    if (mDevice->getComponentUuid() != mOriginalComponentUuid)  return true;
    if (mDevice->getPackageUuid() != mOriginalPackageUuid)      return true;
    if (mDevice->getPadSignalMap() != mOriginalPadSignalMap)    return true;
    return false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

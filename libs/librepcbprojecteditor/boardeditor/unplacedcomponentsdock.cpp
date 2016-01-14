/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "unplacedcomponentsdock.h"
#include "ui_unplacedcomponentsdock.h"
#include <librepcbproject/boards/board.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/project.h>
#include <librepcbproject/settings/projectsettings.h>
#include <librepcbproject/circuit/componentinstance.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/elements.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbcommon/graphics/graphicsscene.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceadd.h>
#include <librepcbcommon/undostack.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbworkspace/workspace.h>
#include "../projecteditor.h"
#include <librepcbproject/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcblibrary/pkg/footprintpreviewgraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UnplacedComponentsDock::UnplacedComponentsDock(ProjectEditor& editor) :
    QDockWidget(0), mProjectEditor(editor), mProject(editor.getProject()), mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mFootprintPreviewGraphicsScene(nullptr), mFootprintPreviewGraphicsItem(nullptr),
    mSelectedComponent(nullptr), mSelectedDevice(nullptr), mSelectedPackage(nullptr),
    mSelectedFootprintUuid(), mCircuitConnection1(), mCircuitConnection2(),
    mBoardConnection1(), mBoardConnection2(), mDisableListUpdate(false)
{
    mUi->setupUi(this);
    mFootprintPreviewGraphicsScene = new GraphicsScene();
    mUi->graphicsView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    mUi->graphicsView->setOriginCrossVisible(false);
    mUi->graphicsView->setScene(mFootprintPreviewGraphicsScene);

    QSettings clientSettings;
    mUi->splitter->restoreState(clientSettings.value("unplaced_components_dock/splitter_state").toByteArray());

    mCircuitConnection1 = connect(&mProject.getCircuit(), &Circuit::componentAdded,
                                  [this](ComponentInstance& cmp){Q_UNUSED(cmp); updateComponentsList();});
    mCircuitConnection2 = connect(&mProject.getCircuit(), &Circuit::componentRemoved,
                                  [this](ComponentInstance& cmp){Q_UNUSED(cmp); updateComponentsList();});

    updateComponentsList();
}

UnplacedComponentsDock::~UnplacedComponentsDock()
{
    QSettings clientSettings;
    clientSettings.setValue("unplaced_components_dock/splitter_state", mUi->splitter->saveState());

    setBoard(nullptr);
    mDisableListUpdate = true;
    disconnect(mCircuitConnection1);        mCircuitConnection1 = QMetaObject::Connection();
    disconnect(mCircuitConnection2);        mCircuitConnection2 = QMetaObject::Connection();
    delete mFootprintPreviewGraphicsItem;   mFootprintPreviewGraphicsItem = nullptr;
    delete mFootprintPreviewGraphicsScene;  mFootprintPreviewGraphicsScene = nullptr;
    delete mUi;                             mUi = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void UnplacedComponentsDock::setBoard(Board* board)
{
    // clean up
    mBoard = nullptr;
    disconnect(mBoardConnection1);  mBoardConnection1 = QMetaObject::Connection();
    disconnect(mBoardConnection2);  mBoardConnection2 = QMetaObject::Connection();
    updateComponentsList();

    // load new board
    mBoard = board;
    if (board)
    {
        mBoardConnection1 = connect(board, &Board::deviceAdded, [this](DeviceInstance& c){Q_UNUSED(c); updateComponentsList();});
        mBoardConnection2 = connect(board, &Board::deviceRemoved, [this](DeviceInstance& c){Q_UNUSED(c); updateComponentsList();});
        mNextPosition = Point::fromMm(0, -20).mappedToGrid(board->getGridProperties().getInterval());
        updateComponentsList();
    }
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void UnplacedComponentsDock::on_lstUnplacedComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    ComponentInstance* component = nullptr;
    if (mBoard && current)
    {
        Uuid cmpUuid(current->data(Qt::UserRole).toString());
        Q_ASSERT(cmpUuid.isNull() == false);
        component = mProject.getCircuit().getComponentInstanceByUuid(cmpUuid);
    }
    setSelectedComponentInstance(component);
}

void UnplacedComponentsDock::on_cbxSelectedDevice_currentIndexChanged(int index)
{
    Uuid deviceUuid(mUi->cbxSelectedDevice->itemData(index, Qt::UserRole).toString());
    FilePath devFp = mProjectEditor.getWorkspace().getLibrary().getLatestDevice(deviceUuid);
    if (devFp.isValid()) {
        const library::Device* device = new library::Device(devFp);
        FilePath pkgFp = mProjectEditor.getWorkspace().getLibrary().getLatestPackage(device->getPackageUuid());
        if (pkgFp.isValid()) {
            const library::Package* package = new library::Package(pkgFp);
            setSelectedDeviceAndPackage(device, package);
        } else {
            setSelectedDeviceAndPackage(nullptr, nullptr);
        }
    }
    else {
        setSelectedDeviceAndPackage(nullptr, nullptr);
    }
}

void UnplacedComponentsDock::on_cbxSelectedFootprint_currentIndexChanged(int index)
{
    Uuid footprintUuid(mUi->cbxSelectedFootprint->itemData(index, Qt::UserRole).toString());
    setSelectedFootprintUuid(footprintUuid);
}

void UnplacedComponentsDock::on_btnAdd_clicked()
{
    if (mBoard && mSelectedComponent && mSelectedDevice && mSelectedPackage && (!mSelectedFootprintUuid.isNull())) {
        addDevice(*mSelectedComponent, mSelectedDevice->getUuid(), mSelectedFootprintUuid);
    }
    updateComponentsList();
}

void UnplacedComponentsDock::on_pushButton_clicked()
{
    if ((!mBoard) || (!mSelectedComponent) || (!mSelectedDevice) || (!mSelectedPackage) || (mSelectedFootprintUuid.isNull())) return;

    Uuid componentLibUuid = mSelectedComponent->getLibComponent().getUuid();
    Uuid deviceLibUuid = mSelectedDevice->getUuid();

    mDisableListUpdate = true;
    for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++)
    {
        Uuid componentUuid(mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
        Q_ASSERT(componentUuid.isNull() == false);
        ComponentInstance* component = mProject.getCircuit().getComponentInstanceByUuid(componentUuid);
        if (!component) continue;
        if (component->getLibComponent().getUuid() != componentLibUuid) continue;
        addDevice(*component, deviceLibUuid, mSelectedFootprintUuid);
    }
    mDisableListUpdate = false;

    updateComponentsList();
}

void UnplacedComponentsDock::on_btnAddAll_clicked()
{
    if (!mBoard) return;

    mDisableListUpdate = true;
    for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++)
    {
        Uuid componentUuid(mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
        Q_ASSERT(componentUuid.isNull() == false);
        ComponentInstance* component = mProject.getCircuit().getComponentInstanceByUuid(componentUuid);
        if (component)
        {
            QList<Uuid> devices = mProjectEditor.getWorkspace().getLibrary().
                getDevicesOfComponent(component->getLibComponent().getUuid()).toList();
            if (devices.count() > 0)
                addDevice(*component, devices.first(), Uuid());
        }
    }
    mDisableListUpdate = false;

    updateComponentsList();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void UnplacedComponentsDock::updateComponentsList() noexcept
{
    if (mDisableListUpdate) return;

    setSelectedComponentInstance(nullptr);
    mUi->lstUnplacedComponents->clear();

    if (mBoard)
    {
        const QMap<Uuid, ComponentInstance*> componentsList = mProject.getCircuit().getComponentInstances();
        const QMap<Uuid, DeviceInstance*> boardDeviceList = mBoard->getDeviceInstances();
        foreach (ComponentInstance* component, componentsList)
        {
            if (boardDeviceList.contains(component->getUuid())) continue;
            if (component->getLibComponent().isSchematicOnly()) continue;

            // add component to list
            int deviceCount = mProjectEditor.getWorkspace().getLibrary().getDevicesOfComponent(component->getLibComponent().getUuid()).count();
            QString name = component->getName();
            QString value = component->getValue(true).replace("\n", "|");
            QString compName = component->getLibComponent().getName(mProject.getSettings().getLocaleOrder());
            QString text = QString("{%1} %2 (%3) [%4]").arg(deviceCount).arg(name, value, compName);
            QListWidgetItem* item = new QListWidgetItem(text, mUi->lstUnplacedComponents);
            item->setData(Qt::UserRole, component->getUuid().toStr());
        }
    }
}

void UnplacedComponentsDock::setSelectedComponentInstance(ComponentInstance* cmp) noexcept
{
    setSelectedDeviceAndPackage(nullptr, nullptr);
    mUi->cbxSelectedDevice->clear();
    mSelectedComponent = cmp;

    if (mBoard && mSelectedComponent)
    {
        QStringList localeOrder = mProject.getSettings().getLocaleOrder();
        QSet<Uuid> devices = mProjectEditor.getWorkspace().getLibrary().getDevicesOfComponent(mSelectedComponent->getLibComponent().getUuid());
        foreach (const Uuid& deviceUuid, devices)
        {
            // TODO: use library metadata instead of loading the XML files
            FilePath devFp = mProjectEditor.getWorkspace().getLibrary().getLatestDevice(deviceUuid);
            if (!devFp.isValid()) continue;
            const library::Device device(devFp);

            Uuid pkgUuid;
            mProjectEditor.getWorkspace().getLibrary().getDeviceMetadata(devFp, &pkgUuid);
            FilePath pkgFp = mProjectEditor.getWorkspace().getLibrary().getLatestPackage(pkgUuid);
            const library::Package package(pkgFp);

            QString devName = device.getName(localeOrder);
            QString pkgName = package.getName(localeOrder);
            QString text = QString("%1 [%2]").arg(devName, pkgName);
            mUi->cbxSelectedDevice->addItem(text, deviceUuid.toStr());
        }
        if (mUi->cbxSelectedDevice->count() > 0)
            mUi->cbxSelectedDevice->setCurrentIndex(0);
    }
}

void UnplacedComponentsDock::setSelectedDeviceAndPackage(const library::Device* device,
                                                         const library::Package* package) noexcept
{
    setSelectedFootprintUuid(Uuid());
    mUi->cbxSelectedFootprint->clear();
    delete mSelectedPackage;    mSelectedPackage = nullptr;
    delete mSelectedDevice;     mSelectedDevice = nullptr;

    if (mBoard && mSelectedComponent && device && package) {
        if (device->getComponentUuid() == mSelectedComponent->getLibComponent().getUuid()) {
            mSelectedDevice = device;
            mSelectedPackage = package;
            QStringList localeOrder = mProject.getSettings().getLocaleOrder();
            int defaultFootprintIndex = 0;
            foreach (const Uuid& uuid, mSelectedPackage->getFootprintUuids()) {
                const library::Footprint* fpt = mSelectedPackage->getFootprintByUuid(uuid);
                Q_ASSERT(fpt); if (!fpt) continue;
                QString name = fpt->getName(localeOrder);
                if (uuid == mSelectedPackage->getDefaultFootprintUuid()) {
                    name.append(tr(" [default]"));
                    defaultFootprintIndex = mUi->cbxSelectedFootprint->count();
                }
                mUi->cbxSelectedFootprint->addItem(name, uuid.toStr());
            }
            if (mUi->cbxSelectedFootprint->count() > 0)
                mUi->cbxSelectedFootprint->setCurrentIndex(defaultFootprintIndex);
        }
    }
}

void UnplacedComponentsDock::setSelectedFootprintUuid(const Uuid& uuid) noexcept
{
    mUi->btnAdd->setEnabled(false);
    if (mFootprintPreviewGraphicsItem) {
        mFootprintPreviewGraphicsScene->removeItem(*mFootprintPreviewGraphicsItem);
        delete mFootprintPreviewGraphicsItem; mFootprintPreviewGraphicsItem = nullptr;
    }
    mSelectedFootprintUuid = uuid;

    if (mBoard && mSelectedComponent && mSelectedDevice && mSelectedPackage && (!mSelectedFootprintUuid.isNull()))
    {
        const library::Footprint* fpt = mSelectedPackage->getFootprintByUuid(mSelectedFootprintUuid);
        if (fpt) {
            mFootprintPreviewGraphicsItem = new library::FootprintPreviewGraphicsItem(
                mProject, mProject.getSettings().getLocaleOrder(), *fpt, mSelectedPackage,
                &mSelectedComponent->getLibComponent(), mSelectedComponent);
            mFootprintPreviewGraphicsScene->addItem(*mFootprintPreviewGraphicsItem);
            mUi->graphicsView->zoomAll();
            mUi->btnAdd->setEnabled(true);
        }
    }
}

void UnplacedComponentsDock::addDevice(ComponentInstance& cmp, const Uuid& deviceUuid,
                                       Uuid footprintUuid) noexcept
{
    Q_ASSERT(mBoard);
    bool cmdActive = false;

    try
    {
        mProjectEditor.getUndoStack().beginCommand(tr("Add device to board"));
        cmdActive = true;

        const library::Device* device = mProject.getLibrary().getDevice(deviceUuid);
        if (!device)
        {
            // copy the device to the project's library
            FilePath devFp = mProjectEditor.getWorkspace().getLibrary().getLatestDevice(deviceUuid);
            if (!devFp.isValid())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("Device not found in library: %1"))
                    .arg(deviceUuid.toStr()));
            }
            device = new library::Device(devFp);
            auto cmd = new CmdProjectLibraryAddElement<library::Device>(mProject.getLibrary(), *device);
            mProjectEditor.getUndoStack().appendToCommand(cmd);
        }

        const library::Package* pkg = mProject.getLibrary().getPackage(device->getPackageUuid());
        if (!pkg)
        {
            // copy the package to the project's library
            FilePath pkgFp = mProjectEditor.getWorkspace().getLibrary().getLatestPackage(device->getPackageUuid());
            if (!pkgFp.isValid())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("Package not found in library: %1"))
                    .arg(device->getPackageUuid().toStr()));
            }
            pkg = new library::Package(pkgFp);
            auto cmd = new CmdProjectLibraryAddElement<library::Package>(mProject.getLibrary(), *pkg);
            mProjectEditor.getUndoStack().appendToCommand(cmd);
        }

        if (footprintUuid.isNull())
            footprintUuid = pkg->getDefaultFootprintUuid();

        // add device to board
        CmdDeviceInstanceAdd* cmd = new CmdDeviceInstanceAdd(*mBoard, cmp, deviceUuid, footprintUuid, mNextPosition);
        mProjectEditor.getUndoStack().appendToCommand(cmd);
        if (mNextPosition.getX() > Length::fromMm(200))
            mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
        else
            mNextPosition += Point::fromMm(10, 0);
        mNextPosition.mapToGrid(mBoard->getGridProperties().getInterval());

        mProjectEditor.getUndoStack().endCommand();
        cmdActive = false;
    }
    catch (Exception& e)
    {
        try {if (cmdActive) mProjectEditor.getUndoStack().abortCommand();} catch (...) {}
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

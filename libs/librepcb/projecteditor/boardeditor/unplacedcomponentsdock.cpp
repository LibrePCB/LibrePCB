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
#include "unplacedcomponentsdock.h"

#include "../cmd/cmdadddevicetoboard.h"
#include "../projecteditor.h"
#include "ui_unplacedcomponentsdock.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/common/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/elements.h>
#include <librepcb/library/pkg/footprintpreviewgraphicsitem.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/project.h>
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UnplacedComponentsDock::UnplacedComponentsDock(ProjectEditor& editor)
  : QDockWidget(0),
    mProjectEditor(editor),
    mProject(editor.getProject()),
    mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mFootprintPreviewGraphicsScene(nullptr),
    mFootprintPreviewGraphicsItem(nullptr),
    mSelectedComponent(nullptr),
    mSelectedDevice(nullptr),
    mSelectedPackage(nullptr),
    mSelectedFootprintUuid(),
    mCircuitConnection1(),
    mCircuitConnection2(),
    mBoardConnection1(),
    mBoardConnection2(),
    mDisableListUpdate(false) {
  mUi->setupUi(this);
  mFootprintPreviewGraphicsScene = new GraphicsScene();
  mUi->graphicsView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
  mUi->graphicsView->setOriginCrossVisible(false);
  mUi->graphicsView->setScene(mFootprintPreviewGraphicsScene);

  mGraphicsLayerProvider.reset(new DefaultGraphicsLayerProvider());

  QSettings clientSettings;
  mUi->splitter->restoreState(
      clientSettings.value("unplaced_components_dock/splitter_state")
          .toByteArray());

  mCircuitConnection1 =
      connect(&mProject.getCircuit(), &Circuit::componentAdded,
              [this](ComponentInstance& cmp) {
                Q_UNUSED(cmp);
                updateComponentsList();
              });
  mCircuitConnection2 =
      connect(&mProject.getCircuit(), &Circuit::componentRemoved,
              [this](ComponentInstance& cmp) {
                Q_UNUSED(cmp);
                updateComponentsList();
              });

  updateComponentsList();
}

UnplacedComponentsDock::~UnplacedComponentsDock() {
  QSettings clientSettings;
  clientSettings.setValue("unplaced_components_dock/splitter_state",
                          mUi->splitter->saveState());

  setBoard(nullptr);
  mDisableListUpdate = true;
  disconnect(mCircuitConnection1);
  mCircuitConnection1 = QMetaObject::Connection();
  disconnect(mCircuitConnection2);
  mCircuitConnection2 = QMetaObject::Connection();
  delete mFootprintPreviewGraphicsItem;
  mFootprintPreviewGraphicsItem = nullptr;
  delete mFootprintPreviewGraphicsScene;
  mFootprintPreviewGraphicsScene = nullptr;
  delete mUi;
  mUi = nullptr;
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int UnplacedComponentsDock::getUnplacedComponentsCount() const noexcept {
  return mUi->lstUnplacedComponents->count();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UnplacedComponentsDock::setBoard(Board* board) {
  // clean up
  mBoard = nullptr;
  disconnect(mBoardConnection1);
  mBoardConnection1 = QMetaObject::Connection();
  disconnect(mBoardConnection2);
  mBoardConnection2 = QMetaObject::Connection();
  updateComponentsList();

  // load new board
  mBoard = board;
  if (board) {
    mBoardConnection1 =
        connect(board, &Board::deviceAdded, [this](BI_Device& c) {
          Q_UNUSED(c);
          updateComponentsList();
        });
    mBoardConnection2 =
        connect(board, &Board::deviceRemoved, [this](BI_Device& c) {
          Q_UNUSED(c);
          updateComponentsList();
        });
    mNextPosition = Point::fromMm(0, -20).mappedToGrid(
        board->getGridProperties().getInterval());
    updateComponentsList();
  }
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void UnplacedComponentsDock::on_lstUnplacedComponents_currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) {
  Q_UNUSED(previous);

  ComponentInstance* component = nullptr;
  if (mBoard && current) {
    tl::optional<Uuid> cmpUuid =
        Uuid::tryFromString(current->data(Qt::UserRole).toString());
    if (cmpUuid)
      component = mProject.getCircuit().getComponentInstanceByUuid(*cmpUuid);
  }
  setSelectedComponentInstance(component);
}

void UnplacedComponentsDock::on_cbxSelectedDevice_currentIndexChanged(
    int index) {
  try {
    tl::optional<Uuid> deviceUuid = Uuid::tryFromString(
        mUi->cbxSelectedDevice->itemData(index, Qt::UserRole).toString());
    FilePath devFp;
    if (deviceUuid)
      devFp = mProjectEditor.getWorkspace().getLibraryDb().getLatestDevice(
          *deviceUuid);
    if (devFp.isValid()) {
      const library::Device* device = new library::Device(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(devFp))));
      FilePath pkgFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatestPackage(
              device->getPackageUuid());
      if (pkgFp.isValid()) {
        const library::Package* package = new library::Package(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(pkgFp))));
        setSelectedDeviceAndPackage(device, package);
      } else {
        setSelectedDeviceAndPackage(nullptr, nullptr);
      }
    } else {
      setSelectedDeviceAndPackage(nullptr, nullptr);
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void UnplacedComponentsDock::on_cbxSelectedFootprint_currentIndexChanged(
    int index) {
  tl::optional<Uuid> footprintUuid = Uuid::tryFromString(
      mUi->cbxSelectedFootprint->itemData(index, Qt::UserRole).toString());
  setSelectedFootprintUuid(footprintUuid);
}

void UnplacedComponentsDock::on_btnAdd_clicked() {
  if (mBoard && mSelectedComponent && mSelectedDevice && mSelectedPackage &&
      mSelectedFootprintUuid) {
    addDeviceManually(*mSelectedComponent, mSelectedDevice->getUuid(),
                      *mSelectedFootprintUuid);
  }
  updateComponentsList();
}

void UnplacedComponentsDock::on_pushButton_clicked() {
  if ((!mBoard) || (!mSelectedComponent) || (!mSelectedDevice) ||
      (!mSelectedPackage) || (!mSelectedFootprintUuid))
    return;

  Uuid componentLibUuid = mSelectedComponent->getLibComponent().getUuid();
  Uuid deviceLibUuid    = mSelectedDevice->getUuid();

  beginUndoCmdGroup();
  for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++) {
    tl::optional<Uuid> componentUuid = Uuid::tryFromString(
        mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
    if (!componentUuid) continue;
    ComponentInstance* component =
        mProject.getCircuit().getComponentInstanceByUuid(*componentUuid);
    if (!component) continue;
    if (component->getLibComponent().getUuid() != componentLibUuid) continue;
    addNextDeviceToCmdGroup(*component, deviceLibUuid, *mSelectedFootprintUuid);
  }
  commitUndoCmdGroup();

  updateComponentsList();
}

void UnplacedComponentsDock::on_btnAddAll_clicked() {
  if (!mBoard) return;

  beginUndoCmdGroup();
  for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++) {
    tl::optional<Uuid> componentUuid = Uuid::tryFromString(
        mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
    if (!componentUuid) continue;
    ComponentInstance* component =
        mProject.getCircuit().getComponentInstanceByUuid(*componentUuid);
    if (component) {
      try {
        QList<Uuid> devices =
            mProjectEditor.getWorkspace()
                .getLibraryDb()
                .getDevicesOfComponent(component->getLibComponent().getUuid())
                .toList();
        if (devices.count() > 0)
          addNextDeviceToCmdGroup(*component, devices.first(), tl::nullopt);
      } catch (const Exception& e) {
        qCritical() << e.getMsg();
      }
    }
  }
  commitUndoCmdGroup();

  updateComponentsList();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnplacedComponentsDock::updateComponentsList() noexcept {
  if (mDisableListUpdate) return;

  int selectedIndex = mUi->lstUnplacedComponents->currentRow();
  setSelectedComponentInstance(nullptr);
  mUi->lstUnplacedComponents->clear();

  if (mBoard) {
    const QMap<Uuid, ComponentInstance*> componentsList =
        mProject.getCircuit().getComponentInstances();
    const QMap<Uuid, BI_Device*> boardDeviceList = mBoard->getDeviceInstances();
    foreach (ComponentInstance* component, componentsList) {
      if (boardDeviceList.contains(component->getUuid())) continue;
      if (component->getLibComponent().isSchematicOnly()) continue;

      // add component to list
      CircuitIdentifier name  = component->getName();
      QString           value = component->getValue(true).replace("\n", "|");
      ElementName compName    = component->getLibComponent().getNames().value(
          mProject.getSettings().getLocaleOrder());
      QString          text = QString("%1: %2 %3").arg(*name, value, *compName);
      QListWidgetItem* item =
          new QListWidgetItem(text, mUi->lstUnplacedComponents);
      item->setData(Qt::UserRole, component->getUuid().toStr());
    }

    if (mUi->lstUnplacedComponents->count() > 0) {
      int index = qMin(selectedIndex, mUi->lstUnplacedComponents->count() - 1);
      mUi->lstUnplacedComponents->setCurrentRow(index);
    }
  }

  setWindowTitle(QString(tr("Place Devices [%1]"))
                     .arg(mUi->lstUnplacedComponents->count()));
  emit unplacedComponentsCountChanged(getUnplacedComponentsCount());
}

void UnplacedComponentsDock::setSelectedComponentInstance(
    ComponentInstance* cmp) noexcept {
  setSelectedDeviceAndPackage(nullptr, nullptr);
  mUi->lblNoDeviceFound->hide();
  mUi->cbxSelectedDevice->clear();
  mUi->cbxSelectedDevice->show();
  mSelectedComponent = cmp;

  if (mBoard && mSelectedComponent) {
    QStringList localeOrder = mProject.getSettings().getLocaleOrder();
    QSet<Uuid>  devices =
        mProjectEditor.getWorkspace().getLibraryDb().getDevicesOfComponent(
            mSelectedComponent->getLibComponent().getUuid());
    foreach (const Uuid& deviceUuid, devices) {
      // get device metadata
      FilePath devFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatestDevice(
              deviceUuid);
      if (!devFp.isValid()) continue;
      QString devName;
      mProjectEditor.getWorkspace()
          .getLibraryDb()
          .getElementTranslations<library::Device>(devFp, localeOrder,
                                                   &devName);
      Uuid pkgUuid =
          Uuid::createRandom();  // only for initialization, will be overwritten
      mProjectEditor.getWorkspace().getLibraryDb().getDeviceMetadata(devFp,
                                                                     &pkgUuid);

      // get package metadata
      FilePath pkgFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatestPackage(
              pkgUuid);
      if (!pkgFp.isValid()) continue;
      QString pkgName;
      mProjectEditor.getWorkspace()
          .getLibraryDb()
          .getElementTranslations<library::Package>(pkgFp, localeOrder,
                                                    &pkgName);

      if (devName.contains(pkgName, Qt::CaseInsensitive)) {
        // Package name is already contained in device name, no need to show it.
        mUi->cbxSelectedDevice->addItem(devName, deviceUuid.toStr());
      } else {
        QString text = QString("%1 [%2]").arg(devName, pkgName);
        mUi->cbxSelectedDevice->addItem(text, deviceUuid.toStr());
      }
    }
    if (mUi->cbxSelectedDevice->count() > 0) {
      mUi->cbxSelectedDevice->model()->sort(0);
      tl::optional<Uuid> deviceUuid =
          mSelectedComponent->getDefaultDeviceUuid();
      if (!deviceUuid) {
        deviceUuid = mLastDeviceOfComponent.value(
            mSelectedComponent->getLibComponent().getUuid(),
            Uuid::createRandom());
      }
      int index = deviceUuid
                      ? mUi->cbxSelectedDevice->findData(deviceUuid->toStr())
                      : 0;
      mUi->cbxSelectedDevice->setCurrentIndex(qMax(index, 0));
    } else {
      mUi->cbxSelectedDevice->hide();
      mUi->lblNoDeviceFound->show();
    }
  }
}

void UnplacedComponentsDock::setSelectedDeviceAndPackage(
    const library::Device* device, const library::Package* package) noexcept {
  setSelectedFootprintUuid(tl::nullopt);
  mUi->cbxSelectedFootprint->clear();
  delete mSelectedPackage;
  mSelectedPackage = nullptr;
  delete mSelectedDevice;
  mSelectedDevice = nullptr;

  if (mBoard && mSelectedComponent && device && package) {
    if (device->getComponentUuid() ==
        mSelectedComponent->getLibComponent().getUuid()) {
      mSelectedDevice         = device;
      mSelectedPackage        = package;
      QStringList localeOrder = mProject.getSettings().getLocaleOrder();
      for (const library::Footprint& fpt : mSelectedPackage->getFootprints()) {
        mUi->cbxSelectedFootprint->addItem(*fpt.getNames().value(localeOrder),
                                           fpt.getUuid().toStr());
      }
      if (mUi->cbxSelectedFootprint->count() > 0) {
        tl::optional<Uuid> footprintUuid =
            mLastFootprintOfDevice.value(mSelectedDevice->getUuid());
        int index =
            footprintUuid
                ? mUi->cbxSelectedFootprint->findData(footprintUuid->toStr())
                : 0;
        mUi->cbxSelectedFootprint->setCurrentIndex(index);
      }
    }
  }
}

void UnplacedComponentsDock::setSelectedFootprintUuid(
    const tl::optional<Uuid>& uuid) noexcept {
  mUi->btnAdd->setEnabled(false);
  if (mFootprintPreviewGraphicsItem) {
    mFootprintPreviewGraphicsScene->removeItem(*mFootprintPreviewGraphicsItem);
    delete mFootprintPreviewGraphicsItem;
    mFootprintPreviewGraphicsItem = nullptr;
  }
  mSelectedFootprintUuid = uuid;

  if (mBoard && mSelectedComponent && mSelectedDevice && mSelectedPackage &&
      mSelectedFootprintUuid) {
    const library::Footprint* fpt =
        mSelectedPackage->getFootprints().find(*mSelectedFootprintUuid).get();
    if (fpt) {
      mFootprintPreviewGraphicsItem = new library::FootprintPreviewGraphicsItem(
          *mGraphicsLayerProvider, mProject.getSettings().getLocaleOrder(),
          *fpt, mSelectedPackage, &mSelectedComponent->getLibComponent(),
          mSelectedComponent);
      mFootprintPreviewGraphicsScene->addItem(*mFootprintPreviewGraphicsItem);
      mUi->graphicsView->zoomAll();
      mUi->btnAdd->setEnabled(true);
    }
  }
}

void UnplacedComponentsDock::beginUndoCmdGroup() noexcept {
  mCurrentUndoCmdGroup.reset(new UndoCommandGroup(tr("Add device to board")));
}

void UnplacedComponentsDock::addNextDeviceToCmdGroup(
    ComponentInstance& cmp, const Uuid& deviceUuid,
    const tl::optional<Uuid>& footprintUuid) noexcept {
  Q_ASSERT(mBoard);
  mLastDeviceOfComponent.insert(cmp.getLibComponent().getUuid(), deviceUuid);
  mLastFootprintOfDevice.insert(deviceUuid, footprintUuid);
  mCurrentUndoCmdGroup->appendChild(
      new CmdAddDeviceToBoard(mProjectEditor.getWorkspace(), *mBoard, cmp,
                              deviceUuid, footprintUuid, mNextPosition));

  // update current position
  if (mNextPosition.getX() > Length::fromMm(200))
    mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
  else
    mNextPosition += Point::fromMm(10, 0);
  mNextPosition.mapToGrid(mBoard->getGridProperties().getInterval());
}

void UnplacedComponentsDock::commitUndoCmdGroup() noexcept {
  mDisableListUpdate = true;
  try {
    mProjectEditor.getUndoStack().execCmd(mCurrentUndoCmdGroup.take());
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
  mDisableListUpdate = false;
}

void UnplacedComponentsDock::addDeviceManually(ComponentInstance& cmp,
                                               const Uuid&        deviceUuid,
                                               Uuid footprintUuid) noexcept {
  Q_ASSERT(mBoard);
  mLastDeviceOfComponent.insert(cmp.getLibComponent().getUuid(), deviceUuid);
  mLastFootprintOfDevice.insert(deviceUuid, footprintUuid);
  addDeviceTriggered(cmp, deviceUuid, footprintUuid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

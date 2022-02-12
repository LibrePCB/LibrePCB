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

#include "../../library/pkg/footprintpreviewgraphicsitem.h"
#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../undostack.h"
#include "../../widgets/graphicsview.h"
#include "../cmd/cmdadddevicetoboard.h"
#include "../projecteditor.h"
#include "ui_unplacedcomponentsdock.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/project/projectsettings.h>
#include <librepcb/core/types/gridproperties.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

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

UnplacedComponentsDock::UnplacedComponentsDock(ProjectEditor& editor,
                                               QWidget* parent) noexcept
  : QDockWidget(parent),
    mProjectEditor(editor),
    mProject(editor.getProject()),
    mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mDisableListUpdate(false),
    mNextPosition(),
    mLastDeviceOfComponent(),
    mLastFootprintOfPackage(),
    mCurrentDevices(),
    mSelectedComponent(nullptr),
    mSelectedDeviceUuid(),
    mSelectedPackage(nullptr),
    mSelectedPackageOwned(false),
    mSelectedFootprintUuid(),
    mGraphicsLayerProvider(new DefaultGraphicsLayerProvider()),
    mPreviewGraphicsScene(new GraphicsScene()),
    mPreviewGraphicsItem(nullptr) {
  mUi->setupUi(this);
  mUi->graphicsView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
  mUi->graphicsView->setOriginCrossVisible(false);
  mUi->graphicsView->setScene(mPreviewGraphicsScene.data());

  // Restore UI settings.
  QSettings clientSettings;
  mUi->splitter->restoreState(
      clientSettings.value("unplaced_components_dock/splitter_state")
          .toByteArray());

  // Update components list each time a component gets added or removed.
  connect(&mProject.getCircuit(), &Circuit::componentAdded, this,
          &UnplacedComponentsDock::updateComponentsList);
  connect(&mProject.getCircuit(), &Circuit::componentRemoved, this,
          &UnplacedComponentsDock::updateComponentsList);
  updateComponentsList();

  // Connect UI events to methods.
  connect(mUi->lstUnplacedComponents, &QListWidget::currentItemChanged, this,
          &UnplacedComponentsDock::currentComponentListItemChanged);
  connect(mUi->lstUnplacedComponents, &QListWidget::itemDoubleClicked, this,
          &UnplacedComponentsDock::addSelectedDeviceToBoard);
  connect(
      mUi->cbxSelectedDevice,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &UnplacedComponentsDock::currentDeviceIndexChanged);
  connect(
      mUi->cbxSelectedFootprint,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &UnplacedComponentsDock::currentFootprintIndexChanged);
  connect(mUi->cbxIsDefaultDevice, &QCheckBox::clicked, this,
          &UnplacedComponentsDock::setSelectedDeviceAsDefault);
  connect(mUi->btnAdd, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addSelectedDeviceToBoard);
  connect(mUi->btnAddSimilar, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addSimilarDevicesToBoard);
  connect(mUi->btnAddPreSelected, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addPreSelectedDevicesToBoard);
  connect(mUi->btnAddAll, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addAllDevicesToBoard);
}

UnplacedComponentsDock::~UnplacedComponentsDock() noexcept {
  QSettings clientSettings;
  clientSettings.setValue("unplaced_components_dock/splitter_state",
                          mUi->splitter->saveState());

  setBoard(nullptr);
  mDisableListUpdate = true;
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
  if (mBoard) {
    disconnect(mBoard, &Board::deviceAdded, this,
               &UnplacedComponentsDock::updateComponentsList);
    disconnect(mBoard, &Board::deviceRemoved, this,
               &UnplacedComponentsDock::updateComponentsList);
    mBoard = nullptr;
    updateComponentsList();
  }

  if (board) {
    mBoard = board;
    connect(mBoard, &Board::deviceAdded, this,
            &UnplacedComponentsDock::updateComponentsList);
    connect(mBoard, &Board::deviceRemoved, this,
            &UnplacedComponentsDock::updateComponentsList);
    mNextPosition = Point::fromMm(0, -20).mappedToGrid(
        mBoard->getGridProperties().getInterval());
    updateComponentsList();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnplacedComponentsDock::updateComponentsList() noexcept {
  if (mDisableListUpdate) return;

  bool hasPreselectedDevices = false;
  int selectedIndex = mUi->lstUnplacedComponents->currentRow();
  setSelectedComponentInstance(nullptr);
  mUi->lstUnplacedComponents->clear();

  if (mBoard) {
    QList<ComponentInstance*> componentsList =
        mProject.getCircuit().getComponentInstances().values();
    const QMap<Uuid, BI_Device*> boardDeviceList = mBoard->getDeviceInstances();

    // Sort components manually using numeric sort.
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setIgnorePunctuation(false);
    collator.setNumericMode(true);
    std::sort(componentsList.begin(), componentsList.end(),
              [&collator](const ComponentInstance* lhs,
                          const ComponentInstance* rhs) {
                return collator(*lhs->getName(), *rhs->getName());
              });

    foreach (ComponentInstance* component, componentsList) {
      if (boardDeviceList.contains(component->getUuid())) continue;
      if (component->getLibComponent().isSchematicOnly()) continue;
      bool hasPreSelectedDevice = component->getDefaultDeviceUuid().has_value();

      // Add component to list.
      QString value = component->getValue(true)
                          .split("\n", QString::SkipEmptyParts)
                          .join("|");
      QString libCmpName = *component->getLibComponent().getNames().value(
          mProject.getSettings().getLocaleOrder());
      QStringList text = {*component->getName() % ":"};
      if (hasPreSelectedDevice) {
        text += "✔";
        hasPreselectedDevices = true;
      }
      text += value;
      text += libCmpName;
      QListWidgetItem* item =
          new QListWidgetItem(text.join(" "), mUi->lstUnplacedComponents);
      item->setData(Qt::UserRole, component->getUuid().toStr());
      QStringList tooltip;
      tooltip += tr("Designator") % ": " % *component->getName();
      tooltip += tr("Value") % ": " % value;
      tooltip += tr("Component") % ": " % libCmpName;
      if (hasPreSelectedDevice) {
        tooltip += "✔ " % tr("Device is already pre-selected in schematics.");
      }
      item->setToolTip(tooltip.join("\n"));
    }

    if (mUi->lstUnplacedComponents->count() > 0) {
      int index = qMin(selectedIndex, mUi->lstUnplacedComponents->count() - 1);
      mUi->lstUnplacedComponents->setCurrentRow(index);
    }
  }

  if (hasPreselectedDevices) {
    mUi->btnAddAll->setVisible(false);
    mUi->btnAddPreSelected->setVisible(true);
  } else {
    mUi->btnAddPreSelected->setVisible(false);
    mUi->btnAddAll->setVisible(true);
  }

  mUi->btnAddPreSelected->setEnabled(getUnplacedComponentsCount() > 0);
  mUi->btnAddAll->setEnabled(getUnplacedComponentsCount() > 0);
  setWindowTitle(tr("Place Devices [%1]").arg(getUnplacedComponentsCount()));
  emit unplacedComponentsCountChanged(getUnplacedComponentsCount());
}

void UnplacedComponentsDock::currentComponentListItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
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

void UnplacedComponentsDock::currentDeviceIndexChanged(int index) noexcept {
  // Set tooltip to make long texts readable.
  mUi->cbxSelectedDevice->setToolTip(mUi->cbxSelectedDevice->currentText());

  // Abort if index is out of bounds.
  if ((index < 0) || (index >= mCurrentDevices.count())) {
    setSelectedDeviceAndPackage(tl::nullopt, nullptr, false);
    return;
  }

  try {
    const DeviceMetadata& device = mCurrentDevices.at(index);
    bool packageOwned = false;
    // Prefer package in project library for several reasons:
    //  - Allow adding devices even if package not found in workspace library
    //  - Use correct package (version) for preview
    //  - Better performance than loading workspace library elements
    const Package* package =
        mProject.getLibrary().getPackage(device.packageUuid);
    if (!package) {
      // If package does not exist in project library, use workspace library.
      FilePath pkgFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatest<Package>(
              device.packageUuid);
      if (pkgFp.isValid()) {
        package = new Package(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(pkgFp))));
        packageOwned = true;
      }
    }
    setSelectedDeviceAndPackage(device.deviceUuid, package, packageOwned);
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void UnplacedComponentsDock::currentFootprintIndexChanged(int index) noexcept {
  // Set tooltip to make long texts readable.
  mUi->cbxSelectedFootprint->setToolTip(
      mUi->cbxSelectedFootprint->currentText());

  tl::optional<Uuid> footprintUuid = Uuid::tryFromString(
      mUi->cbxSelectedFootprint->itemData(index, Qt::UserRole).toString());
  setSelectedFootprintUuid(footprintUuid);
}

void UnplacedComponentsDock::setSelectedComponentInstance(
    ComponentInstance* cmp) noexcept {
  setSelectedDeviceAndPackage(tl::nullopt, nullptr, false);
  mUi->lblNoDeviceFound->hide();
  mUi->cbxSelectedDevice->clear();
  mCurrentDevices.clear();
  mSelectedComponent = cmp;

  if (mBoard && mSelectedComponent) {
    std::pair<QList<DeviceMetadata>, int> devices =
        getAvailableDevices(*mSelectedComponent);
    mCurrentDevices = devices.first;
    for (int i = 0; i < mCurrentDevices.count(); ++i) {
      const DeviceMetadata& device = mCurrentDevices.at(i);
      QString text = device.deviceName;
      if (!text.contains(device.packageName, Qt::CaseInsensitive)) {
        // Package name not contained in device name, so let's show it as well.
        text += " [" % device.packageName % "]";
      }
      mUi->cbxSelectedDevice->addItem(text);
      if (device.selectedInSchematic) {
        // Highlight pre-selected device.
        QFont font =
            mUi->cbxSelectedDevice->itemData(i, Qt::FontRole).value<QFont>();
        font.setBold(true);
        mUi->cbxSelectedDevice->setItemData(i, font, Qt::FontRole);
      }
    }
    mUi->cbxSelectedDevice->setCurrentIndex(devices.second);
    if (mUi->cbxSelectedDevice->count() == 0) {
      mUi->lblNoDeviceFound->show();
    }
  }

  mUi->cbxIsDefaultDevice->setEnabled(mUi->cbxSelectedDevice->count() > 0);
  mUi->cbxSelectedDevice->setEnabled(mUi->cbxSelectedDevice->count() > 1);
}

void UnplacedComponentsDock::setSelectedDeviceAndPackage(
    const tl::optional<Uuid>& deviceUuid, const Package* package,
    bool packageOwned) noexcept {
  setSelectedFootprintUuid(tl::nullopt);
  mUi->lblNoDeviceFound->setVisible(deviceUuid && (!package));
  mUi->cbxIsDefaultDevice->setCheckState(Qt::Unchecked);
  mUi->cbxSelectedFootprint->clear();
  if (mSelectedPackageOwned) {
    delete mSelectedPackage;
  }
  mSelectedPackage = nullptr;
  mSelectedPackageOwned = false;
  mSelectedDeviceUuid = tl::nullopt;

  if (mBoard && mSelectedComponent && deviceUuid && package) {
    mSelectedDeviceUuid = deviceUuid;
    mSelectedPackage = package;
    mSelectedPackageOwned = packageOwned;
    if (deviceUuid == mSelectedComponent->getDefaultDeviceUuid()) {
      mUi->cbxIsDefaultDevice->setCheckState(Qt::Checked);
    } else if (mSelectedComponent->getDefaultDeviceUuid()) {
      mUi->cbxIsDefaultDevice->setCheckState(Qt::PartiallyChecked);
    }
    QStringList localeOrder = mProject.getSettings().getLocaleOrder();
    for (const Footprint& fpt : mSelectedPackage->getFootprints()) {
      mUi->cbxSelectedFootprint->addItem(*fpt.getNames().value(localeOrder),
                                         fpt.getUuid().toStr());
    }
    if (mUi->cbxSelectedFootprint->count() > 0) {
      // Highlight the default footprint (index 0).
      QFont font =
          mUi->cbxSelectedFootprint->itemData(0, Qt::FontRole).value<QFont>();
      font.setBold(true);
      mUi->cbxSelectedFootprint->setItemData(0, font, Qt::FontRole);
      mUi->cbxSelectedFootprint->setItemData(0, tr("Default footprint."),
                                             Qt::ToolTipRole);

      // Select most relevant footprint.
      tl::optional<Uuid> fpt =
          getSuggestedFootprint(mSelectedPackage->getUuid());
      int index = fpt ? mUi->cbxSelectedFootprint->findData(fpt->toStr()) : 0;
      mUi->cbxSelectedFootprint->setCurrentIndex(index);
    }
  }

  mUi->cbxSelectedFootprint->setEnabled(mUi->cbxSelectedFootprint->count() > 1);
}

void UnplacedComponentsDock::setSelectedFootprintUuid(
    const tl::optional<Uuid>& uuid) noexcept {
  mUi->btnAdd->setEnabled(false);
  mUi->btnAddSimilar->setEnabled(false);
  if (mPreviewGraphicsItem) {
    mPreviewGraphicsScene->removeItem(*mPreviewGraphicsItem);
    mPreviewGraphicsItem.reset();
  }
  mSelectedFootprintUuid = uuid;

  if (mBoard && mSelectedComponent && mSelectedDeviceUuid && mSelectedPackage &&
      mSelectedFootprintUuid) {
    const Footprint* fpt =
        mSelectedPackage->getFootprints().find(*mSelectedFootprintUuid).get();
    if (fpt) {
      mPreviewGraphicsItem.reset(new FootprintPreviewGraphicsItem(
          *mGraphicsLayerProvider.data(),
          mProject.getSettings().getLocaleOrder(), *fpt, mSelectedPackage,
          &mSelectedComponent->getLibComponent(), mSelectedComponent));
      mPreviewGraphicsScene->addItem(*mPreviewGraphicsItem);
      mUi->graphicsView->zoomAll();
      mUi->btnAdd->setEnabled(true);
      mUi->btnAddSimilar->setEnabled(true);
    }
  }
}

void UnplacedComponentsDock::setSelectedDeviceAsDefault() noexcept {
  if (mBoard && mSelectedComponent && mSelectedDeviceUuid) {
    try {
      QScopedPointer<CmdComponentInstanceEdit> cmd(new CmdComponentInstanceEdit(
          mProject.getCircuit(), *mSelectedComponent));
      if (mSelectedComponent->getDefaultDeviceUuid() == mSelectedDeviceUuid) {
        cmd->setDefaultDeviceUuid(tl::nullopt);
      } else {
        cmd->setDefaultDeviceUuid(mSelectedDeviceUuid);
      }
      mProjectEditor.getUndoStack().execCmd(cmd.take());
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
  }
  updateComponentsList();
}

void UnplacedComponentsDock::addSelectedDeviceToBoard() noexcept {
  if (mBoard && mSelectedComponent && mSelectedDeviceUuid && mSelectedPackage &&
      mSelectedFootprintUuid) {
    mLastDeviceOfComponent.insert(
        mSelectedComponent->getLibComponent().getUuid(), *mSelectedDeviceUuid);
    mLastFootprintOfPackage.insert(mSelectedPackage->getUuid(),
                                   *mSelectedFootprintUuid);
    emit addDeviceTriggered(*mSelectedComponent, *mSelectedDeviceUuid,
                            *mSelectedFootprintUuid);
  }
  updateComponentsList();
}

void UnplacedComponentsDock::addSimilarDevicesToBoard() noexcept {
  if (mBoard && mSelectedComponent && mSelectedDeviceUuid && mSelectedPackage &&
      mSelectedFootprintUuid) {
    mLastDeviceOfComponent.insert(
        mSelectedComponent->getLibComponent().getUuid(), *mSelectedDeviceUuid);
    mLastFootprintOfPackage.insert(mSelectedPackage->getUuid(),
                                   *mSelectedFootprintUuid);
    autoAddDevicesToBoard(false,
                          mSelectedComponent->getLibComponent().getUuid());
  }

  updateComponentsList();
}

void UnplacedComponentsDock::addAllDevicesToBoard() noexcept {
  if (mBoard) {
    autoAddDevicesToBoard(false, tl::nullopt);
  }

  updateComponentsList();
}

void UnplacedComponentsDock::addPreSelectedDevicesToBoard() noexcept {
  if (mBoard) {
    autoAddDevicesToBoard(true, tl::nullopt);
  }

  updateComponentsList();
}

void UnplacedComponentsDock::autoAddDevicesToBoard(
    bool onlyWithPreSelectedDevice,
    const tl::optional<Uuid>& libCmpUuidFilter) noexcept {
  Q_ASSERT(mBoard);
  QScopedPointer<UndoCommandGroup> cmd(
      new UndoCommandGroup(tr("Add devices to board")));

  for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++) {
    tl::optional<Uuid> componentUuid = Uuid::tryFromString(
        mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
    if (!componentUuid) continue;
    ComponentInstance* component =
        mProject.getCircuit().getComponentInstanceByUuid(*componentUuid);
    if (component &&
        ((!onlyWithPreSelectedDevice) || component->getDefaultDeviceUuid()) &&
        ((!libCmpUuidFilter) ||
         (component->getLibComponent().getUuid() == *libCmpUuidFilter))) {
      std::pair<QList<DeviceMetadata>, int> devices =
          getAvailableDevices(*component);
      if ((devices.second >= 0) && (devices.second < devices.first.count())) {
        const DeviceMetadata& dev = devices.first.at(devices.second);
        tl::optional<Uuid> fptUuid = getSuggestedFootprint(dev.packageUuid);
        cmd->appendChild(new CmdAddDeviceToBoard(
            mProjectEditor.getWorkspace(), *mBoard, *component, dev.deviceUuid,
            fptUuid, mNextPosition));

        // Update current position.
        if (mNextPosition.getX() > Length::fromMm(100)) {
          mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
        } else {
          mNextPosition += Point::fromMm(10, 0);
        }
        mNextPosition.mapToGrid(mBoard->getGridProperties().getInterval());
      }
    }
  }

  mDisableListUpdate = true;
  try {
    mProjectEditor.getUndoStack().execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
  mDisableListUpdate = false;
}

std::pair<QList<UnplacedComponentsDock::DeviceMetadata>, int>
    UnplacedComponentsDock::getAvailableDevices(ComponentInstance& cmp) const
    noexcept {
  QList<DeviceMetadata> devices;
  Uuid cmpUuid = cmp.getLibComponent().getUuid();
  QStringList localeOrder = mProject.getSettings().getLocaleOrder();

  // Get matching devices in project library.
  QHash<Uuid, Device*> prjLibDev =
      mProject.getLibrary().getDevicesOfComponent(cmpUuid);
  for (auto i = prjLibDev.constBegin(); i != prjLibDev.constEnd(); ++i) {
    devices.append(
        DeviceMetadata{i.key(), *i.value()->getNames().value(localeOrder),
                       i.value()->getPackageUuid(), QString(), false});
  }

  // Get matching devices in workspace library.
  try {
    QSet<Uuid> wsLibDev =
        mProjectEditor.getWorkspace().getLibraryDb().getComponentDevices(
            cmpUuid);  // can throw
    wsLibDev -= Toolbox::toSet(prjLibDev.keys());
    foreach (const Uuid& deviceUuid, wsLibDev) {
      // Get device metadata.
      FilePath devFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatest<Device>(
              deviceUuid);  // can throw
      if (!devFp.isValid()) continue;
      QString devName;
      mProjectEditor.getWorkspace().getLibraryDb().getTranslations<Device>(
          devFp, localeOrder,
          &devName);  // can throw
      Uuid pkgUuid = Uuid::createRandom();  // Temporary.
      mProjectEditor.getWorkspace().getLibraryDb().getDeviceMetadata(
          devFp, nullptr,
          &pkgUuid);  // can throw

      devices.append(
          DeviceMetadata{deviceUuid, devName, pkgUuid, QString(), false});
    }
  } catch (const Exception& e) {
    qCritical() << "Error while listing devices in unplaced components dock:"
                << e.getMsg();
  }

  // Determine missing metadata.
  for (DeviceMetadata& device : devices) {
    if (const Package* package =
            mProjectEditor.getProject().getLibrary().getPackage(
                device.packageUuid)) {
      device.packageName = *package->getNames().value(localeOrder);
    } else {
      try {
        FilePath pkgFp =
            mProjectEditor.getWorkspace().getLibraryDb().getLatest<Package>(
                device.packageUuid);  // can throw
        if (!pkgFp.isValid()) continue;
        mProjectEditor.getWorkspace().getLibraryDb().getTranslations<Package>(
            pkgFp, localeOrder,
            &device.packageName);  // can throw
      } catch (const Exception& e) {
        qCritical()
            << "Error while querying packages in unplaced components dock:"
            << e.getMsg();
      }
    }
    device.selectedInSchematic =
        device.deviceUuid == cmp.getDefaultDeviceUuid();
  }

  // Sort by device name, using numeric sort.
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(devices.begin(), devices.end(),
            [&collator](const DeviceMetadata& lhs, const DeviceMetadata& rhs) {
              return collator(lhs.deviceName, rhs.deviceName);
            });

  // Prio 1: Use the device chosen in the schematic.
  if (tl::optional<Uuid> dev = cmp.getDefaultDeviceUuid()) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).deviceUuid == *dev) {
        return std::make_pair(devices, i);
      }
    }
    qWarning() << "Selected device" << *dev
               << "not found in library, will use another device.";
  }

  // Prio 2: Use the device already used for the same component before.
  auto lastDeviceIterator = mLastDeviceOfComponent.find(cmpUuid);
  if ((lastDeviceIterator != mLastDeviceOfComponent.end())) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).deviceUuid == *lastDeviceIterator) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 3: Use the most used device in the current board.
  Q_ASSERT(mBoard);
  QHash<Uuid, int> devOccurences;
  foreach (const BI_Device* device, mBoard->getDeviceInstances()) {
    Q_ASSERT(device);
    if (device->getComponentInstance().getLibComponent().getUuid() ==
        cmp.getLibComponent().getUuid()) {
      ++devOccurences[device->getLibDevice().getUuid()];
    }
  }
  auto maxCountIt =
      std::max_element(devOccurences.constBegin(), devOccurences.constEnd());
  if (maxCountIt != devOccurences.constEnd()) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devOccurences.value(devices.at(i).deviceUuid) == (*maxCountIt)) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 4: Use the first device found in the project library.
  for (int i = 0; i < devices.count(); ++i) {
    if (prjLibDev.contains(devices.at(i).deviceUuid)) {
      return std::make_pair(devices, i);
    }
  }

  // Prio 5: Use the first device found in the workspace library.
  return std::make_pair(devices, devices.isEmpty() ? -1 : 0);
}

tl::optional<Uuid> UnplacedComponentsDock::getSuggestedFootprint(
    const Uuid& libPkgUuid) const noexcept {
  // Prio 1: Use the footprint already used for the same device before.
  auto lastFootprintIterator = mLastFootprintOfPackage.find(libPkgUuid);
  if ((lastFootprintIterator != mLastFootprintOfPackage.end())) {
    return *lastFootprintIterator;
  }

  // Prio 2: Use the most used footprint in the current board.
  Q_ASSERT(mBoard);
  QHash<Uuid, int> fptOccurences;
  foreach (const BI_Device* device, mBoard->getDeviceInstances()) {
    Q_ASSERT(device);
    if (device->getLibPackage().getUuid() == libPkgUuid) {
      ++fptOccurences[device->getLibFootprint().getUuid()];
    }
  }
  auto maxCountIt =
      std::max_element(fptOccurences.constBegin(), fptOccurences.constEnd());
  if (maxCountIt != fptOccurences.constEnd()) {
    QList<Uuid> uuids = fptOccurences.keys(*maxCountIt);
    if (uuids.count() > 0) {
      return uuids.first();
    }
  }

  // Prio 3: Fallback to the default footprint.
  return tl::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

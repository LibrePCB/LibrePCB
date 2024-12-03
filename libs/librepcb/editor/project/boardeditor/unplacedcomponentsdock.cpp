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

#include "../../graphics/defaultgraphicslayerprovider.h"
#include "../../graphics/graphicsscene.h"
#include "../../library/pkg/footprintgraphicsitem.h"
#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../undostack.h"
#include "../../widgets/graphicsview.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdadddevicetoboard.h"
#include "../projecteditor.h"
#include "ui_unplacedcomponentsdock.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

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
    mGraphicsLayerProvider(new DefaultGraphicsLayerProvider(
        mProjectEditor.getWorkspace().getSettings().themes.getActive())),
    mPreviewGraphicsScene(new GraphicsScene()),
    mPreviewGraphicsItem(nullptr) {
  mUi->setupUi(this);

  // Setup "no devices found" label.
  mUi->lblNoDeviceFound->setText(
      mUi->lblNoDeviceFound->text() % " " %
      tr("See details <a href=\"%1\">here</a>.")
          .arg("https://librepcb.org/_branches/develop/faq/"
               "#error-no-dev-or-pkg-found"));
  connect(mUi->lblNoDeviceFound, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mProjectEditor.getWorkspace().getSettings(),
                               this);
            ds.openWebUrl(QUrl(url));
          });

  // Setup graphics view.
  const Theme& theme =
      mProjectEditor.getWorkspace().getSettings().themes.getActive();
  mUi->graphicsView->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->graphicsView->setGridStyle(theme.getBoardGridStyle());
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
  connect(mUi->btnAdd, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addSelectedDeviceToBoard);
  connect(mUi->btnAddSimilar, &QPushButton::clicked, this,
          &UnplacedComponentsDock::addSimilarDevicesToBoard);
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
    mNextPosition =
        Point::fromMm(0, -20).mappedToGrid(mBoard->getGridInterval());
    updateComponentsList();
  }
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
    QList<ComponentInstance*> componentsList =
        mProject.getCircuit().getComponentInstances().values();
    const QMap<Uuid, BI_Device*> boardDeviceList = mBoard->getDeviceInstances();

    // Sort components manually using numeric sort.
    Toolbox::sortNumeric(
        componentsList,
        [](const QCollator& cmp, const ComponentInstance* lhs,
           const ComponentInstance* rhs) {
          return cmp(*lhs->getName(), *rhs->getName());
        },
        Qt::CaseInsensitive, false);

    foreach (ComponentInstance* component, componentsList) {
      if (boardDeviceList.contains(component->getUuid())) continue;
      if (component->getLibComponent().isSchematicOnly()) continue;

      // Add component to list.
      ProjectAttributeLookup lookup(*component, nullptr,
                                    component->getParts(std::nullopt).value(0));
      const QString value =
          AttributeSubstitutor::substitute(lookup("VALUE"), lookup)
              .split("\n", Qt::SkipEmptyParts)
              .join("|");
      QString libCmpName = *component->getLibComponent().getNames().value(
          mProject.getLocaleOrder());
      QStringList text = {*component->getName() % ":"};
      text += value;
      text += libCmpName;
      QListWidgetItem* item =
          new QListWidgetItem(text.join(" "), mUi->lstUnplacedComponents);
      item->setData(Qt::UserRole, component->getUuid().toStr());
      QStringList tooltip;
      tooltip += tr("Designator") % ": " % *component->getName();
      tooltip += tr("Value") % ": " % value;
      tooltip += tr("Component") % ": " % libCmpName;
      item->setToolTip(tooltip.join("\n"));
    }

    if (mUi->lstUnplacedComponents->count() > 0) {
      int index = qMin(selectedIndex, mUi->lstUnplacedComponents->count() - 1);
      mUi->lstUnplacedComponents->setCurrentRow(index);
    }
  }

  mUi->btnAddAll->setEnabled(getUnplacedComponentsCount() > 0);
  setWindowTitle(tr("Place Devices [%1]").arg(getUnplacedComponentsCount()));
  emit unplacedComponentsCountChanged(getUnplacedComponentsCount());
}

void UnplacedComponentsDock::currentComponentListItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);

  ComponentInstance* component = nullptr;
  if (mBoard && current) {
    std::optional<Uuid> cmpUuid =
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
    setSelectedDeviceAndPackage(std::nullopt, nullptr, false);
    return;
  }

  try {
    const DeviceMetadata& device = mCurrentDevices.at(index);
    bool packageOwned = false;
    // Prefer package in project library for several reasons:
    //  - Allow adding devices even if package not found in workspace library
    //  - Use correct package (version) for preview
    //  - Better performance than loading workspace library elements
    Package* package = mProject.getLibrary().getPackage(device.packageUuid);
    if (!package) {
      // If package does not exist in project library, use workspace library.
      FilePath pkgFp =
          mProjectEditor.getWorkspace().getLibraryDb().getLatest<Package>(
              device.packageUuid);
      if (pkgFp.isValid()) {
        package =
            Package::open(std::unique_ptr<TransactionalDirectory>(
                              new TransactionalDirectory(
                                  TransactionalFileSystem::openRO(pkgFp))))
                .release();
        packageOwned = true;
      }
    }
    setSelectedDeviceAndPackage(device.deviceUuid, package, packageOwned);
  } catch (const Exception& e) {
    qCritical() << "Failed to load device & package preview:" << e.getMsg();
  }
}

void UnplacedComponentsDock::currentFootprintIndexChanged(int index) noexcept {
  // Set tooltip to make long texts readable.
  mUi->cbxSelectedFootprint->setToolTip(
      mUi->cbxSelectedFootprint->currentText());

  std::optional<Uuid> footprintUuid = Uuid::tryFromString(
      mUi->cbxSelectedFootprint->itemData(index, Qt::UserRole).toString());
  setSelectedFootprintUuid(footprintUuid);
}

void UnplacedComponentsDock::setSelectedComponentInstance(
    ComponentInstance* cmp) noexcept {
  setSelectedDeviceAndPackage(std::nullopt, nullptr, false);
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
      if (device.isListedInComponentInstance) {
        text += " ✔";
      }
      mUi->cbxSelectedDevice->addItem(QIcon(":/img/library/device.png"), text);
    }
    mUi->cbxSelectedDevice->setCurrentIndex(devices.second);
    if (mUi->cbxSelectedDevice->count() == 0) {
      mUi->lblNoDeviceFound->show();
    }
  }

  mUi->cbxSelectedDevice->setEnabled(mUi->cbxSelectedDevice->count() > 1);
}

void UnplacedComponentsDock::setSelectedDeviceAndPackage(
    const std::optional<Uuid>& deviceUuid, Package* package,
    bool packageOwned) noexcept {
  setSelectedFootprintUuid(std::nullopt);
  mUi->lblNoDeviceFound->setVisible(deviceUuid && (!package));
  mUi->cbxSelectedFootprint->clear();
  if (mSelectedPackageOwned) {
    delete mSelectedPackage;
  }
  mSelectedPackage = nullptr;
  mSelectedPackageOwned = false;
  mSelectedDeviceUuid = std::nullopt;

  if (mBoard && mSelectedComponent && deviceUuid && package) {
    mSelectedDeviceUuid = deviceUuid;
    mSelectedPackage = package;
    mSelectedPackageOwned = packageOwned;
    QStringList localeOrder = mProject.getLocaleOrder();
    for (const Footprint& fpt : mSelectedPackage->getFootprints()) {
      mUi->cbxSelectedFootprint->addItem(QIcon(":/img/library/footprint.png"),
                                         *fpt.getNames().value(localeOrder),
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
      std::optional<Uuid> fpt =
          getSuggestedFootprint(mSelectedPackage->getUuid());
      int index = fpt ? mUi->cbxSelectedFootprint->findData(fpt->toStr()) : 0;
      mUi->cbxSelectedFootprint->setCurrentIndex(index);
    }
  }

  mUi->cbxSelectedFootprint->setEnabled(mUi->cbxSelectedFootprint->count() > 1);
}

void UnplacedComponentsDock::setSelectedFootprintUuid(
    const std::optional<Uuid>& uuid) noexcept {
  mUi->btnAdd->setEnabled(false);
  mUi->btnAddSimilar->setEnabled(false);
  if (mPreviewGraphicsItem) {
    mPreviewGraphicsScene->removeItem(*mPreviewGraphicsItem);
    mPreviewGraphicsItem.reset();
  }
  mSelectedFootprintUuid = uuid;

  if (mBoard && mSelectedComponent && mSelectedDeviceUuid && mSelectedPackage &&
      mSelectedFootprintUuid) {
    if (std::shared_ptr<Footprint> footprint =
            mSelectedPackage->getFootprints().find(*mSelectedFootprintUuid)) {
      mPreviewGraphicsItem.reset(new FootprintGraphicsItem(
          footprint, *mGraphicsLayerProvider.data(),
          Application::getDefaultStrokeFont(), &mSelectedPackage->getPads(),
          &mSelectedComponent->getLibComponent(), mProject.getLocaleOrder()));
      mPreviewGraphicsScene->addItem(*mPreviewGraphicsItem);
      mUi->graphicsView->zoomAll();
      mUi->btnAdd->setEnabled(true);
      mUi->btnAddSimilar->setEnabled(true);
    }
  }
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
    autoAddDevicesToBoard(mSelectedComponent->getLibComponent().getUuid());
  }

  updateComponentsList();
}

void UnplacedComponentsDock::addAllDevicesToBoard() noexcept {
  if (mBoard) {
    autoAddDevicesToBoard(std::nullopt);
  }

  updateComponentsList();
}

void UnplacedComponentsDock::autoAddDevicesToBoard(
    const std::optional<Uuid>& libCmpUuidFilter) noexcept {
  Q_ASSERT(mBoard);
  mProjectEditor.abortBlockingToolsInOtherEditors(this);  // Release undo stack.
  std::unique_ptr<UndoCommandGroup> cmd(
      new UndoCommandGroup(tr("Add devices to board")));

  for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++) {
    std::optional<Uuid> componentUuid = Uuid::tryFromString(
        mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toString());
    if (!componentUuid) continue;
    ComponentInstance* component =
        mProject.getCircuit().getComponentInstanceByUuid(*componentUuid);
    if (component &&
        ((!libCmpUuidFilter) ||
         (component->getLibComponent().getUuid() == *libCmpUuidFilter))) {
      std::pair<QList<DeviceMetadata>, int> devices =
          getAvailableDevices(*component);
      if ((devices.second >= 0) && (devices.second < devices.first.count())) {
        const DeviceMetadata& dev = devices.first.at(devices.second);
        std::optional<Uuid> fptUuid = getSuggestedFootprint(dev.packageUuid);
        cmd->appendChild(new CmdAddDeviceToBoard(
            mProjectEditor.getWorkspace(), *mBoard, *component, dev.deviceUuid,
            fptUuid, std::nullopt, mNextPosition));

        // Update current position.
        if (mNextPosition.getX() > Length::fromMm(100)) {
          mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
        } else {
          mNextPosition += Point::fromMm(10, 0);
        }
        mNextPosition.mapToGrid(mBoard->getGridInterval());
      }
    }
  }

  mDisableListUpdate = true;
  try {
    mProjectEditor.getUndoStack().execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
  mDisableListUpdate = false;
}

std::pair<QList<UnplacedComponentsDock::DeviceMetadata>, int>
    UnplacedComponentsDock::getAvailableDevices(
        ComponentInstance& cmp) const noexcept {
  QList<DeviceMetadata> devices;
  Uuid cmpUuid = cmp.getLibComponent().getUuid();
  QStringList localeOrder = mProject.getLocaleOrder();

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
    qCritical() << "Failed to list devices in unplaced components dock:"
                << e.getMsg();
  }

  // Determine missing metadata.
  const QSet<Uuid> cmpDevices = cmp.getCompatibleDevices();
  for (DeviceMetadata& device : devices) {
    device.isListedInComponentInstance = cmpDevices.contains(device.deviceUuid);
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
        qCritical() << "Failed to query packages in unplaced components dock:"
                    << e.getMsg();
      }
    }
  }

  // Sort by device name, using numeric sort.
  Toolbox::sortNumeric(
      devices,
      [](const QCollator& cmp, const DeviceMetadata& lhs,
         const DeviceMetadata& rhs) {
        return cmp(lhs.deviceName, rhs.deviceName);
      },
      Qt::CaseInsensitive, false);

  // Prio 1: Use the device already used for the same component before, if it
  // is chosen in the component instance.
  auto lastDeviceIterator = mLastDeviceOfComponent.find(cmpUuid);
  if ((lastDeviceIterator != mLastDeviceOfComponent.end())) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).isListedInComponentInstance &&
          (devices.at(i).deviceUuid == *lastDeviceIterator)) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 2: Use the first device chosen in the component instance.
  for (int i = 0; i < devices.count(); ++i) {
    if (devices.at(i).isListedInComponentInstance) {
      return std::make_pair(devices, i);
    }
  }

  // Prio 3: Use the device already used for the same component before.
  if ((lastDeviceIterator != mLastDeviceOfComponent.end())) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).deviceUuid == *lastDeviceIterator) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 4: Use the most used device in the current board.
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

  // Prio 5: Use the first device found in the project library.
  for (int i = 0; i < devices.count(); ++i) {
    if (prjLibDev.contains(devices.at(i).deviceUuid)) {
      return std::make_pair(devices, i);
    }
  }

  // Prio 6: Use the first device found in the workspace library.
  return std::make_pair(devices, devices.isEmpty() ? -1 : 0);
}

std::optional<Uuid> UnplacedComponentsDock::getSuggestedFootprint(
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
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

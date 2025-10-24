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
#include "addcomponentdialog.h"

#include "../editorcommandset.h"
#include "../graphics/graphicslayerlist.h"
#include "../graphics/graphicsscene.h"
#include "../library/pkg/footprintgraphicsitem.h"
#include "../library/sym/symbolgraphicsitem.h"
#include "../modelview/partinformationdelegate.h"
#include "../utils/editortoolbox.h"
#include "../widgets/graphicsview.h"
#include "../widgets/waitingspinnerwidget.h"
#include "../workspace/categorytreemodellegacy.h"
#include "../workspace/desktopservices.h"
#include "partinformationtooltip.h"
#include "ui_addcomponentdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/theme.h>
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

AddComponentDialog::AddComponentDialog(const WorkspaceLibraryDb& db,
                                       const WorkspaceSettings& settings,
                                       const QStringList& localeOrder,
                                       const QStringList& normOrder,
                                       QWidget* parent)
  : QDialog(parent),
    mDb(db),
    mSettings(settings),
    mLocaleOrder(localeOrder),
    mNormOrder(normOrder),
    mUi(new Ui::AddComponentDialog),
    mComponentPreviewScene(new GraphicsScene()),
    mDevicePreviewScene(new GraphicsScene()),
    mLayers(GraphicsLayerList::previewLayers(&mSettings)),
    mCategoryTreeModel(new CategoryTreeModelLegacy(
        mDb, mLocaleOrder,
        CategoryTreeModelLegacy::Filter::CmpCatWithComponents)),
    mPartToolTip(new PartInformationToolTip(mSettings, this)),
    mPartInfoProgress(0),
    mUpdatePartInformationScheduled(false),
    mUpdatePartInformationDownloadStart(0),
    mUpdatePartInformationOnExpand(true),
    mCurrentSearchTerm(),
    mSelectedComponent(nullptr),
    mSelectedSymbVar(nullptr),
    mSelectedDevice(nullptr),
    mSelectedPackage(nullptr),
    mSelectedPart(nullptr),
    mPreviewFootprintGraphicsItem(nullptr) {
  mUi->setupUi(this);
  mUi->treeComponents->setColumnCount(3);
  mUi->treeComponents->header()->setStretchLastSection(false);
  mUi->treeComponents->header()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  mUi->treeComponents->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  mUi->treeComponents->header()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  mUi->treeComponents->header()->setMinimumSectionSize(0);
  mUi->treeComponents->setItemDelegateForColumn(
      2, new PartInformationDelegate(true, this));
  mUi->treeComponents->setColumnHidden(2, true);
  mUi->treeComponents->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mUi->treeComponents, &QTreeWidget::customContextMenuRequested, this,
          &AddComponentDialog::customComponentsContextMenuRequested);
  mUi->lblCompDescription->hide();
  mUi->cbxSymbVar->hide();
  connect(mUi->edtSearch, &QLineEdit::textChanged, this,
          &AddComponentDialog::searchEditTextChanged);
  connect(mUi->treeComponents, &QTreeWidget::currentItemChanged, this,
          &AddComponentDialog::treeComponents_currentItemChanged);
  connect(mUi->treeComponents, &QTreeWidget::itemDoubleClicked, this,
          &AddComponentDialog::treeComponents_itemDoubleClicked);
  connect(mUi->treeComponents, &QTreeWidget::itemExpanded, this,
          &AddComponentDialog::treeComponents_itemExpanded);
  connect(
      mUi->cbxSymbVar,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &AddComponentDialog::cbxSymbVar_currentIndexChanged);
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this, [this]() {
    // Update component tree view since there might be new DB entries.
    // But for now very fundamental since keeping the selection is not
    // implemented yet.
    if ((!mCurrentSearchTerm.isEmpty()) &&
        (!mUi->treeComponents->currentItem())) {
      selectComponentByKeyword(mCurrentSearchTerm);
    }
  });

  // Setup part information tooltip.
  auto setProviderInfo = [this]() {
    mPartToolTip->setProviderInfo(
        PartInformationProvider::instance().getProviderName(),
        PartInformationProvider::instance().getProviderUrl(),
        PartInformationProvider::instance().getProviderLogo(),
        PartInformationProvider::instance().getInfoUrl());
  };
  setProviderInfo();
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::providerInfoChanged, this, setProviderInfo);
  mUi->treeComponents->setMouseTracking(true);
  mUi->treeComponents->installEventFilter(this);
  mPartToolTip->installEventFilter(this);
  connect(mUi->treeComponents, &QTreeWidget::itemEntered, this,
          [this](QTreeWidgetItem* item, int column) {
            if (item && (column == 2)) {
              const auto data = item->data(2, Qt::UserRole)
                                    .value<PartInformationDelegate::Data>();
              if (data.info && (data.info->results == 1)) {
                const QRect rect =
                    mUi->treeComponents->visualItemRect(item).intersected(
                        mUi->treeComponents->viewport()->rect());
                const QPoint pos = mUi->treeComponents->viewport()->mapToGlobal(
                    QPoint(rect.right(), rect.center().y()));
                mPartToolTip->showPart(data.info, pos);
              } else {
                mPartToolTip->hideAndReset(false);
              }
            } else {
              mPartToolTip->hideAndReset();
            }
          });

  // Add actions.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mActionCopyMpn.reset(cmd.copyMpnToClipboard.createAction(
      this, this,
      [this]() {
        if (mSelectedPart) {
          qApp->clipboard()->setText(*mSelectedPart->getMpn());
        }
      },
      EditorCommand::ActionFlag::WidgetShortcut));
  mUi->treeComponents->addAction(mActionCopyMpn.data());
  addAction(cmd.find.createAction(this, this, [this]() {
    mUi->edtSearch->setFocus(Qt::ShortcutFocusReason);
  }));

  // Setup symbol graphics view.
  const Theme& theme = mSettings.themes.getActive();
  mComponentPreviewScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mComponentPreviewScene->setGridStyle(theme.getBoardGridStyle());
  mComponentPreviewScene->setOriginCrossVisible(false);
  mUi->viewComponent->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->viewComponent->setScene(mComponentPreviewScene.data());

  // Setup package graphics view.
  mDevicePreviewScene->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mDevicePreviewScene->setGridStyle(theme.getBoardGridStyle());
  mDevicePreviewScene->setOriginCrossVisible(false);
  mUi->viewDevice->setSpinnerColor(
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->viewDevice->setScene(mDevicePreviewScene.data());

  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &AddComponentDialog::treeCategories_currentItemChanged);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [this](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&mDb, &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&mDb, &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(mDb.isScanInProgress());
  };
  addSpinner(mUi->treeCategories);
  addSpinner(mUi->treeComponents);

  // Setup automatic update of parts information.
  QTimer* partInfoTimer = new QTimer(this);
  partInfoTimer->setInterval(250);
  connect(partInfoTimer, &QTimer::timeout, this, [this]() {
    ++mPartInfoProgress;
    if (mUpdatePartInformationScheduled) {
      updatePartsInformation();
    }
  });
  partInfoTimer->start();
  connect(mUi->treeComponents->horizontalScrollBar(), &QScrollBar::valueChanged,
          this, &AddComponentDialog::schedulePartsInformationUpdate);
  connect(mUi->treeComponents->verticalScrollBar(), &QScrollBar::valueChanged,
          this, &AddComponentDialog::schedulePartsInformationUpdate);
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::serviceOperational, this,
          &AddComponentDialog::schedulePartsInformationUpdate);
  connect(&PartInformationProvider::instance(),
          &PartInformationProvider::newPartsInformationAvailable, this,
          &AddComponentDialog::schedulePartsInformationUpdate);

  // Reset GUI to state of nothing selected.
  setSelectedComponent(nullptr);

  // Restore client settings.
  QSettings clientSettings;
  mUi->cbxAddMore->setChecked(
      clientSettings
          .value("schematic_editor/add_component_dialog/add_more", true)
          .toBool());
  QSize windowSize =
      clientSettings.value("schematic_editor/add_component_dialog/window_size")
          .toSize();
  if (!windowSize.isEmpty()) {
    resize(windowSize);
  }

  // Move focus to search field to allow typing immediately.
  mUi->edtSearch->setFocus(Qt::ShortcutFocusReason);
}

AddComponentDialog::~AddComponentDialog() noexcept {
  // Save client settings.
  QSettings clientSettings;
  clientSettings.setValue("schematic_editor/add_component_dialog/add_more",
                          mUi->cbxAddMore->isChecked());
  clientSettings.setValue("schematic_editor/add_component_dialog/window_size",
                          size());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AddComponentDialog::setLocaleOrder(const QStringList& order) noexcept {
  mLocaleOrder = order;
  mCategoryTreeModel->setLocaleOrder(mLocaleOrder);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::optional<Package::AssemblyType>
    AddComponentDialog::getSelectedPackageAssemblyType() const noexcept {
  return (mSelectedComponent && mSelectedSymbVar && mSelectedDevice &&
          mSelectedPackage)
      ? std::make_optional(mSelectedPackage->getAssemblyType(true))
      : std::nullopt;
}

bool AddComponentDialog::getAutoOpenAgain() const noexcept {
  return mUi->cbxAddMore->isChecked();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AddComponentDialog::selectComponentByKeyword(
    const QString keyword, const std::optional<Uuid>& selectedDevice) noexcept {
  try {
    searchComponents(keyword, selectedDevice, true);
  } catch (const Exception& e) {
    qCritical().noquote() << "Failed to pre-select component by keyword:"
                          << e.getMsg();
  }
}

bool AddComponentDialog::eventFilter(QObject* obj, QEvent* e) noexcept {
  if (mPartToolTip && (e->type() == QEvent::Leave) &&
      ((!mPartToolTip->isVisible()) ||
       ((!mPartToolTip->rect().contains(
           mPartToolTip->mapFromGlobal(QCursor::pos())))))) {
    mPartToolTip->hideAndReset();
  }
  return QDialog::eventFilter(obj, e);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

bool AddComponentDialog::event(QEvent* event) noexcept {
  if (event->type() == QEvent::Resize) {
    schedulePartsInformationUpdate();
  }
  return QDialog::event(event);
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void AddComponentDialog::searchEditTextChanged(const QString& text) noexcept {
  mUi->lblErrorMsg->clear();
  try {
    QModelIndex catIndex = mUi->treeCategories->currentIndex();
    if (text.trimmed().isEmpty() && catIndex.isValid()) {
      // Change tab order: https://github.com/LibrePCB/LibrePCB/issues/1059
      setTabOrder(mUi->edtSearch, mUi->treeCategories);
      setSelectedCategory(
          Uuid::tryFromString(catIndex.data(Qt::UserRole).toString()));
    } else {
      // Change tab order: https://github.com/LibrePCB/LibrePCB/issues/1059
      setTabOrder(mUi->treeCategories, mUi->edtSearch);
      searchComponents(text.trimmed());
    }
  } catch (const Exception& e) {
    mUi->lblErrorMsg->setText(e.getMsg());
  }
}

void AddComponentDialog::treeCategories_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);
  mUi->lblErrorMsg->clear();

  try {
    std::optional<Uuid> categoryUuid =
        Uuid::tryFromString(current.data(Qt::UserRole).toString());
    setSelectedCategory(categoryUuid);
  } catch (Exception& e) {
    mUi->lblErrorMsg->setText(e.getMsg());
  }
}

void AddComponentDialog::treeComponents_currentItemChanged(
    QTreeWidgetItem* current, QTreeWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  mUi->lblErrorMsg->clear();
  try {
    if (current) {
      QTreeWidgetItem* partItem = current;
      QTreeWidgetItem* devItem = current->parent();
      QTreeWidgetItem* cmpItem = devItem ? devItem->parent() : nullptr;
      while (!cmpItem) {
        cmpItem = devItem;
        devItem = partItem;
        partItem = nullptr;
      }
      FilePath cmpFp = FilePath(cmpItem->data(0, Qt::UserRole).toString());
      if ((!mSelectedComponent) ||
          (mSelectedComponent->getDirectory().getAbsPath() != cmpFp)) {
        std::shared_ptr<Component> component(
            Component::open(std::unique_ptr<TransactionalDirectory>(
                                new TransactionalDirectory(
                                    TransactionalFileSystem::openRO(cmpFp))))
                .release());
        setSelectedComponent(component);
      }
      if (devItem) {
        FilePath devFp = FilePath(devItem->data(0, Qt::UserRole).toString());
        if ((!mSelectedDevice) ||
            (mSelectedDevice->getDirectory().getAbsPath() != devFp)) {
          std::shared_ptr<Device> device(
              Device::open(std::unique_ptr<TransactionalDirectory>(
                               new TransactionalDirectory(
                                   TransactionalFileSystem::openRO(devFp))))
                  .release());
          setSelectedDevice(device);
        }
        setSelectedPart(
            partItem
                ? partItem->data(0, Qt::UserRole).value<std::shared_ptr<Part>>()
                : nullptr);
      } else {
        setSelectedDevice(nullptr);
      }
    } else {
      setSelectedComponent(nullptr);
    }
  } catch (const Exception& e) {
    // Do not show a message box as it would be annoying while typing in the
    // search field.
    mUi->lblErrorMsg->setText(e.getMsg());
    setSelectedComponent(nullptr);
  }
}

void AddComponentDialog::treeComponents_itemDoubleClicked(QTreeWidgetItem* item,
                                                          int column) noexcept {
  if (item && item->parent() && item->parent()->parent() && (column == 2)) {
    const auto data =
        item->data(2, Qt::UserRole).value<PartInformationDelegate::Data>();
    if (data.info && data.info->pricingUrl.isValid()) {
      DesktopServices ds(mSettings);
      ds.openWebUrl(data.info->pricingUrl);
    }
  } else if (item) {
    accept();
  }
}

void AddComponentDialog::treeComponents_itemExpanded(
    QTreeWidgetItem* item) noexcept {
  if (mUpdatePartInformationOnExpand && item && (item->childCount() > 0)) {
    updatePartsInformation();
  }
}

void AddComponentDialog::cbxSymbVar_currentIndexChanged(int index) noexcept {
  if ((mSelectedComponent) && (index >= 0)) {
    std::optional<Uuid> uuid =
        Uuid::tryFromString(mUi->cbxSymbVar->itemData(index).toString());
    if (uuid) {
      setSelectedSymbVar(mSelectedComponent->getSymbolVariants().find(*uuid));
    } else {
      setSelectedSymbVar(nullptr);
    }
  } else {
    setSelectedSymbVar(nullptr);
  }
}

void AddComponentDialog::customComponentsContextMenuRequested(
    const QPoint& pos) noexcept {
  Q_UNUSED(pos);

  if ((!mSelectedPart) || (!mActionCopyMpn)) {
    return;
  }

  const EditorCommandSet& cmd = EditorCommandSet::instance();
  auto partInfo = PartInformationProvider::instance().getPartInfo(
      PartInformationProvider::Part{*mSelectedPart->getMpn(),
                                    *mSelectedPart->getManufacturer()});

  QMenu menu(this);
  menu.addAction(mActionCopyMpn.get());
  if (partInfo && partInfo->productUrl.isValid()) {
    menu.addAction(
        cmd.openProductWebsite.createAction(this, this, [this, partInfo]() {
          DesktopServices ds(mSettings);
          ds.openWebUrl(partInfo->productUrl);
        }));
  }
  if (partInfo && partInfo->pricingUrl.isValid()) {
    menu.addAction(
        cmd.openPricingWebsite.createAction(this, this, [this, partInfo]() {
          DesktopServices ds(mSettings);
          ds.openWebUrl(partInfo->pricingUrl);
        }));
  }
  if (partInfo && (!partInfo->resources.isEmpty())) {
    QAction* action =
        new QAction(EditorToolbox::svgIcon(":/fa/solid/file-pdf.svg"),
                    partInfo->resources.first().name + "...", &menu);
    connect(action, &QAction::triggered, this, [this, partInfo]() {
      DesktopServices ds(mSettings);
      ds.openWebUrl(partInfo->resources.first().url);
    });
    menu.addAction(action);
  }
  menu.exec(QCursor::pos());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AddComponentDialog::searchComponents(
    const QString& input, const std::optional<Uuid>& selectedDevice,
    bool selectFirstDevice) {
  mCurrentSearchTerm = input;
  setSelectedComponent(nullptr);
  mUi->treeComponents->clear();

  // Temporarily disable update on expand for performance reasons.
  mUpdatePartInformationOnExpand = false;
  auto disableExpandSg =
      scopeGuard([this]() { mUpdatePartInformationOnExpand = true; });

  QTreeWidgetItem* selectedDeviceItem = nullptr;

  // min. 2 chars to avoid freeze on entering first character due to huge result
  if (input.length() > 1) {
    SearchResult result = search(input);
    const bool expandAllDevices =
        (result.partsCount <= 15) || (result.deviceCount <= 1);
    const bool expandAllComponents =
        (result.deviceCount <= 10) || (result.components.count() <= 1);
    for (auto cmpIt = result.components.begin();
         cmpIt != result.components.end(); ++cmpIt) {
      QTreeWidgetItem* cmpItem = new QTreeWidgetItem(mUi->treeComponents);
      cmpItem->setIcon(0, QIcon(":/img/library/symbol.png"));
      cmpItem->setText(0, cmpIt.value().name);
      cmpItem->setForeground(
          0, cmpIt.value().deprecated ? QBrush(Qt::red) : QBrush());
      cmpItem->setData(0, Qt::UserRole, cmpIt.key().toStr());
      for (auto devIt = cmpIt->devices.begin(); devIt != cmpIt->devices.end();
           ++devIt) {
        QTreeWidgetItem* devItem = new QTreeWidgetItem(cmpItem);
        devItem->setIcon(0, QIcon(":/img/library/device.png"));
        devItem->setText(0, devIt.value().name);
        devItem->setForeground(
            0, devIt.value().deprecated ? QBrush(Qt::red) : QBrush());
        devItem->setData(0, Qt::UserRole, devIt.key().toStr());
        devItem->setText(1, devIt.value().pkgName);
        devItem->setTextAlignment(1, Qt::AlignRight);
        QFont font = devItem->font(1);
        font.setItalic(true);
        devItem->setFont(1, font);
        for (const auto& partPtr : devIt->parts.values()) {
          addPartItem(partPtr, devItem);
        }
        devItem->setExpanded(
            ((!cmpIt.value().match) && (!devIt.value().match)) ||
            expandAllDevices);
        if (devIt.value().uuid == selectedDevice) {
          selectedDeviceItem = devItem;
        }
      }
      cmpItem->setText(1, QString("[%1]").arg(cmpIt.value().devices.count()));
      cmpItem->setTextAlignment(1, Qt::AlignRight);
      cmpItem->setExpanded((!cmpIt.value().match) || expandAllComponents);
    }
  }

  mUi->treeComponents->sortByColumn(0, Qt::AscendingOrder);

  if (selectedDeviceItem) {
    mUi->treeComponents->setCurrentItem(selectedDeviceItem);
    while (selectedDeviceItem->parent()) {
      selectedDeviceItem->parent()->setExpanded(true);
      selectedDeviceItem = selectedDeviceItem->parent();
    }
  } else if (selectFirstDevice) {
    if (QTreeWidgetItem* cmpItem = mUi->treeComponents->topLevelItem(0)) {
      cmpItem->setExpanded(true);
      if (QTreeWidgetItem* devItem = cmpItem->child(0)) {
        mUi->treeComponents->setCurrentItem(devItem);
      } else {
        mUi->treeComponents->setCurrentItem(cmpItem);
      }
    }
  } else {
    QTreeWidgetItem* item = mUi->treeComponents->topLevelItem(0);
    while (item && item->isExpanded() && item->childCount()) {
      item = item->child(0);
    }
    while (item && item->parent() &&
           (!item->text(0).toLower().contains(input.toLower()))) {
      item = item->parent();
    }
    if (item) {
      mUi->treeComponents->setCurrentItem(item);
    }
  }

  // Delay parts information download, but show cached information immediately
  // to avoid flicker.
  updatePartsInformation(1200);
}

AddComponentDialog::SearchResult AddComponentDialog::search(
    const QString& input) {
  SearchResult result;

  // Find in library database.
  const QList<Uuid> matchingComponents =
      mDb.find<Component>(input);  // can throw
  const QList<Uuid> matchingDevices = mDb.find<Device>(input);  // can throw
  const QList<Uuid> matchingPartDevices =
      mDb.findDevicesOfParts(input);  // can throw

  // Add matching components and all their devices and parts.
  QSet<Uuid> fullyAddedDevices;
  foreach (const Uuid& cmpUuid, matchingComponents) {
    FilePath cmpFp = mDb.getLatest<Component>(cmpUuid);  // can throw
    if (!cmpFp.isValid()) continue;
    QSet<Uuid> devices = mDb.getComponentDevices(cmpUuid);  // can throw
    SearchResultComponent& resCmp = result.components[cmpFp];
    resCmp.match = true;
    foreach (const Uuid& devUuid, devices) {
      FilePath devFp = mDb.getLatest<Device>(devUuid);  // can throw
      if (!devFp.isValid()) continue;
      if (resCmp.devices.contains(devFp)) continue;
      Uuid pkgUuid = Uuid::createRandom();
      mDb.getDeviceMetadata(devFp, nullptr,
                            &pkgUuid);  // can throw
      FilePath pkgFp = mDb.getLatest<Package>(pkgUuid);  // can throw
      SearchResultDevice& resDev = resCmp.devices[devFp];
      resDev.uuid = devUuid;
      resDev.pkgFp = pkgFp;
      resDev.match = matchingDevices.contains(devUuid);
      const QList<WorkspaceLibraryDb::Part> parts =
          mDb.getDeviceParts(devUuid);  // can throw
      foreach (const WorkspaceLibraryDb::Part& part, parts) {
        resDev.parts.append(std::make_shared<Part>(
            SimpleString(part.mpn), SimpleString(part.manufacturer),
            part.attributes));
      }
      fullyAddedDevices.insert(devUuid);
    }
  }

  // Add matching devices + parts and their corresponding components.
  QList<Uuid> devices = matchingPartDevices;
  foreach (const Uuid& uuid, matchingDevices) {
    if (!devices.contains(uuid)) {
      devices.append(uuid);
    }
  }
  devices.erase(std::remove_if(devices.begin(), devices.end(),
                               [&fullyAddedDevices](const Uuid& uuid) {
                                 return fullyAddedDevices.contains(uuid);
                               }),
                devices.end());
  foreach (const Uuid& devUuid, devices) {
    FilePath devFp = mDb.getLatest<Device>(devUuid);  // can throw
    if (!devFp.isValid()) continue;
    Uuid cmpUuid = Uuid::createRandom();
    Uuid pkgUuid = Uuid::createRandom();
    mDb.getDeviceMetadata(devFp, &cmpUuid,
                          &pkgUuid);  // can throw
    const FilePath cmpFp = mDb.getLatest<Component>(cmpUuid);  // can throw
    if (!cmpFp.isValid()) continue;
    SearchResultDevice& resDev = result.components[cmpFp].devices[devFp];
    FilePath pkgFp = mDb.getLatest<Package>(pkgUuid);  // can throw
    resDev.uuid = devUuid;
    resDev.pkgFp = pkgFp;
    resDev.match = matchingDevices.contains(devUuid);

    QList<WorkspaceLibraryDb::Part> parts;
    if (resDev.match) {
      // List all parts of device.
      parts = mDb.getDeviceParts(devUuid);  // can throw
    } else {
      // List only matched parts of device.
      parts = mDb.findPartsOfDevice(devUuid,
                                    input);  // can throw
    }
    foreach (const WorkspaceLibraryDb::Part& part, parts) {
      resDev.parts.append(std::make_shared<Part>(
          SimpleString(part.mpn), SimpleString(part.manufacturer),
          part.attributes));
    }
  }

  // Get additional metadata of elements.
  QMutableHashIterator<FilePath, SearchResultComponent> cmpIt(
      result.components);
  while (cmpIt.hasNext()) {
    cmpIt.next();
    mDb.getTranslations<Component>(cmpIt.key(), mLocaleOrder,
                                   &cmpIt.value().name);
    mDb.getMetadata<Component>(cmpIt.key(), nullptr, nullptr,
                               &cmpIt.value().deprecated);
    QMutableHashIterator<FilePath, SearchResultDevice> devIt(
        cmpIt.value().devices);
    while (devIt.hasNext()) {
      devIt.next();
      mDb.getTranslations<Device>(devIt.key(), mLocaleOrder,
                                  &devIt.value().name);
      mDb.getMetadata<Device>(devIt.key(), nullptr, nullptr,
                              &devIt.value().deprecated);
      if (devIt.value().pkgFp.isValid()) {
        mDb.getTranslations<Package>(devIt.value().pkgFp, mLocaleOrder,
                                     &devIt.value().pkgName);
      }
    }
  }

  // Count number it items.
  foreach (const SearchResultComponent& cmp, result.components) {
    result.deviceCount += cmp.devices.count();
    foreach (const SearchResultDevice& dev, cmp.devices) {
      result.partsCount += dev.parts.count();
    }
  }

  return result;
}

void AddComponentDialog::setSelectedCategory(
    const std::optional<Uuid>& categoryUuid) {
  mCurrentSearchTerm.clear();
  setSelectedComponent(nullptr);
  mUi->treeComponents->clear();

  mSelectedCategoryUuid = categoryUuid;
  QSet<Uuid> components = mDb.getByCategory<Component>(categoryUuid);
  foreach (const Uuid& cmpUuid, components) {
    // component
    FilePath cmpFp = mDb.getLatest<Component>(cmpUuid);
    if (!cmpFp.isValid()) continue;
    QString cmpName;
    mDb.getTranslations<Component>(cmpFp, mLocaleOrder, &cmpName);
    bool cmpDeprecated = false;
    mDb.getMetadata<Component>(cmpFp, nullptr, nullptr, &cmpDeprecated);
    QTreeWidgetItem* cmpItem = new QTreeWidgetItem(mUi->treeComponents);
    cmpItem->setIcon(0, QIcon(":/img/library/symbol.png"));
    cmpItem->setText(0, cmpName);
    cmpItem->setForeground(0, cmpDeprecated ? QBrush(Qt::red) : QBrush());
    cmpItem->setData(0, Qt::UserRole, cmpFp.toStr());
    // devices
    QSet<Uuid> devices = mDb.getComponentDevices(cmpUuid);
    foreach (const Uuid& devUuid, devices) {
      try {
        FilePath devFp = mDb.getLatest<Device>(devUuid);
        if (!devFp.isValid()) continue;
        QString devName;
        mDb.getTranslations<Device>(devFp, mLocaleOrder, &devName);
        bool devDeprecated = false;
        mDb.getMetadata<Device>(devFp, nullptr, nullptr, &devDeprecated);
        QTreeWidgetItem* devItem = new QTreeWidgetItem(cmpItem);
        devItem->setIcon(0, QIcon(":/img/library/device.png"));
        devItem->setText(0, devName);
        devItem->setForeground(0, devDeprecated ? QBrush(Qt::red) : QBrush());
        devItem->setData(0, Qt::UserRole, devFp.toStr());
        // package
        Uuid pkgUuid = Uuid::createRandom();  // only for initialization, will
                                              // be overwritten
        mDb.getDeviceMetadata(devFp, nullptr, &pkgUuid);
        FilePath pkgFp = mDb.getLatest<Package>(pkgUuid);
        if (pkgFp.isValid()) {
          QString pkgName;
          mDb.getTranslations<Package>(pkgFp, mLocaleOrder, &pkgName);
          devItem->setText(1, pkgName);
          devItem->setTextAlignment(1, Qt::AlignRight);
          QFont font = devItem->font(1);
          font.setItalic(true);
          devItem->setFont(1, font);
        }
        // Parts
        const QList<WorkspaceLibraryDb::Part> parts =
            mDb.getDeviceParts(devUuid);
        foreach (const WorkspaceLibraryDb::Part& partInfo, parts) {
          std::shared_ptr<Part> part = std::make_shared<Part>(
              SimpleString(partInfo.mpn), SimpleString(partInfo.manufacturer),
              partInfo.attributes);
          addPartItem(part, devItem);
        }
      } catch (const Exception& e) {
        // what could we do here?
      }
    }
    cmpItem->setText(1, QString("[%1]").arg(devices.count()));
    cmpItem->setTextAlignment(1, Qt::AlignRight);
  }

  mUi->treeComponents->sortByColumn(0, Qt::AscendingOrder);

  // Delay parts information download, but show cached information immediately
  // to avoid flicker.
  updatePartsInformation(1000);
}

void AddComponentDialog::setSelectedComponent(
    std::shared_ptr<const Component> cmp) {
  if (cmp && (cmp == mSelectedComponent)) return;

  mUi->lblCompName->setText(tr("No component selected"));
  mUi->lblCompDescription->clear();
  mUi->cbxSymbVar->clear();
  setSelectedDevice(nullptr);
  setSelectedSymbVar(nullptr);
  mSelectedComponent = cmp;

  if (mSelectedComponent) {
    mUi->lblCompName->setText(*cmp->getNames().value(mLocaleOrder));
    mUi->lblCompDescription->setText(
        cmp->getDescriptions().value(mLocaleOrder));

    for (const ComponentSymbolVariant& symbVar : cmp->getSymbolVariants()) {
      QString text = *symbVar.getNames().value(mLocaleOrder);
      if (!symbVar.getNorm().isEmpty()) {
        text += " [" + symbVar.getNorm() + "]";
      }
      mUi->cbxSymbVar->addItem(text, symbVar.getUuid().toStr());
    }
    if (!cmp->getSymbolVariants().isEmpty()) {
      mUi->cbxSymbVar->setCurrentIndex(
          qMax(0, cmp->getSymbolVariantIndexByNorm(mNormOrder)));
    }
  }

  mUi->cbxSymbVar->setVisible(mUi->cbxSymbVar->count() > 1);
  mUi->lblCompDescription->setVisible(
      !mUi->lblCompDescription->text().isEmpty());
}

void AddComponentDialog::setSelectedSymbVar(
    std::shared_ptr<const ComponentSymbolVariant> symbVar) {
  if (symbVar && (symbVar == mSelectedSymbVar)) return;
  mPreviewSymbolGraphicsItems.clear();
  mPreviewSymbols.clear();
  mSelectedSymbVar = symbVar;

  if (mSelectedComponent && mSelectedSymbVar) {
    for (const ComponentSymbolVariantItem& item : symbVar->getSymbolItems()) {
      FilePath symbolFp = mDb.getLatest<Symbol>(item.getSymbolUuid());
      if (!symbolFp.isValid()) continue;  // TODO: show warning
      std::shared_ptr<Symbol> symbol(
          Symbol::open(std::unique_ptr<TransactionalDirectory>(
                           new TransactionalDirectory(
                               TransactionalFileSystem::openRO(symbolFp))))
              .release());
      mPreviewSymbols.append(symbol);

      auto graphicsItem = std::make_shared<SymbolGraphicsItem>(
          *symbol, *mLayers, mSelectedComponent.get(),
          mSelectedSymbVar->getSymbolItems().get(item.getUuid()), mLocaleOrder,
          true);
      graphicsItem->setPosition(item.getSymbolPosition());
      graphicsItem->setRotation(item.getSymbolRotation());
      mPreviewSymbolGraphicsItems.append(graphicsItem);
      mComponentPreviewScene->addItem(*graphicsItem);
      mUi->viewComponent->zoomAll();
    }
  }
}

void AddComponentDialog::setSelectedDevice(std::shared_ptr<const Device> dev) {
  if (dev && (dev == mSelectedDevice)) return;

  mUi->lblDeviceName->setText(tr("No device selected"));
  mPreviewFootprintGraphicsItem.reset();
  mSelectedPackage.reset();
  setSelectedPart(nullptr);
  mSelectedDevice = dev;

  if (mSelectedDevice) {
    FilePath pkgFp = mDb.getLatest<Package>(mSelectedDevice->getPackageUuid());
    if (pkgFp.isValid()) {
      mSelectedPackage = Package::open(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(TransactionalFileSystem::openRO(pkgFp))));
      QString devName = *mSelectedDevice->getNames().value(mLocaleOrder);
      QString pkgName = *mSelectedPackage->getNames().value(mLocaleOrder);
      if (devName.contains(pkgName, Qt::CaseInsensitive)) {
        // Package name is already contained in device name, no need to show it.
        mUi->lblDeviceName->setText(devName);
      } else {
        mUi->lblDeviceName->setText(QString("%1 [%2]").arg(devName, pkgName));
      }
      if (mSelectedPackage->getFootprints().count() > 0) {
        mPreviewFootprintGraphicsItem.reset(new FootprintGraphicsItem(
            mSelectedPackage->getFootprints().first(), *mLayers,
            Application::getDefaultStrokeFont(), &mSelectedPackage->getPads(),
            mSelectedComponent.get(), mLocaleOrder));
        mDevicePreviewScene->addItem(*mPreviewFootprintGraphicsItem);
        mUi->viewDevice->zoomAll();
      }
    }
  }
}

void AddComponentDialog::setSelectedPart(std::shared_ptr<const Part> part) {
  if (part && (part == mSelectedPart)) return;

  mSelectedPart = part;
}

void AddComponentDialog::addPartItem(std::shared_ptr<Part> part,
                                     QTreeWidgetItem* parent) {
  QString text = *part->getMpn();
  if (!part->getManufacturer()->isEmpty()) {
    text += " | " % *part->getManufacturer();
  }

  QTreeWidgetItem* item = new QTreeWidgetItem(parent);
  item->setIcon(0, EditorToolbox::svgIcon(":/fa/solid/cart-shopping.svg"));
  item->setText(0, text);
  item->setText(1, part->getAttributeValuesTr().join(", "));
  item->setToolTip(1, part->getAttributeKeyValuesTr().join("\n"));
  item->setTextAlignment(1, Qt::AlignRight);
  item->setData(0, Qt::UserRole, QVariant::fromValue(part));

  QFont font = item->font(1);
  font.setItalic(true);
  item->setFont(1, font);
}

void AddComponentDialog::schedulePartsInformationUpdate() noexcept {
  mUpdatePartInformationScheduled = true;
}

void AddComponentDialog::updatePartsInformation(int downloadDelayMs) noexcept {
  if (!mSettings.autofetchLivePartInformation.get()) {
    return;
  }

  if (!PartInformationProvider::instance().isOperational()) {
    PartInformationProvider::instance().startOperation();
    return;
  }

  const qint64 ts = QDateTime::currentMSecsSinceEpoch();
  if ((ts + downloadDelayMs) > mUpdatePartInformationDownloadStart) {
    mUpdatePartInformationDownloadStart = ts + downloadDelayMs;
  }
  const bool doRequest = (ts >= mUpdatePartInformationDownloadStart);

  mUi->treeComponents->setColumnHidden(2, false);
  mUpdatePartInformationScheduled = false;

  bool ok = true;
  const QRectF viewRect = mUi->treeComponents->viewport()->rect();
  for (int i = 0; (i < mUi->treeComponents->topLevelItemCount()) && ok; ++i) {
    QTreeWidgetItem* cmpItem = mUi->treeComponents->topLevelItem(i);
    if (!cmpItem->isExpanded()) continue;
    for (int k = 0; (k < cmpItem->childCount()) && ok; ++k) {
      QTreeWidgetItem* devItem = cmpItem->child(k);
      if (!devItem->isExpanded()) continue;
      for (int m = 0; (m < devItem->childCount()) && ok; ++m) {
        QTreeWidgetItem* partItem = devItem->child(m);
        const QRect rect = mUi->treeComponents->visualItemRect(partItem);
        if (rect.bottom() > viewRect.bottom()) {
          // End of view reached, all items below won't be visible.
          ok = false;
          break;
        }
        if (!rect.intersects(mUi->treeComponents->viewport()->rect())) {
          continue;
        }
        PartInformationDelegate::Data data =
            partItem->data(2, Qt::UserRole)
                .value<PartInformationDelegate::Data>();
        bool dataModified = false;
        if (!data.initialized) {
          if (auto partPtr = partItem->data(0, Qt::UserRole)
                                 .value<std::shared_ptr<Part>>()) {
            data.part.mpn = *partPtr->getMpn();
            data.part.manufacturer = *partPtr->getManufacturer();
          } else {
            qCritical() << "Failed to extract part from tree item.";
          }
          data.initialized = true;
          dataModified = true;
        }
        if ((!data.info) && (!data.part.mpn.isEmpty()) &&
            (!data.part.manufacturer.isEmpty())) {
          data.info =
              PartInformationProvider::instance().getPartInfo(data.part);
          if (data.info) dataModified = true;
          if ((!data.info) && (!data.infoRequested) && doRequest) {
            PartInformationProvider::instance().scheduleRequest(data.part);
            data.infoRequested = true;
            dataModified = true;
          }
          if ((!data.info) && (data.infoRequested || (!doRequest))) {
            if ((!doRequest) ||
                PartInformationProvider::instance().isOngoing(data.part)) {
              // Request is still ongoing.
              data.progress = mPartInfoProgress / 2;
              dataModified = true;
              mUpdatePartInformationScheduled = true;  // Require reload.
            } else {
              // Request failed.
              data.progress = 0;
              dataModified = true;
            }
          }
        }
        if (dataModified) {
          partItem->setData(2, Qt::UserRole, QVariant::fromValue(data));
        }
      }
    }
  }
  PartInformationProvider::instance().requestScheduledParts();
}

void AddComponentDialog::accept() noexcept {
  if ((!mSelectedComponent) || (!mSelectedSymbVar)) {
    QMessageBox::information(
        this, tr("Invalid Selection"),
        tr("Please select a component and a symbol variant."));
    return;
  }

  QDialog::accept();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

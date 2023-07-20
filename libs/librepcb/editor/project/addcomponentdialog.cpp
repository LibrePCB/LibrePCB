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
#include "../graphics/defaultgraphicslayerprovider.h"
#include "../graphics/graphicsscene.h"
#include "../library/pkg/footprintgraphicsitem.h"
#include "../library/sym/symbolgraphicsitem.h"
#include "../widgets/graphicsview.h"
#include "../widgets/waitingspinnerwidget.h"
#include "../workspace/categorytreemodel.h"
#include "ui_addcomponentdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/theme.h>
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

AddComponentDialog::AddComponentDialog(const WorkspaceLibraryDb& db,
                                       const QStringList& localeOrder,
                                       const QStringList& normOrder,
                                       const Theme& theme, QWidget* parent)
  : QDialog(parent),
    mDb(db),
    mLocaleOrder(localeOrder),
    mNormOrder(normOrder),
    mUi(new Ui::AddComponentDialog),
    mComponentPreviewScene(new GraphicsScene()),
    mDevicePreviewScene(new GraphicsScene()),
    mGraphicsLayerProvider(new DefaultGraphicsLayerProvider(theme)),
    mCategoryTreeModel(new CategoryTreeModel(
        mDb, mLocaleOrder, CategoryTreeModel::Filter::CmpCatWithComponents)),
    mCurrentSearchTerm(),
    mSelectedComponent(nullptr),
    mSelectedSymbVar(nullptr),
    mSelectedDevice(nullptr),
    mSelectedPackage(nullptr),
    mSelectedPart(nullptr),
    mPreviewFootprintGraphicsItem(nullptr) {
  mUi->setupUi(this);
  mUi->treeComponents->setColumnCount(2);
  mUi->treeComponents->header()->setStretchLastSection(false);
  mUi->treeComponents->header()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  mUi->treeComponents->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  mUi->lblCompDescription->hide();
  mUi->lblSymbVar->hide();
  mUi->cbxSymbVar->hide();
  connect(mUi->edtSearch, &QLineEdit::textChanged, this,
          &AddComponentDialog::searchEditTextChanged);
  connect(mUi->treeComponents, &QTreeWidget::currentItemChanged, this,
          &AddComponentDialog::treeComponents_currentItemChanged);
  connect(mUi->treeComponents, &QTreeWidget::itemDoubleClicked, this,
          &AddComponentDialog::treeComponents_itemDoubleClicked);
  connect(
      mUi->cbxSymbVar,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &AddComponentDialog::cbxSymbVar_currentIndexChanged);
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this, [this]() {
    // Update component tree view since there might be new DB entries. But for
    // now very fundamental since keeping the selection is not implemented yet.
    if ((!mCurrentSearchTerm.isEmpty()) &&
        (!mUi->treeComponents->currentItem())) {
      selectComponentByKeyword(mCurrentSearchTerm);
    }
  });

  // Add actions.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  addAction(cmd.find.createAction(this, this, [this]() {
    mUi->edtSearch->setFocus(Qt::ShortcutFocusReason);
  }));

  // Setup symbol graphics view.
  mUi->viewComponent->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->viewComponent->setGridStyle(theme.getBoardGridStyle());
  mUi->viewComponent->setOriginCrossVisible(false);
  mUi->viewComponent->setScene(mComponentPreviewScene.data());

  // Setup package graphics view.
  mUi->viewDevice->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->viewDevice->setGridStyle(theme.getBoardGridStyle());
  mUi->viewDevice->setOriginCrossVisible(false);
  mUi->viewDevice->setScene(mDevicePreviewScene.data());

  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &AddComponentDialog::treeCategories_currentItemChanged);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [&db](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&db, &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&db, &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(db.isScanInProgress());
  };
  addSpinner(mUi->treeCategories);
  addSpinner(mUi->treeComponents);

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

tl::optional<Package::AssemblyType>
    AddComponentDialog::getSelectedPackageAssemblyType() const noexcept {
  return (mSelectedComponent && mSelectedSymbVar && mSelectedDevice &&
          mSelectedPackage)
      ? tl::make_optional(mSelectedPackage->getAssemblyType(false))
      : tl::nullopt;
}

bool AddComponentDialog::getAutoOpenAgain() const noexcept {
  return mUi->cbxAddMore->isChecked();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AddComponentDialog::selectComponentByKeyword(
    const QString keyword, const tl::optional<Uuid>& selectedDevice) noexcept {
  try {
    searchComponents(keyword, selectedDevice, true);
  } catch (const Exception& e) {
    qCritical().noquote() << "Failed to pre-select component by keyword:"
                          << e.getMsg();
  }
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void AddComponentDialog::searchEditTextChanged(const QString& text) noexcept {
  try {
    QModelIndex catIndex = mUi->treeCategories->currentIndex();
    if (text.trimmed().isEmpty() && catIndex.isValid()) {
      setSelectedCategory(
          Uuid::tryFromString(catIndex.data(Qt::UserRole).toString()));
    } else {
      searchComponents(text.trimmed());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void AddComponentDialog::treeCategories_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);

  try {
    tl::optional<Uuid> categoryUuid =
        Uuid::tryFromString(current.data(Qt::UserRole).toString());
    setSelectedCategory(categoryUuid);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void AddComponentDialog::treeComponents_currentItemChanged(
    QTreeWidgetItem* current, QTreeWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
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
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    setSelectedComponent(nullptr);
  }
}

void AddComponentDialog::treeComponents_itemDoubleClicked(QTreeWidgetItem* item,
                                                          int column) noexcept {
  Q_UNUSED(column);
  if (item && item->parent())
    accept();  // only accept device items (not components)
}

void AddComponentDialog::cbxSymbVar_currentIndexChanged(int index) noexcept {
  if ((mSelectedComponent) && (index >= 0)) {
    tl::optional<Uuid> uuid =
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AddComponentDialog::searchComponents(
    const QString& input, const tl::optional<Uuid>& selectedDevice,
    bool selectFirstResult) {
  mCurrentSearchTerm = input;
  setSelectedComponent(nullptr);
  mUi->treeComponents->clear();

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
      cmpItem->setData(0, Qt::UserRole, cmpIt.key().toStr());
      for (auto devIt = cmpIt->devices.begin(); devIt != cmpIt->devices.end();
           ++devIt) {
        QTreeWidgetItem* devItem = new QTreeWidgetItem(cmpItem);
        devItem->setIcon(0, QIcon(":/img/library/device.png"));
        devItem->setText(0, devIt.value().name);
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
  } else if (selectFirstResult) {
    if (QTreeWidgetItem* cmpItem = mUi->treeComponents->topLevelItem(0)) {
      cmpItem->setExpanded(true);
      if (QTreeWidgetItem* devItem = cmpItem->child(0)) {
        mUi->treeComponents->setCurrentItem(devItem);
      } else {
        mUi->treeComponents->setCurrentItem(cmpItem);
      }
    }
  }
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
      parts = mDb.findPartsOfDevice(devUuid, input);  // can throw
    }
    foreach (const WorkspaceLibraryDb::Part& part, parts) {
      resDev.parts.append(std::make_shared<Part>(
          SimpleString(part.mpn), SimpleString(part.manufacturer),
          part.attributes));
    }
  }

  // Get name of elements.
  QMutableHashIterator<FilePath, SearchResultComponent> cmpIt(
      result.components);
  while (cmpIt.hasNext()) {
    cmpIt.next();
    mDb.getTranslations<Component>(cmpIt.key(), mLocaleOrder,
                                   &cmpIt.value().name);
    QMutableHashIterator<FilePath, SearchResultDevice> devIt(
        cmpIt.value().devices);
    while (devIt.hasNext()) {
      devIt.next();
      mDb.getTranslations<Device>(devIt.key(), mLocaleOrder,
                                  &devIt.value().name);
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
    const tl::optional<Uuid>& categoryUuid) {
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
    QTreeWidgetItem* cmpItem = new QTreeWidgetItem(mUi->treeComponents);
    cmpItem->setIcon(0, QIcon(":/img/library/symbol.png"));
    cmpItem->setText(0, cmpName);
    cmpItem->setData(0, Qt::UserRole, cmpFp.toStr());
    // devices
    QSet<Uuid> devices = mDb.getComponentDevices(cmpUuid);
    foreach (const Uuid& devUuid, devices) {
      try {
        FilePath devFp = mDb.getLatest<Device>(devUuid);
        if (!devFp.isValid()) continue;
        QString devName;
        mDb.getTranslations<Device>(devFp, mLocaleOrder, &devName);
        QTreeWidgetItem* devItem = new QTreeWidgetItem(cmpItem);
        devItem->setIcon(0, QIcon(":/img/library/device.png"));
        devItem->setText(0, devName);
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

  mUi->lblSymbVar->setVisible(mUi->cbxSymbVar->count() > 1);
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
          *symbol, *mGraphicsLayerProvider, mSelectedComponent,
          mSelectedSymbVar->getSymbolItems().get(item.getUuid()), mLocaleOrder);
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
            mSelectedPackage->getFootprints().first(), *mGraphicsLayerProvider,
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
  item->setIcon(0, QIcon(":/img/library/part.png"));
  item->setText(0, text);
  item->setText(1, part->getAttributeValuesTr().join(", "));
  item->setToolTip(1, part->getAttributeKeyValuesTr().join("\n"));
  item->setTextAlignment(1, Qt::AlignRight);
  item->setData(0, Qt::UserRole, QVariant::fromValue(part));

  QFont font = item->font(1);
  font.setItalic(true);
  item->setFont(1, font);
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

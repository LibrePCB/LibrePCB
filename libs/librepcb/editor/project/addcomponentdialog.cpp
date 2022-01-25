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

#include "../library/pkg/footprintpreviewgraphicsitem.h"
#include "../library/sym/symbolpreviewgraphicsitem.h"
#include "../widgets/graphicsview.h"
#include "../workspace/categorytreemodel.h"
#include "ui_addcomponentdialog.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
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
                                       QWidget* parent)
  : QDialog(parent),
    mDb(db),
    mLocaleOrder(localeOrder),
    mNormOrder(normOrder),
    mUi(new Ui::AddComponentDialog),
    mComponentPreviewScene(new GraphicsScene()),
    mDevicePreviewScene(new GraphicsScene()),
    mGraphicsLayerProvider(new DefaultGraphicsLayerProvider()),
    mCategoryTreeModel(new CategoryTreeModel(
        mDb, mLocaleOrder, CategoryTreeModel::Filter::CmpCatWithComponents)),
    mSelectedComponent(nullptr),
    mSelectedSymbVar(nullptr),
    mSelectedDevice(nullptr),
    mSelectedPackage(nullptr),
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

  mUi->viewComponent->setScene(mComponentPreviewScene.data());
  mUi->viewComponent->setOriginCrossVisible(false);

  mUi->viewDevice->setScene(mDevicePreviewScene.data());
  mUi->viewDevice->setOriginCrossVisible(false);
  mUi->viewDevice->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));

  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &AddComponentDialog::treeCategories_currentItemChanged);

  // Reset GUI to state of nothing selected
  setSelectedComponent(nullptr);

  // Restore client settings.
  QSettings clientSettings;
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

tl::optional<Uuid> AddComponentDialog::getSelectedComponentUuid() const
    noexcept {
  if (mSelectedComponent && mSelectedSymbVar)
    return mSelectedComponent->getUuid();
  else
    return tl::nullopt;
}

tl::optional<Uuid> AddComponentDialog::getSelectedSymbVarUuid() const noexcept {
  if (mSelectedComponent && mSelectedSymbVar)
    return mSelectedSymbVar->getUuid();
  else
    return tl::nullopt;
}

tl::optional<Uuid> AddComponentDialog::getSelectedDeviceUuid() const noexcept {
  if (mSelectedComponent && mSelectedSymbVar && mSelectedDevice)
    return mSelectedDevice->getUuid();
  else
    return tl::nullopt;
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
      QTreeWidgetItem* cmpItem =
          current->parent() ? current->parent() : current;
      FilePath cmpFp = FilePath(cmpItem->data(0, Qt::UserRole).toString());
      if ((!mSelectedComponent) ||
          (mSelectedComponent->getDirectory().getAbsPath() != cmpFp)) {
        Component* component = new Component(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(cmpFp))));
        setSelectedComponent(component);
      }
      if (current->parent()) {
        FilePath devFp = FilePath(current->data(0, Qt::UserRole).toString());
        if ((!mSelectedDevice) ||
            (mSelectedDevice->getDirectory().getAbsPath() != devFp)) {
          Device* device = new Device(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(
                  TransactionalFileSystem::openRO(devFp))));
          setSelectedDevice(device);
        }
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
      setSelectedSymbVar(
          mSelectedComponent->getSymbolVariants().find(*uuid).get());
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

void AddComponentDialog::searchComponents(const QString& input) {
  setSelectedComponent(nullptr);
  mUi->treeComponents->clear();

  // min. 2 chars to avoid freeze on entering first character due to huge result
  if (input.length() > 1) {
    SearchResult result = searchComponentsAndDevices(input);
    QHashIterator<FilePath, SearchResultComponent> cmpIt(result);
    while (cmpIt.hasNext()) {
      cmpIt.next();
      QTreeWidgetItem* cmpItem = new QTreeWidgetItem(mUi->treeComponents);
      cmpItem->setText(0, cmpIt.value().name);
      cmpItem->setData(0, Qt::UserRole, cmpIt.key().toStr());
      QHashIterator<FilePath, SearchResultDevice> devIt(cmpIt.value().devices);
      while (devIt.hasNext()) {
        devIt.next();
        QTreeWidgetItem* devItem = new QTreeWidgetItem(cmpItem);
        devItem->setText(0, devIt.value().name);
        devItem->setData(0, Qt::UserRole, devIt.key().toStr());
        devItem->setText(1, devIt.value().pkgName);
        devItem->setTextAlignment(1, Qt::AlignRight);
      }
      cmpItem->setText(1, QString("[%1]").arg(cmpIt.value().devices.count()));
      cmpItem->setTextAlignment(1, Qt::AlignRight);
      cmpItem->setExpanded(!cmpIt.value().match);
    }
  }

  mUi->treeComponents->sortByColumn(0, Qt::AscendingOrder);
}

AddComponentDialog::SearchResult AddComponentDialog::searchComponentsAndDevices(
    const QString& input) {
  SearchResult result;
  // add matching devices and their corresponding components
  QList<Uuid> devices = mDb.find<Device>(input);  // can throw
  foreach (const Uuid& devUuid, devices) {
    FilePath devFp = mDb.getLatest<Device>(devUuid);  // can throw
    if (!devFp.isValid()) continue;
    Uuid cmpUuid = Uuid::createRandom();
    Uuid pkgUuid = Uuid::createRandom();
    mDb.getDeviceMetadata(devFp, &cmpUuid,
                          &pkgUuid);  // can throw
    FilePath cmpFp = mDb.getLatest<Component>(cmpUuid);  // can throw
    if (!cmpFp.isValid()) continue;
    FilePath pkgFp = mDb.getLatest<Package>(pkgUuid);  // can throw
    SearchResultDevice& resDev = result[cmpFp].devices[devFp];
    resDev.pkgFp = pkgFp;
    resDev.match = true;
  }

  // add matching components and all their devices
  QList<Uuid> components = mDb.find<Component>(input);  // can throw
  foreach (const Uuid& cmpUuid, components) {
    FilePath cmpFp = mDb.getLatest<Component>(cmpUuid);  // can throw
    if (!cmpFp.isValid()) continue;
    QSet<Uuid> devices = mDb.getComponentDevices(cmpUuid);  // can throw
    SearchResultComponent& resCmp = result[cmpFp];
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
      resDev.pkgFp = pkgFp;
    }
  }

  // get name of elements
  QMutableHashIterator<FilePath, SearchResultComponent> resultIt(result);
  while (resultIt.hasNext()) {
    resultIt.next();
    mDb.getTranslations<Component>(resultIt.key(), mLocaleOrder,
                                   &resultIt.value().name);
    QMutableHashIterator<FilePath, SearchResultDevice> devIt(
        resultIt.value().devices);
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

  return result;
}

void AddComponentDialog::setSelectedCategory(
    const tl::optional<Uuid>& categoryUuid) {
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

void AddComponentDialog::setSelectedComponent(const Component* cmp) {
  if (cmp && (cmp == mSelectedComponent.data())) return;

  mUi->lblCompName->setText(tr("No component selected"));
  mUi->lblCompDescription->clear();
  mUi->cbxSymbVar->clear();
  setSelectedDevice(nullptr);
  setSelectedSymbVar(nullptr);
  mSelectedComponent.reset(cmp);

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
    const ComponentSymbolVariant* symbVar) {
  if (symbVar && (symbVar == mSelectedSymbVar)) return;
  mPreviewSymbolGraphicsItems.clear();
  mSelectedSymbVar = symbVar;

  if (mSelectedComponent && mSelectedSymbVar) {
    for (const ComponentSymbolVariantItem& item : symbVar->getSymbolItems()) {
      FilePath symbolFp = mDb.getLatest<Symbol>(item.getSymbolUuid());
      if (!symbolFp.isValid()) continue;  // TODO: show warning
      const Symbol* symbol = new Symbol(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(TransactionalFileSystem::openRO(
              symbolFp))));  // TODO: fix memory leak...
      auto graphicsItem = std::make_shared<SymbolPreviewGraphicsItem>(
          *mGraphicsLayerProvider, mLocaleOrder, *symbol,
          mSelectedComponent.data(), symbVar->getUuid(), item.getUuid());
      graphicsItem->setPos(item.getSymbolPosition().toPxQPointF());
      graphicsItem->setRotation(-item.getSymbolRotation().toDeg());
      mPreviewSymbolGraphicsItems.append(graphicsItem);
      mComponentPreviewScene->addItem(*graphicsItem);
      mUi->viewComponent->zoomAll();
    }
  }
}

void AddComponentDialog::setSelectedDevice(const Device* dev) {
  if (dev && (dev == mSelectedDevice.data())) return;

  mUi->lblDeviceName->setText(tr("No device selected"));
  mPreviewFootprintGraphicsItem.reset();
  mSelectedPackage.reset();
  mSelectedDevice.reset(dev);

  if (mSelectedDevice) {
    FilePath pkgFp = mDb.getLatest<Package>(mSelectedDevice->getPackageUuid());
    if (pkgFp.isValid()) {
      mSelectedPackage.reset(new Package(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(pkgFp)))));
      QString devName = *mSelectedDevice->getNames().value(mLocaleOrder);
      QString pkgName = *mSelectedPackage->getNames().value(mLocaleOrder);
      if (devName.contains(pkgName, Qt::CaseInsensitive)) {
        // Package name is already contained in device name, no need to show it.
        mUi->lblDeviceName->setText(devName);
      } else {
        mUi->lblDeviceName->setText(QString("%1 [%2]").arg(devName, pkgName));
      }
      if (mSelectedPackage->getFootprints().count() > 0) {
        mPreviewFootprintGraphicsItem.reset(new FootprintPreviewGraphicsItem(
            *mGraphicsLayerProvider, mLocaleOrder,
            *mSelectedPackage->getFootprints().first(), mSelectedPackage.data(),
            mSelectedComponent.data()));
        mDevicePreviewScene->addItem(*mPreviewFootprintGraphicsItem);
        mUi->viewDevice->zoomAll();
      }
    }
  }
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

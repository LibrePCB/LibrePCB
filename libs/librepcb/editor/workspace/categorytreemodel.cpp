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
#include "categorytreemodel.h"

#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CategoryTreeModel::CategoryTreeModel(const WorkspaceLibraryDb& library,
                                     const QStringList& localeOrder,
                                     Filters filters) noexcept
  : QAbstractItemModel(nullptr),
    mLibrary(library),
    mLocaleOrder(localeOrder),
    mFilters(filters),
    mRootItem(new Item{std::weak_ptr<Item>(), tl::nullopt, {}, {}, {}}) {
  update();
  connect(&mLibrary, &WorkspaceLibraryDb::scanSucceeded, this,
          &CategoryTreeModel::update);
}

CategoryTreeModel::~CategoryTreeModel() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

int CategoryTreeModel::columnCount(const QModelIndex& parent) const noexcept {
  Q_UNUSED(parent);
  return 1;
}

int CategoryTreeModel::rowCount(const QModelIndex& parent) const noexcept {
  const Item* item = itemFromIndex(parent);
  return item ? item->childs.count() : 0;
}

QModelIndex CategoryTreeModel::index(int row, int column,
                                     const QModelIndex& parent) const noexcept {
  Item* p = itemFromIndex(parent);
  if ((p) && (row >= 0) && (row < p->childs.count()) && (column == 0)) {
    return createIndex(row, column, p);
  } else {
    return QModelIndex();
  }
}

QModelIndex CategoryTreeModel::parent(const QModelIndex& index) const noexcept {
  if (index.isValid() && (index.model() == this)) {
    return indexFromItem(static_cast<Item*>(index.internalPointer()));
  } else {
    return QModelIndex();
  }
}

QVariant CategoryTreeModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const noexcept {
  if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal)) {
    switch (section) {
      case 0:
        return tr("Category");
      default:
        break;
    }
  }
  return QVariant();
}

QVariant CategoryTreeModel::data(const QModelIndex& index, int role) const
    noexcept {
  if (const Item* item = itemFromIndex(index)) {
    switch (role) {
      case Qt::DisplayRole:
        return item->text;
      case Qt::ToolTipRole:
        return item->tooltip;
      case Qt::UserRole:
        return item->uuid ? item->uuid->toStr() : QString();
      default:
        break;
    }
  }
  return QVariant();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CategoryTreeModel::update() noexcept {
  qDebug() << "CategoryTreeModel update started.";
  QElapsedTimer t;
  t.start();

  // Determine new items.
  QVector<std::shared_ptr<Item>> items = getChilds(nullptr);

  // Add virtual category for library elements with no category assigned.
  try {
    if (containsItems(tl::nullopt)) {
      items.append(std::shared_ptr<Item>(
          new Item{std::weak_ptr<Item>(),
                   tl::nullopt,
                   tr("(Without Category)"),
                   tr("All library elements without a category"),
                   {}}));
    }
  } catch (const Exception& e) {
    qCritical() << "CategoryTreeModel failed to update items:" << e.getMsg();
  }

  // Update tree with new items in a way which keeps the selection in views.
  updateModelItem(mRootItem, items);

  qDebug() << "CategoryTreeModel update finished in" << t.elapsed() << "ms.";
}

QVector<std::shared_ptr<CategoryTreeModel::Item>> CategoryTreeModel::getChilds(
    std::shared_ptr<Item> parent) const noexcept {
  QVector<std::shared_ptr<Item>> childs;
  tl::optional<Uuid> parentUuid = parent ? parent->uuid : tl::nullopt;
  try {
    QSet<Uuid> uuids = listPackageCategories()
        ? mLibrary.getChilds<PackageCategory>(parentUuid)
        : mLibrary.getChilds<ComponentCategory>(parentUuid);
    foreach (const Uuid& uuid, uuids) {
      std::shared_ptr<Item> child(
          new Item{parent, uuid, QString(), QString(), {}});
      child->childs = getChilds(child);
      if (!child->childs.isEmpty() || listAll() || containsItems(uuid)) {
        FilePath fp = listPackageCategories()
            ? mLibrary.getLatest<PackageCategory>(uuid)
            : mLibrary.getLatest<ComponentCategory>(uuid);
        if (fp.isValid()) {
          if (listPackageCategories()) {
            mLibrary.getTranslations<PackageCategory>(
                fp, mLocaleOrder, &child->text, &child->tooltip);
          } else {
            mLibrary.getTranslations<ComponentCategory>(
                fp, mLocaleOrder, &child->text, &child->tooltip);
          }
        }
        childs.append(child);
      }
    }
  } catch (const Exception& e) {
    qCritical() << "CategoryTreeModel failed to update items:" << e.getMsg();
  }

  // Sort items by text.
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(
      childs.begin(), childs.end(),
      [&collator](const std::shared_ptr<Item>& a, std::shared_ptr<Item>& b) {
        return collator(a->text, b->text);
      });

  return childs;
}

bool CategoryTreeModel::containsItems(const tl::optional<Uuid>& uuid) const {
  if (listPackageCategories()) {
    if (mFilters.testFlag(Filter::PkgCatWithPackages) &&
        (mLibrary.getByCategory<Package>(uuid, 1).count() > 0)) {
      return true;
    }
  } else {
    if (mFilters.testFlag(Filter::CmpCatWithSymbols) &&
        (mLibrary.getByCategory<Symbol>(uuid, 1).count() > 0)) {
      return true;
    }
    if (mFilters.testFlag(Filter::CmpCatWithComponents) &&
        (mLibrary.getByCategory<Component>(uuid, 1).count() > 0)) {
      return true;
    }
    if (mFilters.testFlag(Filter::CmpCatWithDevices) &&
        (mLibrary.getByCategory<Device>(uuid, 1).count() > 0)) {
      return true;
    }
  }
  return false;
}

bool CategoryTreeModel::listAll() const noexcept {
  return mFilters.testFlag(Filter::PkgCat) || mFilters.testFlag(Filter::CmpCat);
}

bool CategoryTreeModel::listPackageCategories() const noexcept {
  return mFilters.testFlag(Filter::PkgCat) ||
      mFilters.testFlag(Filter::PkgCatWithPackages);
}

void CategoryTreeModel::updateModelItem(
    std::shared_ptr<Item> parentItem,
    const QVector<std::shared_ptr<Item>>& newChilds) noexcept {
  // Update parent of new items and get list of UUIDs.
  QSet<tl::optional<Uuid>> newUUids;
  foreach (std::shared_ptr<Item> item, newChilds) {
    newUUids.insert(item->uuid);
    item->parent = parentItem;
  }

  // Step 1: Remove no longer exsting categories from model.
  for (int i = parentItem->childs.count() - 1; i >= 0; --i) {
    if (!newUUids.contains(parentItem->childs.at(i)->uuid)) {
      QModelIndex idx = indexFromItem(parentItem.get());
      Q_ASSERT(idx.isValid() != (parentItem == mRootItem));
      beginRemoveRows(idx, i, i);
      parentItem->childs.removeAt(i);
      endRemoveRows();
    }
  }

  // Step 2: Add new categories to model and update existing categories.
  for (int i = 0; i < newChilds.count(); ++i) {
    std::shared_ptr<Item> item = parentItem->childs.value(i);  // Might be null.
    std::shared_ptr<Item> newItem = newChilds.at(i);
    if ((!item) || (item->uuid != newItem->uuid)) {
      QModelIndex idx = indexFromItem(parentItem.get());
      Q_ASSERT(idx.isValid() != (parentItem == mRootItem));
      beginInsertRows(idx, i, i);
      parentItem->childs.insert(i, newItem);
      endInsertRows();
    } else {
      if ((item->text != newItem->text) ||
          (item->tooltip != newItem->tooltip)) {
        item->text = newItem->text;
        item->tooltip = newItem->tooltip;
        QModelIndex idx = indexFromItem(item.get());
        Q_ASSERT(idx.isValid());
        emit dataChanged(idx, idx);
      }
      updateModelItem(item, newItem->childs);
    }
  }

  Q_ASSERT(parentItem->childs.count() == newChilds.count());
}

CategoryTreeModel::Item* CategoryTreeModel::itemFromIndex(
    const QModelIndex& index) const noexcept {
  if (!index.isValid()) {
    return mRootItem.get();
  } else if (index.model() != this) {
    return nullptr;
  } else if (index.column() != 0) {
    return nullptr;
  } else if (Item* parent = static_cast<Item*>(index.internalPointer())) {
    return parent->childs.value(index.row()).get();
  } else {
    return nullptr;
  }
}

QModelIndex CategoryTreeModel::indexFromItem(const Item* item) const noexcept {
  if (std::shared_ptr<Item> parent = (item ? item->parent.lock() : nullptr)) {
    for (int i = 0; i < parent->childs.count(); ++i) {
      if (parent->childs.at(i).get() == item) {
        return createIndex(i, 0, parent.get());
      }
    }
  }
  return QModelIndex();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

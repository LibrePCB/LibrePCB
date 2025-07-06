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
#include "categorytreemodellegacy.h"

#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/utils/toolbox.h>
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

CategoryTreeModelLegacy::CategoryTreeModelLegacy(
    const WorkspaceLibraryDb& library, const QStringList& localeOrder,
    Filters filters) noexcept
  : QAbstractItemModel(nullptr),
    mLibrary(library),
    mLocaleOrder(localeOrder),
    mFilters(filters),
    mRootItem(new Item{std::weak_ptr<Item>(), std::nullopt, {}, {}, {}}) {
  update();
  connect(&mLibrary, &WorkspaceLibraryDb::scanSucceeded, this,
          &CategoryTreeModelLegacy::update);
}

CategoryTreeModelLegacy::~CategoryTreeModelLegacy() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CategoryTreeModelLegacy::setLocaleOrder(
    const QStringList& order) noexcept {
  if (order != mLocaleOrder) {
    mLocaleOrder = order;
    update();
  }
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

int CategoryTreeModelLegacy::columnCount(
    const QModelIndex& parent) const noexcept {
  Q_UNUSED(parent);
  return 1;
}

int CategoryTreeModelLegacy::rowCount(
    const QModelIndex& parent) const noexcept {
  const Item* item = itemFromIndex(parent);
  return item ? item->childs.count() : 0;
}

QModelIndex CategoryTreeModelLegacy::index(
    int row, int column, const QModelIndex& parent) const noexcept {
  Item* p = itemFromIndex(parent);
  if ((p) && (row >= 0) && (row < p->childs.count()) && (column == 0)) {
    return createIndex(row, column, p);
  } else {
    return QModelIndex();
  }
}

QModelIndex CategoryTreeModelLegacy::parent(
    const QModelIndex& index) const noexcept {
  if (index.isValid() && (index.model() == this)) {
    return indexFromItem(static_cast<Item*>(index.internalPointer()));
  } else {
    return QModelIndex();
  }
}

QVariant CategoryTreeModelLegacy::headerData(int section,
                                             Qt::Orientation orientation,
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

QVariant CategoryTreeModelLegacy::data(const QModelIndex& index,
                                       int role) const noexcept {
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

void CategoryTreeModelLegacy::update() noexcept {
  qDebug() << "Update category tree model...";
  QElapsedTimer t;
  t.start();

  // Determine new items.
  QVector<std::shared_ptr<Item>> items = getChilds(nullptr);

  // Add virtual category for library elements with no category assigned.
  try {
    if (containsItems(std::nullopt)) {
      items.append(std::shared_ptr<Item>(
          new Item{std::weak_ptr<Item>(),
                   std::nullopt,
                   tr("(Without Category)"),
                   tr("All library elements without a category"),
                   {}}));
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to update category tree model:" << e.getMsg();
  }

  // Update tree with new items in a way which keeps the selection in views.
  updateModelItem(mRootItem, items);

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";
}

QVector<std::shared_ptr<CategoryTreeModelLegacy::Item>>
    CategoryTreeModelLegacy::getChilds(
        std::shared_ptr<Item> parent) const noexcept {
  QVector<std::shared_ptr<Item>> childs;
  std::optional<Uuid> parentUuid = parent ? parent->uuid : std::nullopt;
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
    qCritical() << "Failed to update category tree model items:" << e.getMsg();
  }

  // Sort items by text.
  Toolbox::sortNumeric(
      childs,
      [](const QCollator& cmp, const std::shared_ptr<Item>& lhs,
         const std::shared_ptr<Item>& rhs) {
        return cmp(lhs->text, rhs->text);
      },
      Qt::CaseInsensitive, false);

  return childs;
}

bool CategoryTreeModelLegacy::containsItems(
    const std::optional<Uuid>& uuid) const {
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

bool CategoryTreeModelLegacy::listAll() const noexcept {
  return mFilters.testFlag(Filter::PkgCat) || mFilters.testFlag(Filter::CmpCat);
}

bool CategoryTreeModelLegacy::listPackageCategories() const noexcept {
  return mFilters.testFlag(Filter::PkgCat) ||
      mFilters.testFlag(Filter::PkgCatWithPackages);
}

void CategoryTreeModelLegacy::updateModelItem(
    std::shared_ptr<Item> parentItem,
    const QVector<std::shared_ptr<Item>>& newChilds) noexcept {
  for (int i = 0; i < newChilds.count(); ++i) {
    std::shared_ptr<Item> item = parentItem->childs.value(i);  // Might be null.
    std::shared_ptr<Item> newItem = newChilds.at(i);
    if (item) {
      // Update existing item.
      if ((item->uuid != newItem->uuid) || (item->text != newItem->text) ||
          (item->tooltip != newItem->tooltip)) {
        item->uuid = newItem->uuid;
        item->text = newItem->text;
        item->tooltip = newItem->tooltip;
        QModelIndex idx = indexFromItem(item.get());
        Q_ASSERT(idx.isValid());
        emit dataChanged(idx, idx);
      }
      updateModelItem(item, newItem->childs);
    } else {
      // Add new item.
      newItem->parent = parentItem;  // Update parent of item.
      QModelIndex idx = indexFromItem(parentItem.get());
      Q_ASSERT(idx.isValid() != (parentItem == mRootItem));
      beginInsertRows(idx, i, i);
      parentItem->childs.insert(i, newItem);
      endInsertRows();
    }
  }

  // Remove no longer existing items.
  const int removeCount = parentItem->childs.count() - newChilds.count();
  if (removeCount > 0) {
    QModelIndex idx = indexFromItem(parentItem.get());
    Q_ASSERT(idx.isValid() != (parentItem == mRootItem));
    const int removeFrom = newChilds.count();
    beginRemoveRows(idx, removeFrom, removeFrom + removeCount - 1);
    parentItem->childs.remove(removeFrom, removeCount);
    endRemoveRows();
  }

  // Sanity check that the number of childs is now correct.
  Q_ASSERT(parentItem->childs.count() == newChilds.count());
}

CategoryTreeModelLegacy::Item* CategoryTreeModelLegacy::itemFromIndex(
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

QModelIndex CategoryTreeModelLegacy::indexFromItem(
    const Item* item) const noexcept {
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

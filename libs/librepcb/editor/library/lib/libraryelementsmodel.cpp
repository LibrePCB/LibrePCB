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
#include "libraryelementsmodel.h"

#include "../../utils/slinthelpers.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementsModel::LibraryElementsModel(const Workspace& ws,
                                           const FilePath& libFp,
                                           QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mDb(mWorkspace.getLibraryDb()),
    mLocaleOrder(mWorkspace.getSettings().libraryLocaleOrder.get()),
    mLibPath(libFp),
    mRoot(new TreeItem{TreeItemType::Root,
                       std::weak_ptr<TreeItem>(),
                       std::nullopt,
                       {},
                       {},
                       {}}),
    mElementsModel(new slint::VectorModel<ui::TreeViewItemData>()) {
  refresh();
}

LibraryElementsModel::~LibraryElementsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LibraryElementsModel::setSelectedCategory(
    const std::optional<Uuid>& uuid) noexcept {
  if (uuid == mSelectedCategory) return;

  mSelectedCategory = uuid;
  mElementsModel->clear();
  if (!uuid) return;
  std::optional<TreeItemType> prevType;
  if (auto item = mCategories.value(*uuid)) {
    for (auto child : item->childs) {
      if (child->type != TreeItemType::ComponentCategory) {
        QIcon icon;
        QString category;
        switch (child->type) {
          case TreeItemType::ComponentCategory: {
            icon = QIcon(":/img/places/folder.png");
            break;
          }
          case TreeItemType::Symbol: {
            icon = QIcon(":/img/library/symbol.png");
            category = "Symbols";
            break;
          }
          case TreeItemType::Component: {
            icon = QIcon(":/img/library/component.png");
            category = "Components";
            break;
          }
          case TreeItemType::Device: {
            icon = QIcon(":/img/library/device.png");
            category = "Devices";
            break;
          }
          default: {
            break;
          }
        }
        if (child->type != prevType) {
          mElementsModel->push_back(ui::TreeViewItemData{
              0,  // Level
              slint::Image(),  // Icon
              q2s(category),  // Text
              slint::SharedString(),  // Hint
              slint::SharedString(),  // User data
              false,  // Is project file or folder
              false,  // Has children
              false,  // Expanded
              false,  // Supports pinning
              false,  // Pinned
              ui::TreeViewItemAction::None,  // Action
          });
          prevType = child->type;
        }
        mElementsModel->push_back(ui::TreeViewItemData{
            1,  // Level
            q2s(icon.pixmap(32)),  // Icon
            q2s(child->text),  // Text
            q2s(child->tooltip),  // Hint
            child->uuid ? q2s(child->uuid->toStr())
                        : slint::SharedString(),  // User data
            false,  // Is project file or folder
            false,  // Has children
            false,  // Expanded
            false,  // Supports pinning
            false,  // Pinned
            ui::TreeViewItemAction::None,  // Action
        });
      }
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibraryElementsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::TreeViewItemData> LibraryElementsModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void LibraryElementsModel::set_row_data(
    std::size_t i, const ui::TreeViewItemData& data) noexcept {
  Q_UNUSED(i);
  Q_UNUSED(data);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryElementsModel::refresh() noexcept {
  qDebug() << "Update library elements tree...";
  QElapsedTimer t;
  t.start();

  // Determine new items.
  // QVector<std::shared_ptr<TreeItem>> items = getChilds(nullptr);

  // Add virtual category for library elements with no category assigned.
  try {
    // if (containsItems(std::nullopt)) {
    //   items.append(std::shared_ptr<TreeItem>(
    //       new Item{std::weak_ptr<TreeItem>(),
    //                std::nullopt,
    //                tr("(Without Category)"),
    //                tr("All library elements without a category"),
    //                {}}));
    // }
  } catch (const Exception& e) {
    qCritical() << "Failed to update library elements tree:" << e.getMsg();
  }

  // Update tree with new items in a way which keeps the selection in views.
  // updateModelItem(mRoot, items);
  // mRoot->childs = items;

  mItems.clear();
  mLibCategories.clear();
  loadCategories<ComponentCategory>();
  loadElements<Symbol, ComponentCategory>(TreeItemType::Symbol);
  loadElements<Component, ComponentCategory>(TreeItemType::Component);
  loadElements<Device, ComponentCategory>(TreeItemType::Device);
  addChildsToModel(*mRoot, 0);
  notify_reset();

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";
}

template <typename CategoryType>
void LibraryElementsModel::loadCategories() {
  try {
    mLibCategories = mDb.getAll<CategoryType>(mLibPath);
    for (auto it = mLibCategories.begin(); it != mLibCategories.end(); it++) {
      getOrCreateCategory<CategoryType>(*it);
    }
  } catch (const Exception& e) {
    // TODO
  }
}

template <typename CategoryType>
std::shared_ptr<LibraryElementsModel::TreeItem>
    LibraryElementsModel::getOrCreateCategory(const Uuid& uuid) {
  auto it = mCategories.find(uuid);
  if (it != mCategories.end()) {
    return *it;
  }

  auto item = std::make_shared<TreeItem>();
  item->type = TreeItemType::ComponentCategory;
  item->uuid = uuid;
  try {
    FilePath fp = mLibCategories.key(uuid);
    if (!fp.isValid()) {
      fp = mDb.getLatest<CategoryType>(uuid);
    }
    mDb.getTranslations<CategoryType>(fp, mLocaleOrder, &item->text,
                                      &item->tooltip);
    std::optional<Uuid> parentUuid;
    mDb.getCategoryMetadata<CategoryType>(fp, &parentUuid);
    auto parent = mRoot;
    if (parentUuid) {
      if (auto p = getOrCreateCategory<CategoryType>(*parentUuid)) {
        parent = p;
      }
    }
    item->parent = parent;
    parent->childs.append(item);
  } catch (const Exception& e) {
    // TODO
  }
  mCategories.insert(uuid, item);
  return item;
}

template <typename ElementType, typename CategoryType>
void LibraryElementsModel::loadElements(TreeItemType type) {
  try {
    const QSet<FilePath> elements =
        Toolbox::toSet(mDb.getAll<ElementType>(mLibPath).keys());
    for (const FilePath& fp : elements) {
      for (const Uuid& catUuid : mDb.getCategoriesOf<ElementType>(fp)) {
        if (auto cat = getOrCreateCategory<CategoryType>(catUuid)) {
          auto item = std::make_shared<TreeItem>();
          item->type = type;
          // item->uuid = uuid;
          mDb.getTranslations<ElementType>(fp, mLocaleOrder, &item->text,
                                           &item->tooltip);
          cat->childs.push_back(item);
          item->parent = cat;
        }
      }
    }
  } catch (const Exception& e) {
    // TODO
  }
}

void LibraryElementsModel::addChildsToModel(TreeItem& item,
                                            int level) noexcept {
  // Sort items.
  Toolbox::sortNumeric(
      item.childs,
      [](const QCollator& cmp, const std::shared_ptr<TreeItem>& lhs,
         const std::shared_ptr<TreeItem>& rhs) {
        if (lhs->type != rhs->type) {
          return static_cast<int>(lhs->type) < static_cast<int>(rhs->type);
        } else {
          return cmp(lhs->text, rhs->text);
        }
      },
      Qt::CaseInsensitive, false);

  for (auto child : item.childs) {
    QIcon icon;
    switch (child->type) {
      case TreeItemType::ComponentCategory: {
        icon = QIcon(":/img/places/folder.png");
        break;
      }
      case TreeItemType::Symbol: {
        icon = QIcon(":/img/library/symbol.png");
        break;
      }
      case TreeItemType::Component: {
        icon = QIcon(":/img/library/component.png");
        break;
      }
      case TreeItemType::Device: {
        icon = QIcon(":/img/library/device.png");
        break;
      }
      default: {
        break;
      }
    }
    const int elementsCount =
        std::count_if(child->childs.begin(), child->childs.end(),
                      [](const std::shared_ptr<TreeItem>& item) {
                        return item->type != TreeItemType::ComponentCategory;
                      });
    if (child->type == TreeItemType::ComponentCategory) {
      mItems.push_back(ui::TreeViewItemData{
          level,  // Level
          slint::Image(),  // q2s(icon.pixmap(32)),  // Icon
          q2s(child->text),  // Text
          //(elementsCount > 0) ? q2s(QString::number(elementsCount))
          //                    : slint::SharedString(),  // Comment
          q2s(child->tooltip),  // Hint
          child->uuid ? q2s(child->uuid->toStr())
                      : slint::SharedString(),  // User data
          false,  // Is project file or folder
          false,  // Has children
          false,  // Expanded
          false,  // Supports pinning
          false,  // Pinned
          ui::TreeViewItemAction::None,  // Action
      });
    }
    addChildsToModel(*child, level + 1);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

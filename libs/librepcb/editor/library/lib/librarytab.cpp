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
#include "librarytab.h"

#include "../libraryeditor2.h"
#include "libraryelementsmodel.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/library/library.h>
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

LibraryTab::LibraryTab(GuiApplication& app, LibraryEditor2& editor,
                       QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mLibrary(mEditor.getLibrary()),
    mDb(editor.getWorkspace().getLibraryDb()),
    mLocaleOrder(editor.getWorkspace().getSettings().libraryLocaleOrder.get()),
    mLibPath(mLibrary.getDirectory().getAbsPath()),
    mCmpCatElementCount(0),
    mPkgCatElementCount(0),
    mCategories(new slint::VectorModel<ui::TreeViewItemData>()),
    mCurrentCategoryIndex(0),
    mFilteredElements(new slint::VectorModel<ui::TreeViewItemData>()),
    mCurrentElementIndex(-1) {
  refreshLibElements();
  setSelectedCategory(std::nullopt);
}

LibraryTab::~LibraryTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int LibraryTab::getLibraryIndex() const noexcept {
  return mEditor.getUiIndex();
}

ui::TabData LibraryTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Library,  // Type
      q2s(*mLibrary.getNames().getDefaultValue()),  // Title
      ui::TabFeatures{},  // Features
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::LibraryTabData LibraryTab::getDerivedUiData() const noexcept {
  return ui::LibraryTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mLibrary.getIconAsPixmap()),  // Icon
      q2s(*mLibrary.getNames().getDefaultValue()),  // Name
      q2s(mLibrary.getDescriptions().getDefaultValue()),  // Description
      nullptr,  // Keywords
      q2s(mLibrary.getAuthor()),  // Author
      q2s(mLibrary.getVersion().toStr()),  // Version
      mLibrary.isDeprecated(),  // Deprecated
      q2s(mLibrary.getUrl().toString()),  // URL
      nullptr,  // Dependencies
      q2s(*mLibrary.getManufacturer()),  // Manufacturer
      mCategories,  // Component categories
      mCurrentCategoryIndex,  // Current category index
      mFilteredElements,  // Filtered elements
      mCurrentElementIndex,  // Current element index
  };
}

void LibraryTab::setDerivedUiData(const ui::LibraryTabData& data) noexcept {
  if (data.categories_index != mCurrentCategoryIndex) {
    mCurrentCategoryIndex = data.categories_index;
    setSelectedCategory(mCategories->row_data(mCurrentCategoryIndex));
  }
  mCurrentElementIndex = data.element_index;
  onDerivedUiDataChanged.notify();
}

void LibraryTab::trigger(ui::TabAction a) noexcept {
  auto data = (mCurrentElementIndex == -1)
      ? mCategories->row_data(mCurrentCategoryIndex)
      : mFilteredElements->row_data(mCurrentElementIndex);
  const QString userData = data ? s2q(data->user_data) : QString();
  const std::optional<Uuid> uuid = Uuid::tryFromString(userData);
  const FilePath fp = uuid ? mLibCategories.key(*uuid) : FilePath(userData);

  switch (a) {
    case ui::TabAction::Delete: {
      qDebug() << "Delete" << fp;
      break;
    }
    case ui::TabAction::EditProperties: {
      qDebug() << "Open" << fp;
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryTab::refreshLibElements() noexcept {
  qDebug() << "Update library elements tree...";
  QElapsedTimer t;
  t.start();

  mLibCategories.clear();
  mLibElementsMap.clear();
  mCmpCatElementCount = 0;
  mPkgCatElementCount = 0;

  mCmpCatRoot = createRootItem(TreeItemType::ComponentCategory,
                               tr("Component Categories"));
  mPkgCatRoot =
      createRootItem(TreeItemType::PackageCategory, tr("Package Categories"));

  loadCategories<ComponentCategory>(TreeItemType::ComponentCategory,
                                    QIcon(":/img/places/folder.png"),
                                    *mCmpCatRoot);
  loadCategories<PackageCategory>(TreeItemType::PackageCategory,
                                  QIcon(":/img/places/folder_green.png"),
                                  *mPkgCatRoot);

  loadElements<Symbol, ComponentCategory>(
      TreeItemType::Symbol, q2s(QIcon(":/img/library/symbol.png").pixmap(32)),
      TreeItemType::ComponentCategory, QIcon(":/img/places/folder.png"),
      *mCmpCatRoot, mCmpCatElementCount);
  loadElements<Package, PackageCategory>(
      TreeItemType::Package, q2s(QIcon(":/img/library/package.png").pixmap(32)),
      TreeItemType::PackageCategory, QIcon(":/img/places/folder_green.png"),
      *mPkgCatRoot, mPkgCatElementCount);
  loadElements<Component, ComponentCategory>(
      TreeItemType::Component,
      q2s(QIcon(":/img/library/component.png").pixmap(32)),
      TreeItemType::ComponentCategory, QIcon(":/img/places/folder.png"),
      *mCmpCatRoot, mCmpCatElementCount);
  loadElements<Device, ComponentCategory>(
      TreeItemType::Device, q2s(QIcon(":/img/library/device.png").pixmap(32)),
      TreeItemType::ComponentCategory, QIcon(":/img/places/folder.png"),
      *mCmpCatRoot, mCmpCatElementCount);

  for (auto& root : {mCmpCatRoot, mPkgCatRoot}) {
    auto uncategorized = root->childs.takeAt(0);
    sortItemsRecursive(root->childs);
    if (!uncategorized->childs.isEmpty()) {
      root->childs.insert(0, uncategorized);
    }
  }

  mCategories->clear();
  const int count = mCmpCatElementCount + mPkgCatElementCount;
  mCategories->push_back(ui::TreeViewItemData{
      0,  // Level
      slint::Image(),  // Icon
      q2s(tr("All Library Elements")),  // Text
      (count > 0) ? q2s(QString::number(count))
                  : slint::SharedString(),  // Comment
      slint::SharedString(),  // Hint
      false,  // Italic
      true,  // Bold
      slint::SharedString(),  // User data
      false,  // Is project file or folder
      false,  // Has children
      false,  // Expanded
      false,  // Supports pinning
      false,  // Pinned
      ui::TreeViewItemAction::None,  // Action
  });
  addCategoriesToModel(TreeItemType::ComponentCategory, *mCmpCatRoot,
                       mCmpCatElementCount);
  addCategoriesToModel(TreeItemType::PackageCategory, *mPkgCatRoot,
                       mPkgCatElementCount);

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";
}

std::shared_ptr<LibraryTab::TreeItem> LibraryTab::createRootItem(
    TreeItemType type, const QString& text) noexcept {
  Uuid uuid = Uuid::createRandom();
  std::shared_ptr<TreeItem> root(new TreeItem{
      type,
      slint::Image(),
      text,
      uuid.toStr(),
      false,
      {},
  });
  mLibElementsMap.insert(uuid, root);

  uuid = Uuid::createRandom();
  std::shared_ptr<TreeItem> uncategorized(new TreeItem{
      type,
      q2s(QIcon(":/img/status/dialog_warning.png").pixmap(32)),
      tr("Not Categorized"),
      uuid.toStr(),
      false,
      {},
  });
  root->childs.append(uncategorized);
  mLibElementsMap.insert(uuid, uncategorized);
  return root;
}

template <typename CategoryType>
void LibraryTab::loadCategories(TreeItemType type, const QIcon& icon,
                                TreeItem& root) {
  try {
    const auto categories = mDb.getAll<CategoryType>(mLibPath);
    mLibCategories.insert(categories);
    for (auto it = categories.begin(); it != categories.end(); it++) {
      getOrCreateCategory<CategoryType>(type, icon, *it, root);
    }
  } catch (const Exception& e) {
    // TODO
  }
}

template <typename CategoryType>
std::shared_ptr<LibraryTab::TreeItem> LibraryTab::getOrCreateCategory(
    TreeItemType type, const QIcon& icon, const Uuid& uuid, TreeItem& root) {
  auto it = mLibElementsMap.find(uuid);
  if (it != mLibElementsMap.end()) {
    return *it;
  }

  auto item = std::make_shared<TreeItem>();
  item->type = type;
  item->userData = uuid.toStr();
  item->fromOtherLib = false;
  try {
    FilePath fp = mLibCategories.key(uuid);
    if (!fp.isValid()) {
      fp = mDb.getLatest<CategoryType>(uuid);
      item->fromOtherLib = true;
      item->icon = q2s(icon.pixmap(32, QIcon::Disabled));
    } else {
      item->icon = q2s(icon.pixmap(32, QIcon::Normal));
    }
    if ((!fp.isValid()) ||
        (!mDb.getTranslations<CategoryType>(fp, mLocaleOrder, &item->text))) {
      item->text = tr("Unknown") % " (" % uuid.toStr() % ")";
    }
    std::optional<Uuid> parentUuid;
    if (fp.isValid()) {
      mDb.getCategoryMetadata<CategoryType>(fp, &parentUuid);
    }
    auto parent = &root;
    if (parentUuid) {
      if (auto p = getOrCreateCategory<CategoryType>(type, icon, *parentUuid,
                                                     root)) {
        parent = p.get();
      }
    }
    parent->childs.append(item);
  } catch (const Exception& e) {
    // TODO
  }
  mLibElementsMap.insert(uuid, item);
  return item;
}

template <typename ElementType, typename CategoryType>
void LibraryTab::loadElements(TreeItemType type, slint::Image icon,
                              TreeItemType catType, const QIcon& catIcon,
                              TreeItem& root, int& count) {
  try {
    const QSet<FilePath> elements =
        Toolbox::toSet(mDb.getAll<ElementType>(mLibPath).keys());
    count += elements.count();
    for (const FilePath& fp : elements) {
      auto item = std::make_shared<TreeItem>();
      item->type = type;
      item->icon = icon;
      item->userData = fp.toStr();
      item->fromOtherLib = false;
      mDb.getTranslations<ElementType>(fp, mLocaleOrder, &item->text);

      bool addedToCategory = false;
      for (const Uuid& catUuid : mDb.getCategoriesOf<ElementType>(fp)) {
        if (auto cat = getOrCreateCategory<CategoryType>(catType, catIcon,
                                                         catUuid, root)) {
          cat->childs.push_back(item);
          addedToCategory = true;
        }
      }
      if (!addedToCategory) {
        auto uncategorized = root.childs.at(0);
        Q_ASSERT(uncategorized);
        uncategorized->childs.push_back(item);
      }
    }
  } catch (const Exception& e) {
    // TODO
  }
}

void LibraryTab::sortItemsRecursive(
    QVector<std::shared_ptr<TreeItem>>& items) noexcept {
  Toolbox::sortNumeric(
      items,
      [](const QCollator& cmp, const std::shared_ptr<TreeItem>& lhs,
         const std::shared_ptr<TreeItem>& rhs) {
        if (lhs->type != rhs->type) {
          return static_cast<int>(lhs->type) < static_cast<int>(rhs->type);
        } else {
          return cmp(lhs->text, rhs->text);
        }
      },
      Qt::CaseInsensitive, false);
  for (auto child : items) {
    sortItemsRecursive(child->childs);
  }
}

void LibraryTab::addCategoriesToModel(TreeItemType type, TreeItem& root,
                                      int count) noexcept {
  mCategories->push_back(ui::TreeViewItemData{
      0,  // Level
      root.icon,  // Icon
      q2s(root.text),  // Text
      (count > 0) ? q2s(QString::number(count))
                  : slint::SharedString(),  // Comment
      slint::SharedString(),  // Hint
      false,  // Italic
      true,  // Bold
      q2s(root.userData),  // User data
      false,  // Is project file or folder
      false,  // Has children
      false,  // Expanded
      false,  // Supports pinning
      false,  // Pinned
      ui::TreeViewItemAction::None,  // Action
  });
  addCategoriesToModel(root, type, *mCategories, 1);
}

void LibraryTab::addCategoriesToModel(
    TreeItem& item, TreeItemType type,
    slint::VectorModel<ui::TreeViewItemData>& model, int level) noexcept {
  for (auto child : item.childs) {
    if (child->type == type) {
      const int count =
          std::count_if(child->childs.begin(), child->childs.end(),
                        [type](const std::shared_ptr<TreeItem>& item) {
                          return item->type != type;
                        });
      model.push_back(ui::TreeViewItemData{
          level,  // Level
          child->icon,  // Icon
          q2s(child->text),  // Text
          (count > 0) ? q2s(QString::number(count))
                      : slint::SharedString(),  // Comment
          slint::SharedString(),  // Hint
          child->fromOtherLib,  // Italic
          count > 0,  // Bold
          q2s(child->userData),  // User data
          false,  // Is project file or folder
          false,  // Has children
          false,  // Expanded
          false,  // Supports pinning
          false,  // Pinned
          ui::TreeViewItemAction::None,  // Action
      });
    }
    addCategoriesToModel(*child, type, model, level + 1);
  }
}

void LibraryTab::setSelectedCategory(
    const std::optional<ui::TreeViewItemData>& data) noexcept {
  const bool isRoot = data ? (data->level == 0) : true;
  const auto userData = data ? s2q(data->user_data) : QString();
  const std::optional<Uuid> uuid =
      data ? Uuid::tryFromString(userData) : std::nullopt;

  QVector<std::shared_ptr<TreeItem>> items;
  if (isRoot) {
    QSet<std::shared_ptr<TreeItem>> set;
    if (uuid) {
      if (auto item = mLibElementsMap.value(*uuid)) {
        getChildsRecursive(*item, set);
      }
    } else {
      getChildsRecursive(*mCmpCatRoot, set);
      getChildsRecursive(*mPkgCatRoot, set);
    }
    items = Toolbox::toList(set);
    sortItemsRecursive(items);
  } else if (uuid) {
    if (auto item = mLibElementsMap.value(*uuid)) {
      items = item->childs;
    }
  }

  std::vector<ui::TreeViewItemData> rows;
  rows.reserve(items.count() + 4);
  std::optional<TreeItemType> type;
  for (auto item : items) {
    if (item->type != type) {
      QString category;
      switch (item->type) {
        case TreeItemType::Symbol: {
          category = "Symbols";
          break;
        }
        case TreeItemType::Package: {
          category = "Packages";
          break;
        }
        case TreeItemType::Component: {
          category = "Components";
          break;
        }
        case TreeItemType::Device: {
          category = "Devices";
          break;
        }
        default: {
          continue;
        }
      }
      rows.push_back(ui::TreeViewItemData{
          0,  // Level
          slint::Image(),  // Icon
          q2s(category),  // Text
          slint::SharedString(),  // Comment
          slint::SharedString(),  // Hint
          false,  // Italic
          true,  // Bold
          slint::SharedString(),  // User data
          false,  // Is project file or folder
          false,  // Has children
          false,  // Expanded
          false,  // Supports pinning
          false,  // Pinned
          ui::TreeViewItemAction::None,  // Action
      });
      type = item->type;
    }
    rows.push_back(ui::TreeViewItemData{
        1,  // Level
        item->icon,  // Icon
        q2s(item->text),  // Text
        slint::SharedString(),  // Comment
        slint::SharedString(),  // Hint
        false,  // Italic
        false,  // Bold
        q2s(item->userData),  // User data
        false,  // Is project file or folder
        false,  // Has children
        false,  // Expanded
        false,  // Supports pinning
        false,  // Pinned
        ui::TreeViewItemAction::None,  // Action
    });
  }
  mFilteredElements->set_vector(std::move(rows));
}

void LibraryTab::getChildsRecursive(
    TreeItem& item, QSet<std::shared_ptr<TreeItem>>& childs) noexcept {
  childs |= Toolbox::toSet(item.childs);
  for (auto child : item.childs) {
    getChildsRecursive(*child, childs);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

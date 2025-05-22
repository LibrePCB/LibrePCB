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
    mLibElementsRoot(),
    mCategories(new slint::VectorModel<ui::TreeViewItemData>()),
    mFilteredElements(new slint::VectorModel<ui::TreeViewItemData>()) {
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
      mCurrentCategoryIndex,
      mFilteredElements,  // Filtered elements
  };
}

void LibraryTab::setDerivedUiData(const ui::LibraryTabData& data) noexcept {
  if (data.categories_index != mCurrentCategoryIndex) {
    mCurrentCategoryIndex = data.categories_index;
    auto item = mCategories->row_data(mCurrentCategoryIndex);
    auto userData = item ? s2q(item->user_data) : QString();
    setSelectedCategory(Uuid::tryFromString(userData));
  }
  onDerivedUiDataChanged.notify();
}

void LibraryTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
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
  mLibElementsRoot.reset(
      new TreeItem{TreeItemType::ComponentCategory,  // Ignored
                   QString(),
                   QString(),
                   QString(),
                   false,
                   {}});
  mLibElementsMap.clear();
  loadCategories<ComponentCategory>(TreeItemType::ComponentCategory);
  loadCategories<PackageCategory>(TreeItemType::PackageCategory);
  loadElements<Symbol, ComponentCategory>(TreeItemType::Symbol,
                                          TreeItemType::ComponentCategory);
  loadElements<Package, PackageCategory>(TreeItemType::Package,
                                         TreeItemType::PackageCategory);
  loadElements<Component, ComponentCategory>(TreeItemType::Component,
                                             TreeItemType::ComponentCategory);
  loadElements<Device, ComponentCategory>(TreeItemType::Device,
                                          TreeItemType::ComponentCategory);

  for (auto child : mLibElementsRoot->childs) {
    sortItemsRecursive(child->childs);
  }

  mCategories->clear();
  addCategoriesToModel(TreeItemType::ComponentCategory,
                       tr("Component Categories"),
                       QIcon(":/img/places/folder.png"));
  addCategoriesToModel(TreeItemType::PackageCategory, tr("Package Categories"),
                       QIcon(":/img/places/folder_green.png"));

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";
}

template <typename CategoryType>
void LibraryTab::loadCategories(TreeItemType type) {
  try {
    mLibCategories = mDb.getAll<CategoryType>(mLibPath);
    for (auto it = mLibCategories.begin(); it != mLibCategories.end(); it++) {
      getOrCreateCategory<CategoryType>(type, *it);
    }
  } catch (const Exception& e) {
    // TODO
  }
}

template <typename CategoryType>
std::shared_ptr<LibraryTab::TreeItem> LibraryTab::getOrCreateCategory(
    TreeItemType type, const Uuid& uuid) {
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
    }
    if ((!fp.isValid()) ||
        (!mDb.getTranslations<CategoryType>(fp, mLocaleOrder, &item->text,
                                            &item->tooltip))) {
      item->text = tr("Unknown") % " (" % uuid.toStr() % ")";
    }
    std::optional<Uuid> parentUuid;
    if (fp.isValid()) {
      mDb.getCategoryMetadata<CategoryType>(fp, &parentUuid);
    }
    auto parent = mLibElementsRoot;
    if (parentUuid) {
      if (auto p = getOrCreateCategory<CategoryType>(type, *parentUuid)) {
        parent = p;
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
void LibraryTab::loadElements(TreeItemType type, TreeItemType catType) {
  try {
    const QSet<FilePath> elements =
        Toolbox::toSet(mDb.getAll<ElementType>(mLibPath).keys());
    for (const FilePath& fp : elements) {
      for (const Uuid& catUuid : mDb.getCategoriesOf<ElementType>(fp)) {
        if (auto cat = getOrCreateCategory<CategoryType>(catType, catUuid)) {
          auto item = std::make_shared<TreeItem>();
          item->type = type;
          item->userData = fp.toStr();
          item->fromOtherLib = false;
          mDb.getTranslations<ElementType>(fp, mLocaleOrder, &item->text,
                                           &item->tooltip);
          cat->childs.push_back(item);
        }
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

void LibraryTab::addCategoriesToModel(TreeItemType type, const QString& title,
                                      const QIcon& icon) noexcept {
  mCategories->push_back(ui::TreeViewItemData{
      0,  // Level
      slint::Image(),  // Icon
      q2s(title),  // Text
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
  addCategoriesToModel(*mLibElementsRoot, type, *mCategories, 1, icon);
}

void LibraryTab::addCategoriesToModel(
    TreeItem& item, TreeItemType type,
    slint::VectorModel<ui::TreeViewItemData>& model, int level,
    const QIcon& icon) noexcept {
  for (auto child : item.childs) {
    const int elementsCount =
        std::count_if(child->childs.begin(), child->childs.end(),
                      [type](const std::shared_ptr<TreeItem>& item) {
                        return item->type != type;
                      });
    if (child->type == type) {
      const QIcon::Mode mode =
          child->fromOtherLib ? QIcon::Disabled : QIcon::Normal;
      model.push_back(ui::TreeViewItemData{
          level,  // Level
          q2s(icon.pixmap(32, mode)),  // Icon
          q2s(child->text),  // Text
          (elementsCount > 0) ? q2s(QString::number(elementsCount))
                              : slint::SharedString(),  // Comment
          q2s(child->tooltip),  // Hint
          child->fromOtherLib,  // Italic
          elementsCount > 0,  // Bold
          q2s(child->userData),  // User data
          false,  // Is project file or folder
          false,  // Has children
          false,  // Expanded
          false,  // Supports pinning
          false,  // Pinned
          ui::TreeViewItemAction::None,  // Action
      });
    }
    addCategoriesToModel(*child, type, model, level + 1, icon);
  }
}

void LibraryTab::setSelectedCategory(const std::optional<Uuid>& uuid) noexcept {
  // if ((uuid == mSelectedCategory) && (!force)) return;

  // mSelectedCategory = uuid;
  mFilteredElements->clear();

  auto item = uuid ? mLibElementsMap.value(*uuid) : mLibElementsRoot;
  if (!item) return;

  QVector<std::shared_ptr<TreeItem>> items;
  if (uuid) {
    if (auto item = mLibElementsMap.value(*uuid)) {
      items = item->childs;
    }
  } else {
    getChildsRecursive(*mLibElementsRoot, items);
    sortItemsRecursive(items);
  }

  std::optional<TreeItemType> type;
  QIcon icon;
  for (auto item : items) {
    if (item->type != type) {
      QString category;
      switch (item->type) {
        case TreeItemType::Symbol: {
          icon = QIcon(":/img/library/symbol.png");
          category = "Symbols";
          break;
        }
        case TreeItemType::Package: {
          icon = QIcon(":/img/library/package.png");
          category = "Packages";
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
          continue;
        }
      }
      mFilteredElements->push_back(ui::TreeViewItemData{
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
    mFilteredElements->push_back(ui::TreeViewItemData{
        1,  // Level
        q2s(icon.pixmap(32)),  // Icon
        q2s(item->text),  // Text
        slint::SharedString(),  // Comment
        q2s(item->tooltip),  // Hint
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
}

void LibraryTab::getChildsRecursive(
    TreeItem& item, QVector<std::shared_ptr<TreeItem>>& childs) noexcept {
  childs += item.childs;
  for (auto child : item.childs) {
    getChildsRecursive(*child, childs);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

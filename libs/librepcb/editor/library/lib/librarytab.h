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

#ifndef LIBREPCB_EDITOR_LIBRARYTAB_H
#define LIBREPCB_EDITOR_LIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../windowtab.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class WorkspaceLibraryDb;

namespace editor {

class LibraryEditor2;
class LibraryElementsModel;

/*******************************************************************************
 *  Class LibraryTab
 ******************************************************************************/

/**
 * @brief The LibraryTab class
 */
class LibraryTab final : public WindowTab {
  Q_OBJECT

  enum class TreeItemType {
    ComponentCategory,
    PackageCategory,
    Symbol,
    Package,
    Component,
    Device,
  };
  struct TreeItem {
    TreeItemType type;
    std::weak_ptr<TreeItem> parent;  ///< nullptr for root categories
    std::optional<Uuid> uuid;  ///< std::nullopt for items without category
    QString text;
    QString tooltip;
    QVector<std::shared_ptr<TreeItem>> childs;
  };

public:
  // Signals
  Signal<LibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  LibraryTab() = delete;
  LibraryTab(const LibraryTab& other) = delete;
  explicit LibraryTab(GuiApplication& app, LibraryEditor2& editor,
                      QObject* parent = nullptr) noexcept;
  ~LibraryTab() noexcept;

  // General Methods
  int getLibraryIndex() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::LibraryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::LibraryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  LibraryTab& operator=(const LibraryTab& rhs) = delete;

private:  // Methods
  void refreshLibElements() noexcept;
  template <typename CategoryType>
  void loadCategories(TreeItemType type);
  template <typename CategoryType>
  std::shared_ptr<TreeItem> getOrCreateCategory(TreeItemType type,
                                                const Uuid& uuid);
  template <typename ElementType, typename CategoryType>
  void loadElements(TreeItemType type, TreeItemType catType);
  void sortItemsRecursive(QVector<std::shared_ptr<TreeItem>>& items) noexcept;
  void refreshCategoriesModel(
      TreeItemType type,
      slint::VectorModel<ui::TreeViewItemData>& model) noexcept;
  void addChildsToModel(TreeItem& item, TreeItemType type,
                        slint::VectorModel<ui::TreeViewItemData>& model,
                        int level) noexcept;
  void setSelectedCategory(const std::optional<Uuid>& uuid,
                           bool force) noexcept;
  void getChildsRecursive(TreeItem& item,
                          QVector<std::shared_ptr<TreeItem>>& childs) noexcept;

private:
  LibraryEditor2& mEditor;
  Library& mLibrary;
  const WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;
  const FilePath mLibPath;

  // Library content
  QHash<FilePath, Uuid> mLibCategories;
  std::shared_ptr<TreeItem> mLibElementsRoot;
  QHash<Uuid, std::shared_ptr<TreeItem>> mLibElementsMap;

  // UI data
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mCmpCategories;
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mPkgCategories;
  int mCmpCatIndex = -1;
  int mPkgCatIndex = -1;
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mFilteredElements;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

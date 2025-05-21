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

#ifndef LIBREPCB_EDITOR_LIBRARYELEMENTSMODEL_H
#define LIBREPCB_EDITOR_LIBRARYELEMENTSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;
class WorkspaceLibraryDb;

namespace editor {

/*******************************************************************************
 *  Class LibraryElementsModel
 ******************************************************************************/

/**
 * @brief The LibraryElementsModel class
 */
class LibraryElementsModel : public QObject,
                             public slint::Model<ui::TreeViewItemData> {
  Q_OBJECT

  enum class TreeItemType {
    Root,
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
  // Constructors / Destructor
  LibraryElementsModel() = delete;
  LibraryElementsModel(const LibraryElementsModel& other) = delete;
  explicit LibraryElementsModel(const Workspace& ws, const FilePath& libFp,
                                QObject* parent = nullptr) noexcept;
  virtual ~LibraryElementsModel() noexcept;

  // General Methods
  std::shared_ptr<slint::Model<ui::TreeViewItemData>> getElementsModel()
      const noexcept {
    return mElementsModel;
  }
  const std::optional<Uuid>& getSelectedCategory() const noexcept {
    return mSelectedCategory;
  }
  void setSelectedCategory(const std::optional<Uuid>& uuid) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::TreeViewItemData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::TreeViewItemData& data) noexcept override;

  // Operator Overloadings
  LibraryElementsModel& operator=(const LibraryElementsModel& rhs) = delete;

private:  // Methods
  void refresh() noexcept;
  template <typename CategoryType>
  void loadCategories();
  template <typename CategoryType>
  std::shared_ptr<TreeItem> getOrCreateCategory(const Uuid& uuid);
  template <typename ElementType, typename CategoryType>
  void loadElements(TreeItemType type);

  // QVector<std::shared_ptr<TreeItem>> getChilds(
  //     std::shared_ptr<TreeItem> parent, QSet<FilePath>& sym) const noexcept;
  void addChildsToModel(TreeItem& item, int level) noexcept;

private:
  const Workspace& mWorkspace;
  const WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;
  const FilePath mLibPath;
  std::shared_ptr<TreeItem> mRoot;
  QHash<FilePath, Uuid> mLibCategories;
  QHash<Uuid, std::shared_ptr<TreeItem>> mCategories;
  std::vector<ui::TreeViewItemData> mItems;

  std::optional<Uuid> mSelectedCategory;
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mElementsModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

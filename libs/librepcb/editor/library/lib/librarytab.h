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
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class WorkspaceLibraryDb;

namespace editor {

class LibraryDependenciesModel;
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
    Uncategorized,
    ComponentCategory,
    PackageCategory,
    Symbol,
    Package,
    Component,
    Device,
  };
  struct TreeItem {
    TreeItemType type;
    slint::Image icon;
    QString text;
    QString userData;
    QVector<std::shared_ptr<TreeItem>> childs;
  };

public:
  // Signals
  Signal<LibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  LibraryTab() = delete;
  LibraryTab(const LibraryTab& other) = delete;
  explicit LibraryTab(GuiApplication& app, LibraryEditor2& editor,
                      bool wizardMode, QObject* parent = nullptr) noexcept;
  ~LibraryTab() noexcept;

  // General Methods
  const FilePath& getDirectoryPath() const noexcept { return mDirPath; }
  ui::TabData getUiData() const noexcept override;
  ui::LibraryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::LibraryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  LibraryTab& operator=(const LibraryTab& rhs) = delete;

signals:
  void componentCategoryEditorRequested(LibraryEditor2& editor,
                                        const FilePath& fp);
  void packageCategoryEditorRequested(LibraryEditor2& editor,
                                      const FilePath& fp);
  void symbolEditorRequested(LibraryEditor2& editor, const FilePath& fp);
  void packageEditorRequested(LibraryEditor2& editor, const FilePath& fp);
  void componentEditorRequested(LibraryEditor2& editor, const FilePath& fp);
  void deviceEditorRequested(LibraryEditor2& editor, const FilePath& fp);

private:  // Methods
  void refreshMetadata() noexcept;
  void commitMetadata() noexcept;
  void commitDependencies() noexcept;
  void refreshLibElements() noexcept;
  std::shared_ptr<TreeItem> createRootItem(TreeItemType type, const QIcon& icon,
                                           const QString& text) noexcept;
  template <typename CategoryType>
  void loadCategories(TreeItemType type, const QIcon& icon, TreeItem& root);
  template <typename CategoryType>
  std::shared_ptr<TreeItem> getOrCreateCategory(TreeItemType type,
                                                const QIcon& icon,
                                                const Uuid& uuid,
                                                TreeItem& root);
  template <typename ElementType, typename CategoryType>
  void loadElements(TreeItemType type, slint::Image icon, TreeItemType catType,
                    const QIcon& catIcon, TreeItem& root, int& count);
  void sortItemsRecursive(QVector<std::shared_ptr<TreeItem>>& items) noexcept;
  void addCategoriesToModel(TreeItemType type, TreeItem& root,
                            int count) noexcept;
  void addCategoriesToModel(TreeItem& item, TreeItemType type,
                            slint::VectorModel<ui::TreeViewItemData>& model,
                            int level) noexcept;
  void setSelectedCategory(
      const std::optional<ui::TreeViewItemData>& data) noexcept;
  void getChildsRecursive(TreeItem& item,
                          QSet<std::shared_ptr<TreeItem>>& childs) noexcept;

private:
  LibraryEditor2& mEditor;
  Library& mLibrary;
  const WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;
  const FilePath mDirPath;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  int mCurrentCategoryIndex;
  int mCurrentElementIndex;

  // Library metadata to be applied
  slint::SharedString mName;
  slint::SharedString mNameError;
  ElementName mNameParsed;
  slint::SharedString mDescription;
  slint::SharedString mKeywords;
  slint::SharedString mAuthor;
  slint::SharedString mVersion;
  slint::SharedString mVersionError;
  Version mVersionParsed;
  bool mDeprecated;
  slint::SharedString mUrl;
  slint::SharedString mUrlError;
  slint::SharedString mManufacturer;

  // Library content
  QHash<FilePath, Uuid> mLibCategories;
  std::shared_ptr<TreeItem> mUncategorizedRoot;
  std::shared_ptr<TreeItem> mCmpCatRoot;
  int mCmpCatElementCount;
  std::shared_ptr<TreeItem> mPkgCatRoot;
  int mPkgCatElementCount;
  QHash<QString, std::shared_ptr<TreeItem>> mLibElementsMap;

  // UI data
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mCategories;
  std::shared_ptr<slint::VectorModel<ui::TreeViewItemData>> mFilteredElements;
  std::shared_ptr<LibraryDependenciesModel> mDependencies;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

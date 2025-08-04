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
#include "../libraryeditortab.h"

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
class LibraryEditor;

/*******************************************************************************
 *  Class LibraryTab
 ******************************************************************************/

/**
 * @brief The LibraryTab class
 */
class LibraryTab final : public LibraryEditorTab {
  Q_OBJECT

  struct TreeItem {
    ui::LibraryTreeViewItemType type;
    FilePath path;  // Only when part of this library.
    QString name;
    QString summary;
    bool isExternal = false;
    QString userData;  // UUID for categories, filepath for elements
    QVector<std::shared_ptr<TreeItem>> childs;
  };

public:
  // Signals
  Signal<LibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  LibraryTab() = delete;
  LibraryTab(const LibraryTab& other) = delete;
  explicit LibraryTab(LibraryEditor& editor, bool wizardMode,
                      QObject* parent = nullptr) noexcept;
  ~LibraryTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  void setUiData(const ui::TabData& data) noexcept override;
  ui::LibraryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::LibraryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  LibraryTab& operator=(const LibraryTab& rhs) = delete;

signals:
  void componentCategoryEditorRequested(LibraryEditor& editor,
                                        const FilePath& fp, bool copyFrom);
  void packageCategoryEditorRequested(LibraryEditor& editor, const FilePath& fp,
                                      bool copyFrom);
  void symbolEditorRequested(LibraryEditor& editor, const FilePath& fp,
                             bool copyFrom);
  void packageEditorRequested(LibraryEditor& editor, const FilePath& fp,
                              bool copyFrom);
  void componentEditorRequested(LibraryEditor& editor, const FilePath& fp,
                                bool copyFrom);
  void deviceEditorRequested(LibraryEditor& editor, const FilePath& fp,
                             bool copyFrom);

protected:
  std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() override;
  bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                   bool checkOnly) override;
  template <typename MessageType>
  bool autoFixHelper(const std::shared_ptr<const RuleCheckMessage>& msg,
                     bool checkOnly);
  template <typename MessageType>
  void autoFix(const MessageType& msg);
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  void refreshUiData() noexcept;
  void commitUiData() noexcept;
  void refreshLibElements() noexcept;
  std::shared_ptr<TreeItem> createRootItem(
      ui::LibraryTreeViewItemType type) noexcept;
  template <typename CategoryType>
  void loadCategories(ui::LibraryTreeViewItemType type, TreeItem& root);
  template <typename CategoryType>
  std::shared_ptr<TreeItem> getOrCreateCategory(
      ui::LibraryTreeViewItemType type, const Uuid& uuid, TreeItem& root);
  template <typename ElementType, typename CategoryType>
  void loadElements(ui::LibraryTreeViewItemType type,
                    ui::LibraryTreeViewItemType catType, TreeItem& root,
                    int& count);
  void sortItemsRecursive(QVector<std::shared_ptr<TreeItem>>& items) noexcept;
  void addCategoriesToModel(ui::LibraryTreeViewItemType type, TreeItem& root,
                            int count) noexcept;
  void addCategoriesToModel(
      TreeItem& item, ui::LibraryTreeViewItemType type,
      slint::VectorModel<ui::LibraryTreeViewItemData>& model,
      int level) noexcept;
  void setSelectedCategory(
      const std::optional<ui::LibraryTreeViewItemData>& data) noexcept;
  void getChildsRecursive(TreeItem& item,
                          QSet<std::shared_ptr<TreeItem>>& childs) noexcept;
  QList<std::shared_ptr<TreeItem>> getSelectedCategories() const noexcept;
  QList<std::shared_ptr<TreeItem>> getSelectedElements() const noexcept;
  void duplicateElements(
      const QList<std::shared_ptr<TreeItem>>& items) noexcept;
  void moveElementsTo(const QList<std::shared_ptr<TreeItem>>& items,
                      const FilePath& dstLib) noexcept;
  void deleteElements(const QList<std::shared_ptr<TreeItem>>& items) noexcept;

private:
  Library& mLibrary;
  const WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  int mCurrentCategoryIndex;
  int mCurrentElementIndex;
  QString mFilterTerm;

  // Library metadata to be applied
  QByteArray mIcon;
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
  std::shared_ptr<LibraryDependenciesModel> mDependencies;
  slint::SharedString mManufacturer;

  // Library content
  QHash<FilePath, Uuid> mLibCategories;
  std::shared_ptr<TreeItem> mUncategorizedRoot;
  std::shared_ptr<TreeItem> mCmpCatRoot;
  int mCmpCatElementCount;
  std::shared_ptr<TreeItem> mPkgCatRoot;
  int mPkgCatElementCount;
  QHash<QString, std::shared_ptr<TreeItem>> mLibElementsMap;  // Key: user-data
  std::shared_ptr<slint::VectorModel<ui::LibraryTreeViewItemData>> mCategories;
  std::shared_ptr<slint::VectorModel<ui::LibraryTreeViewItemData>> mElements;
  std::shared_ptr<slint::FilterModel<ui::LibraryTreeViewItemData>>
      mFilteredElements;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

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

#include "../../dialogs/filedialog.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdlibraryedit.h"
#include "../libraryeditor.h"
#include "librarydependenciesmodel.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

LibraryTab::LibraryTab(LibraryEditor& editor, bool wizardMode,
                       QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mLibrary(mEditor.getLibrary()),
    mDb(editor.getWorkspace().getLibraryDb()),
    mLocaleOrder(editor.getWorkspace().getSettings().libraryLocaleOrder.get()),
    mWizardMode(wizardMode),
    mCurrentPageIndex(mWizardMode ? 0 : 2),
    mCurrentCategoryIndex(0),
    mCurrentElementIndex(-1),
    mNameParsed(mLibrary.getNames().getDefaultValue()),
    mVersionParsed(mLibrary.getVersion()),
    mDeprecated(false),
    mDependencies(new LibraryDependenciesModel(editor.getWorkspace(),
                                               mLibrary.getUuid())),
    mCmpCatElementCount(0),
    mPkgCatElementCount(0),
    mCategories(new slint::VectorModel<ui::LibraryTreeViewItemData>()),
    mElements(new slint::VectorModel<ui::LibraryTreeViewItemData>()),
    mFilteredElements(new slint::FilterModel<ui::LibraryTreeViewItemData>(
        mElements, [this](const ui::LibraryTreeViewItemData& data) {
          return (data.level == 0) || mFilterTerm.isEmpty() ||
              s2q(data.name).contains(mFilterTerm, Qt::CaseInsensitive);
        })) {
  // Update the library element lists each time the library scan succeeded,
  // i.e. new information about the libraries is available. Attention: Use
  // the "scanSucceeded" signal, not "scanFinished" since "scanFinished" is
  // also called when a scan is aborted, i.e. *no* new information is available!
  // This can cause wrong list items after removing or adding elements, since
  // these operations are immediately applied on the list widgets (for immediate
  // feedback) but will then be reverted if a scan was aborted.
  // TODO: This is currently not true anymore.
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &LibraryTab::refreshLibElements);

  // Connect library editor.
  connect(&mEditor, &LibraryEditor::manualModificationsMade, this, [this]() {
    mManualModificationsMade = true;
    onUiDataChanged.notify();
  });

  // Connect undo stack.
  mUndoStack.reset(&mEditor.getUndoStack());  // Not nice :-/
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &LibraryTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &LibraryTab::refreshUiData);

  // Connect models.
  connect(mDependencies.get(), &LibraryDependenciesModel::modified, this,
          &LibraryTab::commitUiData, Qt::QueuedConnection);

  // Refresh content.
  refreshUiData();
  refreshLibElements();
  setSelectedCategory(std::nullopt);
  scheduleChecks();
}

LibraryTab::~LibraryTab() noexcept {
  deactivate();

  mUndoStack.release();  // We have "borrowed" it from the library editor...
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath LibraryTab::getDirectoryPath() const noexcept {
  return mLibrary.getDirectory().getAbsPath();
}

ui::TabData LibraryTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mLibrary.getDirectory().isWritable());
  features.undo = toFs(mEditor.getUndoStack().canUndo());
  features.redo = toFs(mEditor.getUndoStack().canRedo());
  features.find = toFs(true);

  return ui::TabData{
      ui::TabType::Library,  // Type
      q2s(*mLibrary.getNames().getDefaultValue()),  // Title
      features,  // Features
      !mLibrary.getDirectory().isWritable(),  // Read-only
      mEditor.hasUnsavedChanges(),  // Unsaved changes
      q2s(mEditor.getUndoStack().getUndoCmdText()),  // Undo text
      q2s(mEditor.getUndoStack().getRedoCmdText()),  // Redo text
      q2s(mFilterTerm),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

void LibraryTab::setUiData(const ui::TabData& data) noexcept {
  LibraryEditorTab::setUiData(data);

  const auto filterTerm = s2q(data.find_term).trimmed();
  if (filterTerm != mFilterTerm) {
    mFilterTerm = filterTerm;
    mFilteredElements->reset();
    onUiDataChanged.notify();
  }
}

ui::LibraryTabData LibraryTab::getDerivedUiData() const noexcept {
  return ui::LibraryTabData{
      mEditor.getUiIndex(),  // Library index
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      q2s(mLibrary.getIconAsPixmap()),  // Icon
      mName,  // Name
      mNameError,  // Name error
      mDescription,  // Description
      mKeywords,  // Keywords
      mAuthor,  // Author
      mVersion,  // Version
      mVersionError,  // Version error
      mDeprecated,  // Deprecated
      mUrl,  // URL
      mUrlError,  // URL error
      mDependencies,  // Dependencies
      mManufacturer,  // Manufacturer
      mCategories,  // Component categories
      mCurrentCategoryIndex,  // Current category index
      mFilteredElements,  // Filtered elements
      mCurrentElementIndex,  // Current element index
      ui::RuleCheckData{
          ui::RuleCheckType::LibraryCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check error count
          mCheckError,  // Check execution error
          !mLibrary.getDirectory().isWritable(),  // Check read-only
      },
      slint::SharedString(),  // Move category to
      slint::SharedString(),  // Move element to
  };
}

void LibraryTab::setDerivedUiData(const ui::LibraryTabData& data) noexcept {
  mName = data.name;
  if (auto value = validateElementName(s2q(mName), mNameError)) {
    mNameParsed = *value;
  }
  mDescription = data.description;
  mKeywords = data.keywords;
  mAuthor = data.author;
  mVersion = data.version;
  if (auto value = validateVersion(s2q(mVersion), mVersionError)) {
    mVersionParsed = *value;
  }
  mDeprecated = data.deprecated;
  mUrl = data.url;
  validateUrl(s2q(mUrl), mUrlError, true);
  mManufacturer = data.manufacturer;

  // Page index
  mCurrentPageIndex = data.page_index;

  // Current category index
  if (data.categories_index != mCurrentCategoryIndex) {
    mCurrentCategoryIndex = data.categories_index;
    setSelectedCategory(mCategories->row_data(mCurrentCategoryIndex));
  }

  // Current element index
  mCurrentElementIndex = data.filtered_elements_index;

  // Move category to other library
  if (!data.move_category_to_lib.empty()) {
    moveElementsTo(getSelectedCategories(),
                   FilePath(s2q(data.move_category_to_lib)));
  }

  // Move elements to other library
  if (!data.move_element_to_lib.empty()) {
    moveElementsTo(getSelectedElements(),
                   FilePath(s2q(data.move_element_to_lib)));
  }

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void LibraryTab::trigger(ui::TabAction a) noexcept {
  auto data = (mCurrentElementIndex == -1)
      ? mCategories->row_data(mCurrentCategoryIndex)
      : mFilteredElements->row_data(mCurrentElementIndex);
  const QString userData = data ? s2q(data->user_data) : QString();
  std::shared_ptr<TreeItem> item = mLibElementsMap.value(userData);

  switch (a) {
    case ui::TabAction::Back: {
      if (mWizardMode && (mCurrentPageIndex > 0)) {
        --mCurrentPageIndex;
      }
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Next: {
      commitUiData();
      if (mWizardMode && (mCurrentPageIndex == 0) && mEditor.save()) {
        ++mCurrentPageIndex;
      } else if (mWizardMode && (mCurrentPageIndex == 1)) {
        mWizardMode = false;
        ++mCurrentPageIndex;
      }
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Apply: {
      commitUiData();
      refreshUiData();
      break;
    }
    case ui::TabAction::Save: {
      commitUiData();

      // Remove obsolete message approvals (bypassing the undo stack). Since
      // the checks are run asynchronously, the approvals may be outdated, so
      // we first run the checks once synchronosuly.
      runChecks();
      mLibrary.setMessageApprovals(mLibrary.getMessageApprovals() -
                                   mDisappearedApprovals);

      mEditor.save();
      refreshUiData();
      break;
    }
    case ui::TabAction::EditProperties: {
      if (item && item->path.isValid()) {
        switch (item->type) {
          case ui::LibraryTreeViewItemType::ComponentCategory: {
            emit componentCategoryEditorRequested(mEditor, item->path, false);
            break;
          }
          case ui::LibraryTreeViewItemType::PackageCategory: {
            emit packageCategoryEditorRequested(mEditor, item->path, false);
            break;
          }
          case ui::LibraryTreeViewItemType::Symbol: {
            emit symbolEditorRequested(mEditor, item->path, false);
            break;
          }
          case ui::LibraryTreeViewItemType::Package: {
            emit packageEditorRequested(mEditor, item->path, false);
            break;
          }
          case ui::LibraryTreeViewItemType::Component: {
            emit componentEditorRequested(mEditor, item->path);
            break;
          }
          case ui::LibraryTreeViewItemType::Device: {
            emit deviceEditorRequested(mEditor, item->path);
            break;
          }
          default: {
            break;
          }
        }
      }
      break;
    }
    case ui::TabAction::Undo: {
      try {
        commitUiData();
        mEditor.getUndoStack().undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitUiData();
        mEditor.getUndoStack().redo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::LibraryChooseIcon: {
      const QString fp = FileDialog::getOpenFileName(
          qApp->activeWindow(), tr("Choose Library Icon"),
          mLibrary.getDirectory().getAbsPath().toNative(),
          tr("Portable Network Graphics (*.png)"));
      if (!fp.isEmpty()) {
        try {
          mIcon = FileUtils::readFile(FilePath(fp));  // can throw
          commitUiData();
        } catch (const Exception& e) {
          QMessageBox::critical(qApp->activeWindow(), tr("Could not open file"),
                                e.getMsg());
        }
      }
      break;
    }
    case ui::TabAction::LibraryCategoriesDuplicate: {
      duplicateElements(getSelectedCategories());
      break;
    }
    case ui::TabAction::LibraryCategoriesRemove: {
      deleteElements(getSelectedCategories());
      break;
    }
    case ui::TabAction::LibraryElementsDuplicate: {
      duplicateElements(getSelectedElements());
      break;
    }
    case ui::TabAction::LibraryElementsRemove: {
      deleteElements(getSelectedElements());
      break;
    }
    case ui::TabAction::Close: {
      commitUiData();
      WindowTab::trigger(a);
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
    LibraryTab::runChecksImpl() {
  return std::make_pair(mLibrary.runChecks(), mLibrary.getMessageApprovals());
}

bool LibraryTab::autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                             bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool LibraryTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

void LibraryTab::messageApprovalChanged(const SExpression& approval,
                                        bool approved) noexcept {
  if (mLibrary.setMessageApproved(approval, approved)) {
    mEditor.setManualModificationsMade();
  }
}

void LibraryTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
void LibraryTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitUiData();
}

template <>
void LibraryTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryTab::refreshUiData() noexcept {
  mIcon = mLibrary.getIcon();
  mName = q2s(*mLibrary.getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mLibrary.getNames().getDefaultValue();
  mDescription = q2s(mLibrary.getDescriptions().getDefaultValue());
  mKeywords = q2s(mLibrary.getKeywords().getDefaultValue());
  mAuthor = q2s(mLibrary.getAuthor());
  mVersion = q2s(mLibrary.getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mLibrary.getVersion();
  mDeprecated = mLibrary.isDeprecated();
  mUrl = q2s(mLibrary.getUrl().toString());
  mUrlError = slint::SharedString();
  mDependencies->setUuids(mLibrary.getDependencies());
  mManufacturer = q2s(*mLibrary.getManufacturer());

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void LibraryTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdLibraryEdit> cmd(new CmdLibraryEdit(mLibrary));
    cmd->setIcon(mIcon);
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mLibrary.getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mLibrary.getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mLibrary.getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    const QString urlStr = s2q(mUrl);
    if (urlStr != mLibrary.getUrl().toString()) {
      cmd->setUrl(QUrl(urlStr.trimmed(), QUrl::TolerantMode));
    }
    cmd->setDependencies(mDependencies->getUuids());
    const QString manufacturer = s2q(mManufacturer);
    if (manufacturer != mLibrary.getManufacturer()) {
      cmd->setManufacturer(cleanSimpleString(manufacturer));
    }
    mEditor.getUndoStack().execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void LibraryTab::refreshLibElements() noexcept {
  // For big libraries this can take a while.
  QApplication::setOverrideCursor(Qt::WaitCursor);
  auto cursorScopeGuard =
      scopeGuard([]() { QApplication::restoreOverrideCursor(); });

  qDebug() << "Update library elements tree...";
  QElapsedTimer t;
  t.start();

  mLibCategories.clear();
  mLibElementsMap.clear();
  mCmpCatElementCount = 0;
  mPkgCatElementCount = 0;

  mUncategorizedRoot =
      createRootItem(ui::LibraryTreeViewItemType::Uncategorized);
  mCmpCatRoot = createRootItem(ui::LibraryTreeViewItemType::ComponentCategory);
  mPkgCatRoot = createRootItem(ui::LibraryTreeViewItemType::PackageCategory);

  loadCategories<ComponentCategory>(
      ui::LibraryTreeViewItemType::ComponentCategory, *mCmpCatRoot);
  loadCategories<PackageCategory>(ui::LibraryTreeViewItemType::PackageCategory,
                                  *mPkgCatRoot);

  loadElements<Symbol, ComponentCategory>(
      ui::LibraryTreeViewItemType::Symbol,
      ui::LibraryTreeViewItemType::ComponentCategory, *mCmpCatRoot,
      mCmpCatElementCount);
  loadElements<Package, PackageCategory>(
      ui::LibraryTreeViewItemType::Package,
      ui::LibraryTreeViewItemType::PackageCategory, *mPkgCatRoot,
      mPkgCatElementCount);
  loadElements<Component, ComponentCategory>(
      ui::LibraryTreeViewItemType::Component,
      ui::LibraryTreeViewItemType::ComponentCategory, *mCmpCatRoot,
      mCmpCatElementCount);
  loadElements<Device, ComponentCategory>(
      ui::LibraryTreeViewItemType::Device,
      ui::LibraryTreeViewItemType::ComponentCategory, *mCmpCatRoot,
      mCmpCatElementCount);

  sortItemsRecursive(mCmpCatRoot->childs);
  sortItemsRecursive(mPkgCatRoot->childs);

  mCategories->clear();
  const int count = mCmpCatElementCount + mPkgCatElementCount;
  mCategories->push_back(ui::LibraryTreeViewItemData{
      ui::LibraryTreeViewItemType::All,  // Type
      0,  // Level
      slint::SharedString(),  // Name (set in UI)
      slint::SharedString(),  // Summary
      count,  // Elements
      false,  // Is external
      slint::SharedString(),  // User data
  });
  if (!mUncategorizedRoot->childs.isEmpty()) {
    addCategoriesToModel(ui::LibraryTreeViewItemType::Uncategorized,
                         *mUncategorizedRoot,
                         mUncategorizedRoot->childs.count());
  }
  addCategoriesToModel(ui::LibraryTreeViewItemType::ComponentCategory,
                       *mCmpCatRoot, mCmpCatElementCount);
  addCategoriesToModel(ui::LibraryTreeViewItemType::PackageCategory,
                       *mPkgCatRoot, mPkgCatElementCount);

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";

  // Refresh filtered elements e.g. after the rescan has finished.
  setSelectedCategory(mCategories->row_data(mCurrentCategoryIndex));
}

std::shared_ptr<LibraryTab::TreeItem> LibraryTab::createRootItem(
    ui::LibraryTreeViewItemType type) noexcept {
  Uuid uuid = Uuid::createRandom();
  std::shared_ptr<TreeItem> root(new TreeItem{
      type,
      FilePath(),
      QString(),
      QString(),
      false,
      uuid.toStr(),
      {},
  });
  mLibElementsMap.insert(uuid.toStr(), root);
  return root;
}

template <typename CategoryType>
void LibraryTab::loadCategories(ui::LibraryTreeViewItemType type,
                                TreeItem& root) {
  try {
    const auto categories =
        mDb.getAll<CategoryType>(mLibrary.getDirectory().getAbsPath());
    mLibCategories.insert(categories);
    for (auto it = categories.begin(); it != categories.end(); it++) {
      getOrCreateCategory<CategoryType>(type, *it, root);
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to load categories:" << e.getMsg();
  }
}

template <typename CategoryType>
std::shared_ptr<LibraryTab::TreeItem> LibraryTab::getOrCreateCategory(
    ui::LibraryTreeViewItemType type, const Uuid& uuid, TreeItem& root) {
  auto it = mLibElementsMap.find(uuid.toStr());
  if (it != mLibElementsMap.end()) {
    return *it;
  }

  auto item = std::make_shared<TreeItem>();
  item->type = type;
  item->userData = uuid.toStr();
  try {
    FilePath fp = mLibCategories.key(uuid);
    if (!fp.isValid()) {
      fp = mDb.getLatest<CategoryType>(uuid);
      item->isExternal = true;
    } else {
      item->path = fp;
      item->isExternal = false;
    }
    if ((!fp.isValid()) ||
        (!mDb.getTranslations<CategoryType>(fp, mLocaleOrder, &item->name))) {
      item->name = tr("Unknown") % " (" % uuid.toStr() % ")";
    }
    std::optional<Uuid> parentUuid;
    if (fp.isValid()) {
      mDb.getCategoryMetadata<CategoryType>(fp, &parentUuid);
    }
    auto parent = &root;
    if (parentUuid) {
      if (auto p = getOrCreateCategory<CategoryType>(type, *parentUuid, root)) {
        parent = p.get();
      }
    }
    parent->childs.append(item);
  } catch (const Exception& e) {
    qCritical() << "Failed to load category:" << e.getMsg();
  }
  mLibElementsMap.insert(uuid.toStr(), item);
  return item;
}

template <typename ElementType, typename CategoryType>
void LibraryTab::loadElements(ui::LibraryTreeViewItemType type,
                              ui::LibraryTreeViewItemType catType,
                              TreeItem& root, int& count) {
  try {
    const QSet<FilePath> elements = Toolbox::toSet(
        mDb.getAll<ElementType>(mLibrary.getDirectory().getAbsPath()).keys());
    count += elements.count();
    for (const FilePath& fp : elements) {
      auto item = std::make_shared<TreeItem>();
      item->type = type;
      item->path = fp;
      item->userData = fp.toStr();
      mDb.getTranslations<ElementType>(fp, mLocaleOrder, &item->name,
                                       &item->summary);
      item->summary = item->summary.split("\n").first().left(200);

      bool addedToCategory = false;
      for (const Uuid& catUuid : mDb.getCategoriesOf<ElementType>(fp)) {
        if (auto cat =
                getOrCreateCategory<CategoryType>(catType, catUuid, root)) {
          cat->childs.push_back(item);
          addedToCategory = true;
        }
      }
      if (!addedToCategory) {
        mUncategorizedRoot->childs.push_back(item);
      }
      mLibElementsMap.insert(fp.toStr(), item);
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to load elements:" << e.getMsg();
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
          return cmp(lhs->name, rhs->name);
        }
      },
      Qt::CaseInsensitive, false);
  for (auto child : items) {
    sortItemsRecursive(child->childs);
  }
}

void LibraryTab::addCategoriesToModel(ui::LibraryTreeViewItemType type,
                                      TreeItem& root, int count) noexcept {
  mCategories->push_back(ui::LibraryTreeViewItemData{
      type,  // Type
      0,  // Level
      q2s(root.name),  // Name
      slint::SharedString(),  // Summary
      count,  // Elements
      false,  // Is external
      q2s(root.userData),  // User data
  });
  addCategoriesToModel(root, type, *mCategories, 1);
}

void LibraryTab::addCategoriesToModel(
    TreeItem& item, ui::LibraryTreeViewItemType type,
    slint::VectorModel<ui::LibraryTreeViewItemData>& model,
    int level) noexcept {
  for (auto child : item.childs) {
    if (child->type == type) {
      const int count =
          std::count_if(child->childs.begin(), child->childs.end(),
                        [type](const std::shared_ptr<TreeItem>& item) {
                          return item->type != type;
                        });
      model.push_back(ui::LibraryTreeViewItemData{
          type,  // Type
          level,  // Level
          q2s(child->name),  // Name
          q2s(child->summary),  // Summary
          count,  // Elements
          child->isExternal,  // Is external
          q2s(child->userData),  // User data
      });
    }
    addCategoriesToModel(*child, type, model, level + 1);
  }
}

void LibraryTab::setSelectedCategory(
    const std::optional<ui::LibraryTreeViewItemData>& data) noexcept {
  const bool isRoot = data ? (data->level == 0) : true;
  const std::optional<Uuid> uuid =
      data ? Uuid::tryFromString(s2q(data->user_data)) : std::nullopt;

  QVector<std::shared_ptr<TreeItem>> items;
  if (isRoot) {
    if (uuid) {
      if (auto item = mLibElementsMap.value(uuid->toStr())) {
        QSet<std::shared_ptr<TreeItem>> set;
        getChildsRecursive(*item, set);
        items = Toolbox::toList(set);
      }
    } else {
      items = mLibElementsMap.values().toVector();
    }
    sortItemsRecursive(items);
  } else if (uuid) {
    if (auto item = mLibElementsMap.value(uuid->toStr())) {
      items = item->childs;
    }
  }

  std::vector<ui::LibraryTreeViewItemData> rows;
  rows.reserve(items.count() + 4);
  std::optional<ui::LibraryTreeViewItemType> type;
  const QSet<ui::LibraryTreeViewItemType> types = {
      ui::LibraryTreeViewItemType::Symbol,
      ui::LibraryTreeViewItemType::Package,
      ui::LibraryTreeViewItemType::Component,
      ui::LibraryTreeViewItemType::Device,
  };
  for (auto item : items) {
    if (!types.contains(item->type)) {
      continue;
    }
    if (item->type != type) {
      rows.push_back(ui::LibraryTreeViewItemData{
          item->type,  // Type
          0,  // Level
          slint::SharedString(),  // Name
          slint::SharedString(),  // Summary
          0,  // Elements
          false,  // Is external
          slint::SharedString(),  // User data
      });
      type = item->type;
    }
    rows.push_back(ui::LibraryTreeViewItemData{
        item->type,  // Type
        1,  // Level
        q2s(item->name),  // Name
        q2s(item->summary),  // Summary
        0,  // Elements
        false,  // Is external
        q2s(item->userData),  // User data
    });
  }
  mElements->set_vector(std::move(rows));
}

void LibraryTab::getChildsRecursive(
    TreeItem& item, QSet<std::shared_ptr<TreeItem>>& childs) noexcept {
  childs |= Toolbox::toSet(item.childs);
  for (auto child : item.childs) {
    getChildsRecursive(*child, childs);
  }
}

QList<std::shared_ptr<LibraryTab::TreeItem>> LibraryTab::getSelectedCategories()
    const noexcept {
  QList<std::shared_ptr<TreeItem>> result;
  if (auto data = mCategories->row_data(mCurrentCategoryIndex)) {
    if (auto item = mLibElementsMap.value(s2q(data->user_data))) {
      if (item->path.isValid()) {
        result.append(item);
      }
    }
  }
  return result;
}

QList<std::shared_ptr<LibraryTab::TreeItem>> LibraryTab::getSelectedElements()
    const noexcept {
  QList<std::shared_ptr<TreeItem>> result;
  if (auto data = mFilteredElements->row_data(mCurrentElementIndex)) {
    if (auto item = mLibElementsMap.value(s2q(data->user_data))) {
      if (item->path.isValid()) {
        result.append(item);
      }
    }
  }
  return result;
}

void LibraryTab::duplicateElements(
    const QList<std::shared_ptr<TreeItem>>& items) noexcept {
  if (items.count() != 1) {
    return;
  }

  auto item = items.first();
  switch (item->type) {
    case ui::LibraryTreeViewItemType::ComponentCategory: {
      emit componentCategoryEditorRequested(mEditor, item->path, true);
      break;
    }
    case ui::LibraryTreeViewItemType::PackageCategory: {
      emit packageCategoryEditorRequested(mEditor, item->path, true);
      break;
    }
    case ui::LibraryTreeViewItemType::Symbol: {
      emit symbolEditorRequested(mEditor, item->path, true);
      break;
    }
    case ui::LibraryTreeViewItemType::Package: {
      emit packageEditorRequested(mEditor, item->path, true);
      break;
    }
    case ui::LibraryTreeViewItemType::Component: {
      mEditor.duplicateInLegacyComponentEditor(item->path);
      break;
    }
    case ui::LibraryTreeViewItemType::Device: {
      mEditor.duplicateInLegacyDeviceEditor(item->path);
      break;
    }
    default: {
      break;
    }
  }
}

void LibraryTab::moveElementsTo(const QList<std::shared_ptr<TreeItem>>& items,
                                const FilePath& dstLib) noexcept {
  // Destination path sanity check.
  if ((!dstLib.isValid()) || (dstLib == mEditor.getFilePath()) ||
      (!dstLib.isLocatedInDir(
          mEditor.getWorkspace().getLocalLibrariesPath()))) {
    return;
  }

  // Get the destination library name.
  QString libName = dstLib.toNative();
  try {
    mEditor.getWorkspace().getLibraryDb().getTranslations<Library>(
        dstLib, mEditor.getWorkspace().getSettings().libraryLocaleOrder.get(),
        &libName);  // can throw
  } catch (const Exception&) {
  }

  // Extract names and file paths.
  QStringList names;
  QSet<FilePath> paths;
  for (auto item : items) {
    names.append(item->name);
    paths.insert(item->path);
  }
  Toolbox::sortNumeric(names);

  // Build message (list only the first few elements to avoid a huge message
  // box)
  QString msg =
      tr("Are you sure to move the following elements into the library '%1'?")
          .arg(libName) %
      "\n\n";
  const QStringList listedNames = names.mid(0, 10);
  for (const QString& name : listedNames) {
    msg.append(" - " % name % "\n");
  }
  if (names.count() > listedNames.count()) {
    msg.append(" - ...\n");
  }
  msg.append("\n" % tr("Note: This operation cannot be easily undone!") % "\n");

  // Show confirmation dialog.
  QDialog dialog(qApp->activeWindow());
  dialog.setWindowTitle(tr("Move %1 Elements").arg(items.count()));
  QVBoxLayout* vLayoutOuter = new QVBoxLayout(&dialog);
  QHBoxLayout* hLayoutTop = new QHBoxLayout();
  vLayoutOuter->addItem(hLayoutTop);
  hLayoutTop->setSpacing(9);
  QVBoxLayout* vLayoutLeft = new QVBoxLayout();
  hLayoutTop->addItem(vLayoutLeft);
  QLabel* lblIcon = new QLabel(&dialog);
  lblIcon->setPixmap(QPixmap(":/img/status/dialog_warning.png"));
  lblIcon->setScaledContents(true);
  lblIcon->setFixedSize(48, 48);
  vLayoutLeft->addWidget(lblIcon);
  vLayoutLeft->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
                                       QSizePolicy::MinimumExpanding));
  QVBoxLayout* vLayoutRight = new QVBoxLayout();
  hLayoutTop->addItem(vLayoutRight);
  QLabel* lblMsg = new QLabel(msg, &dialog);
  lblMsg->setMinimumWidth(300);
  lblMsg->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  lblMsg->setWordWrap(true);
  vLayoutRight->addWidget(lblMsg);
  QHBoxLayout* hLayoutBot = new QHBoxLayout();
  hLayoutBot->setSpacing(9);
  vLayoutOuter->addItem(hLayoutBot);
  QCheckBox* cbxKeep = new QCheckBox(
      tr("Keep elements in current library (make a copy)"), &dialog);
  cbxKeep->setChecked(!mEditor.isWritable());
  cbxKeep->setEnabled(mEditor.isWritable());
  hLayoutBot->addWidget(cbxKeep);
  hLayoutBot->setStretch(0, 1);
  QDialogButtonBox* btnBox =
      new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::Cancel,
                           Qt::Horizontal, &dialog);
  connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  hLayoutBot->addWidget(btnBox);
  const int ret = dialog.exec();

  if (ret == QDialog::Accepted) {
    // Close opened tabs of elements to be moved.
    // TODO: Ask for saving if there are unsaved changes!
    mEditor.forceClosingTabs(paths);
    for (const FilePath& fp : paths) {
      const QString relPath = fp.toRelative(fp.getParentDir().getParentDir());
      const FilePath destination = dstLib.getPathTo(relPath);
      try {
        if (!cbxKeep->isChecked()) {
          qInfo().nospace() << "Move library element from " << fp.toNative()
                            << " to " << destination.toNative() << "...";
          FileUtils::move(fp, destination);
        } else {
          qInfo().nospace() << "Copy library element from " << fp.toNative()
                            << " to " << destination.toNative() << "...";
          FileUtils::copyDirRecursively(fp, destination);
        }
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
    }
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
  }
}

void LibraryTab::deleteElements(
    const QList<std::shared_ptr<TreeItem>>& items) noexcept {
  if (items.isEmpty()) {
    return;
  }

  // Extract names and file paths.
  QStringList names;
  QSet<FilePath> paths;
  for (auto item : items) {
    names.append(item->name);
    paths.insert(item->path);
  }
  Toolbox::sortNumeric(names);

  // Build message (list only the first few elements to avoid a huge message
  // box)
  QString msg = tr("WARNING: Library elements must normally NOT be removed "
                   "because this will break other elements which depend on "
                   "this one! They should be just marked as deprecated "
                   "instead.\n\nAre you still sure to delete the following "
                   "library elements?") %
      "\n\n";
  const QStringList listedNames = names.mid(0, 10);
  for (const QString& name : listedNames) {
    msg.append(" - " % name % "\n");
  }
  if (names.count() > listedNames.count()) {
    msg.append(" - ...\n");
  }
  msg.append("\n" % tr("This cannot be undone!"));

  // Show message box
  const int ret = QMessageBox::warning(
      qApp->activeWindow(), tr("Remove %1 Elements").arg(items.count()), msg,
      QMessageBox::Yes, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes) {
    // Close opened tabs of elements to be deleted.
    mEditor.forceClosingTabs(paths);
    for (const FilePath& fp : paths) {
      try {
        FileUtils::removeDirRecursively(fp);
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
    }
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

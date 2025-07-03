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
#include "libraryelementsmodel.h"

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
    mDirPath(mLibrary.getDirectory().getAbsPath()),
    mWizardMode(wizardMode),
    mCurrentPageIndex(wizardMode ? 0 : 2),
    mCurrentCategoryIndex(0),
    mCurrentElementIndex(-1),
    mNameParsed(mLibrary.getNames().getDefaultValue()),
    mVersionParsed(mLibrary.getVersion()),
    mDeprecated(false),
    mCmpCatElementCount(0),
    mPkgCatElementCount(0),
    mCategories(new slint::VectorModel<ui::TreeViewItemData>()),
    mFilteredElements(new slint::VectorModel<ui::TreeViewItemData>()),
    mDependencies(new LibraryDependenciesModel(editor.getWorkspace(),
                                               mLibrary.getUuid())) {
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
          &LibraryTab::refreshMetadata);

  // Connect models.
  connect(mDependencies.get(), &LibraryDependenciesModel::modified, this,
          &LibraryTab::commitDependencies, Qt::QueuedConnection);

  // Refresh content.
  refreshMetadata();
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

ui::TabData LibraryTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mLibrary.getDirectory().isWritable());
  features.undo = toFs(mEditor.getUndoStack().canUndo());
  features.redo = toFs(mEditor.getUndoStack().canRedo());

  return ui::TabData{
      ui::TabType::Library,  // Type
      q2s(*mLibrary.getNames().getDefaultValue()),  // Title
      features,  // Features
      !mLibrary.getDirectory().isWritable(),  // Read-only
      mEditor.hasUnsavedChanges(),  // Unsaved changes
      q2s(mEditor.getUndoStack().getUndoCmdText()),  // Undo text
      q2s(mEditor.getUndoStack().getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
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
  mCurrentElementIndex = data.element_index;

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void LibraryTab::trigger(ui::TabAction a) noexcept {
  auto data = (mCurrentElementIndex == -1)
      ? mCategories->row_data(mCurrentCategoryIndex)
      : mFilteredElements->row_data(mCurrentElementIndex);
  const QString userData = data ? s2q(data->user_data) : QString();
  const std::optional<Uuid> uuid = Uuid::tryFromString(userData);
  const FilePath fp = uuid ? mLibCategories.key(*uuid) : FilePath(userData);
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
      if (mWizardMode && (mCurrentPageIndex == 1)) {
        mWizardMode = false;
        ++mCurrentPageIndex;
      }
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Apply: {
      commitMetadata();
      refreshMetadata();
      break;
    }
    case ui::TabAction::Save: {
      commitMetadata();

      // Remove obsolete message approvals (bypassing the undo stack).
      mLibrary.setMessageApprovals(mLibrary.getMessageApprovals() -
                                   mDisappearedApprovals);

      if (mEditor.save() && mWizardMode && (mCurrentPageIndex == 0)) {
        ++mCurrentPageIndex;
      }
      refreshMetadata();
      break;
    }
    case ui::TabAction::Delete: {
      qDebug() << "Delete" << fp;
      break;
    }
    case ui::TabAction::EditProperties: {
      if (fp.isValid() && item) {
        switch (item->type) {
          case TreeItemType::ComponentCategory: {
            emit componentCategoryEditorRequested(mEditor, fp);
            break;
          }
          case TreeItemType::PackageCategory: {
            emit packageCategoryEditorRequested(mEditor, fp);
            break;
          }
          case TreeItemType::Symbol: {
            emit symbolEditorRequested(mEditor, fp);
            break;
          }
          case TreeItemType::Package: {
            emit packageEditorRequested(mEditor, fp);
            break;
          }
          case TreeItemType::Component: {
            emit componentEditorRequested(mEditor, fp);
            break;
          }
          case TreeItemType::Device: {
            emit deviceEditorRequested(mEditor, fp);
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
        commitMetadata();
        mEditor.getUndoStack().undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitMetadata();
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
          commitMetadata();
        } catch (const Exception& e) {
          QMessageBox::critical(qApp->activeWindow(), tr("Could not open file"),
                                e.getMsg());
        }
      }
      break;
    }
    case ui::TabAction::Close: {
      commitMetadata();
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

template <>
void LibraryTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitMetadata();
}

template <>
void LibraryTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitMetadata();
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
 *  Private Methods
 ******************************************************************************/

void LibraryTab::refreshMetadata() noexcept {
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
  mManufacturer = q2s(*mLibrary.getManufacturer());

  mDependencies->setUuids(mLibrary.getDependencies());

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void LibraryTab::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryEdit> cmd(new CmdLibraryEdit(mLibrary));
    cmd->setIcon(mIcon);
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mLibrary.getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    const QString urlStr = s2q(mUrl).trimmed();
    if (urlStr != mLibrary.getUrl().toString()) {
      cmd->setUrl(QUrl(urlStr, QUrl::TolerantMode));
    }
    cmd->setManufacturer(cleanSimpleString(s2q(mManufacturer)));
    mEditor.getUndoStack().execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void LibraryTab::commitDependencies() noexcept {
  try {
    std::unique_ptr<CmdLibraryEdit> cmd(new CmdLibraryEdit(mLibrary));
    cmd->setDependencies(mDependencies->getUuids());
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

  mUncategorizedRoot = createRootItem(TreeItemType::Uncategorized,
                                      QIcon(":/img/status/dialog_warning.png"),
                                      tr("Not Categorized"));
  mCmpCatRoot = createRootItem(TreeItemType::ComponentCategory, QIcon(),
                               tr("Component Categories"));
  mPkgCatRoot = createRootItem(TreeItemType::PackageCategory, QIcon(),
                               tr("Package Categories"));

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

  sortItemsRecursive(mCmpCatRoot->childs);
  sortItemsRecursive(mPkgCatRoot->childs);

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
  if (!mUncategorizedRoot->childs.isEmpty()) {
    addCategoriesToModel(TreeItemType::Uncategorized, *mUncategorizedRoot,
                         mUncategorizedRoot->childs.count());
  }
  addCategoriesToModel(TreeItemType::ComponentCategory, *mCmpCatRoot,
                       mCmpCatElementCount);
  addCategoriesToModel(TreeItemType::PackageCategory, *mPkgCatRoot,
                       mPkgCatElementCount);

  qDebug() << "Finished category tree model update in" << t.elapsed() << "ms.";

  // Refresh filtered elements e.g. after the rescan has finished.
  setSelectedCategory(mCategories->row_data(mCurrentCategoryIndex));
}

std::shared_ptr<LibraryTab::TreeItem> LibraryTab::createRootItem(
    TreeItemType type, const QIcon& icon, const QString& text) noexcept {
  Uuid uuid = Uuid::createRandom();
  std::shared_ptr<TreeItem> root(new TreeItem{
      type,
      q2s(icon.pixmap(32)),
      text,
      QString(),
      QString(),
      uuid.toStr(),
      {},
  });
  mLibElementsMap.insert(uuid.toStr(), root);
  return root;
}

template <typename CategoryType>
void LibraryTab::loadCategories(TreeItemType type, const QIcon& icon,
                                TreeItem& root) {
  try {
    const auto categories = mDb.getAll<CategoryType>(mDirPath);
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
  mLibElementsMap.insert(uuid.toStr(), item);
  return item;
}

template <typename ElementType, typename CategoryType>
void LibraryTab::loadElements(TreeItemType type, slint::Image icon,
                              TreeItemType catType, const QIcon& catIcon,
                              TreeItem& root, int& count) {
  try {
    const QSet<FilePath> elements =
        Toolbox::toSet(mDb.getAll<ElementType>(mDirPath).keys());
    count += elements.count();
    for (const FilePath& fp : elements) {
      auto item = std::make_shared<TreeItem>();
      item->type = type;
      item->icon = icon;
      item->userData = fp.toStr();
      mDb.getTranslations<ElementType>(fp, mLocaleOrder, &item->text,
                                       &item->hint);

      QString mpn = item->text;
      if (mpn.endsWith(")")) {
        mpn = mpn.split("(", Qt::SkipEmptyParts).first().trimmed();
      }
      mpn = mpn.split(" ", Qt::SkipEmptyParts).last();
      mpn.replace("x", "X");
      if ((mpn.length() > 3) && (mpn.toUpper() == mpn)) {
        item->comment = item->hint.split("\n").first().trimmed();
      }

      bool addedToCategory = false;
      for (const Uuid& catUuid : mDb.getCategoriesOf<ElementType>(fp)) {
        if (auto cat = getOrCreateCategory<CategoryType>(catType, catIcon,
                                                         catUuid, root)) {
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
          false,  // Italic
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
        q2s(item->comment),  // Comment
        q2s(item->hint),  // Comment
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

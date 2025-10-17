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
#include "componenttab.h"

#include "../../guiapplication.h"
#include "../../modelview/attributelistmodel.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdcomponentedit.h"
#include "../cmd/cmdcomponentsignaledit.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "../libraryeditor.h"
#include "../libraryelementcategoriesmodel.h"
#include "componentsignallistmodel.h"
#include "componentsignalnamelistmodel.h"
#include "componentvariantlistmodel.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentcheckmessages.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
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

ComponentTab::ComponentTab(LibraryEditor& editor,
                           std::unique_ptr<Component> cmp, Mode mode,
                           QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mComponent(std::move(cmp)),
    mIsNewElement(isPathOutsideLibDir()),
    mWizardMode(mode != Mode::Open),
    mCurrentPageIndex(mWizardMode ? 0 : 2),
    mChooseCategory(false),
    mNameParsed(mComponent->getNames().getDefaultValue()),
    mVersionParsed(mComponent->getVersion()),
    mDeprecated(false),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                          editor.getWorkspace().getSettings(),
                                          CategoryTreeModel::Filter::CmpCat)),
    mSchematicOnly(false),
    mPrefixParsed(mComponent->getPrefixes().getDefaultValue()),
    mAttributes(new AttributeListModel()),
    mSignals(new ComponentSignalListModel()),
    mSignalsSorted(new slint::SortModel<ui::ComponentSignalData>(
        mSignals,
        [](const ui::ComponentSignalData& a, const ui::ComponentSignalData& b) {
          return a.sort_index < b.sort_index;
        })),
    mSignalNames(new ComponentSignalNameListModel()),
    mVariants(new ComponentVariantListModel(mApp.getWorkspace(),
                                            mApp.getPreviewLayers(),
                                            mApp.getLibraryElementCache())),
    mIsInterfaceBroken(false),
    mOriginalIsSchematicOnly(mComponent->isSchematicOnly()),
    mOriginalSignalUuids(mComponent->getSignals().getUuidSet()),
    mOriginalSymbolVariants(mComponent->getSymbolVariants()) {
  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentTab::refreshUiData);

  // Connect models.
  mAttributes->setReferences(&mComponent->getAttributes(), mUndoStack.get());
  mSignals->setReferences(mComponent.get(), mUndoStack.get());
  mSignalNames->setReferences(&mComponent->getSignals(), mUndoStack.get());
  mVariants->setReferences(&mComponent->getSymbolVariants(), mComponent.get(),
                           mSignalNames, mUndoStack.get(), &mWizardMode);
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &ComponentTab::commitUiData, Qt::QueuedConnection);

  // Refresh content.
  refreshUiData();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (mode == Mode::New) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }

  // Make save button primary if it's a new element.
  if (mode != Mode::Open) {
    mManualModificationsMade = true;
  }
}

ComponentTab::~ComponentTab() noexcept {
  deactivate();

  // Reset references to avoid dangling pointers as the UI might still have
  // shared pointers to these models.
  mVariants->setReferences(nullptr, nullptr, nullptr, nullptr, nullptr);
  mSignalNames->setReferences(nullptr, nullptr);
  mSignals->setReferences(nullptr, nullptr);
  mAttributes->setReferences(nullptr, nullptr);

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath ComponentTab::getDirectoryPath() const noexcept {
  return mComponent->getDirectory().getAbsPath();
}

ui::TabData ComponentTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::Component,  // Type
      q2s(*mComponent->getNames().getDefaultValue()),  // Title
      features,  // Features
      !writable,  // Read-only
      hasUnsavedChanges(),  // Unsaved changes
      q2s(mUndoStack->getUndoCmdText()),  // Undo text
      q2s(mUndoStack->getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::ComponentTabData ComponentTab::getDerivedUiData() const noexcept {
  return ui::ComponentTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mComponent->getDirectory().getAbsPath().toStr()),  // Path
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      mName,  // Name
      mNameError,  // Name error
      mDescription,  // Description
      mKeywords,  // Keywords
      mAuthor,  // Author
      mVersion,  // Version
      mVersionError,  // Version error
      mDeprecated,  // Deprecated
      mCategories,  // Categories
      mCategoriesTree,  // Categories tree
      mChooseCategory,  // Choose category
      mDatasheetUrl,  // Datasheet URL
      mDatasheetUrlError,  // Datasheet URL error
      mSchematicOnly,  // Schematic-only
      mPrefix,  // Prefix
      mPrefixError,  // Prefix error
      mDefaultValue,  // Default value
      mDefaultValueError,  // Default value error
      mAttributes,  // Attributes
      mSignalsSorted,  // Signals
      mNewSignalName,  // New signal name
      mNewSignalNameError,  // New signal name error
      mSignalNames,  // Signal names
      mVariants,  // Variants
      ui::RuleCheckData{
          ui::RuleCheckType::ComponentCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Check execution error
          !isWritable(),  // Check read-only
      },
      l2s(mApp.getWorkspace().getSettings().defaultLengthUnit.get()),  // Unit
      mIsInterfaceBroken,  // Interface broken
      slint::SharedString(),  // New category
  };
}

void ComponentTab::setDerivedUiData(const ui::ComponentTabData& data) noexcept {
  // Page change
  if (data.page_index != mCurrentPageIndex) {
    mCurrentPageIndex = data.page_index;
    onUiDataChanged.notify();
  }

  // Metadata
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
  if (auto uuid = Uuid::tryFromString(s2q(data.new_category))) {
    mCategories->add(*uuid);
  }
  mChooseCategory = data.choose_category;
  mDatasheetUrl = data.datasheet_url;
  validateUrl(s2q(mDatasheetUrl), mDatasheetUrlError, true);
  mSchematicOnly = data.schematic_only;
  mPrefix = data.prefix;
  if (auto value = validateComponentPrefix(s2q(mPrefix), mPrefixError)) {
    mPrefixParsed = *value;
  }
  mDefaultValue = data.default_value;
  validateComponentDefaultValue(s2q(mDefaultValue), mDefaultValueError);

  // New signal
  if (data.new_signal_name != mNewSignalName) {
    mNewSignalName = data.new_signal_name;
    const QString name = s2q(mNewSignalName);
    const QStringList names = Toolbox::expandRangesInString(name);
    const bool duplicate =
        std::any_of(names.begin(), names.end(), [this](const QString& n) {
          return mComponent->getSignals().contains(cleanCircuitIdentifier(n));
        });
    if (!name.trimmed().isEmpty()) {
      validateCircuitIdentifier(names.value(0), mNewSignalNameError, duplicate);
    } else {
      mNewSignalNameError = slint::SharedString();
    }
  }

  onDerivedUiDataChanged.notify();
}

void ComponentTab::trigger(ui::TabAction a) noexcept {
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
      if (mWizardMode && (mCurrentPageIndex == 0)) {
        ++mCurrentPageIndex;
        save();
      } else if (mWizardMode && (mCurrentPageIndex == 1)) {
        ++mCurrentPageIndex;
        save();
      } else if (mWizardMode && (mCurrentPageIndex == 2)) {
        mWizardMode = false;
        scheduleChecks();
        save();
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
      save();
      break;
    }
    case ui::TabAction::Undo: {
      try {
        commitUiData();
        mUndoStack->undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitUiData();
        mUndoStack->redo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Close: {
      if (requestClose()) {
        WindowTab::trigger(a);
      }
      break;
    }
    case ui::TabAction::OpenDatasheet: {
      commitUiData();
      if (auto dbRes = mComponent->getResources().value(0)) {
        DesktopServices::downloadAndOpenResourceAsync(
            mApp.getWorkspace().getSettings(), *dbRes->getName(),
            dbRes->getMediaType(), dbRes->getUrl(), qApp->activeWindow());
      }
      break;
    }
    case ui::TabAction::ComponentAddSignals: {
      if (mSignals->add(s2q(mNewSignalName))) {
        mNewSignalName = slint::SharedString();
        mNewSignalNameError = slint::SharedString();
        onDerivedUiDataChanged.notify();
      }
      break;
    }
    case ui::TabAction::ComponentAddVariant: {
      mVariants->add();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image ComponentTab::renderScene(float width, float height,
                                       int scene) noexcept {
  const int variant = scene / 1000;
  const int gate = scene % 1000;
  return mVariants->renderScene(variant, gate, width, height);
}

bool ComponentTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The component '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mComponent->getNames().getDefaultValue()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);
  if (choice == QMessageBox::Yes) {
    return save();
  } else if (choice == QMessageBox::No) {
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
    ComponentTab::runChecksImpl() {
  // Do not run checks during wizard mode as it would be too early.
  if (mWizardMode) {
    return std::nullopt;
  }

  return std::make_pair(mComponent->runChecks(),
                        mComponent->getMessageApprovals());
}

bool ComponentTab::autoFixImpl(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingCategories>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingComponentDefaultValue>(msg, checkOnly))
    return true;
  if (autoFixHelper<MsgMissingSymbolVariant>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgNonFunctionalComponentSignalInversionSign>(msg,
                                                                  checkOnly))
    return true;
  return false;
}

template <typename MessageType>
bool ComponentTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (checkOnly) {
        return true;
      } else {
        return autoFix(*m);  // can throw
      }
    }
  }
  return false;
}

void ComponentTab::messageApprovalChanged(const SExpression& approval,
                                          bool approved) noexcept {
  if (mComponent->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void ComponentTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
bool ComponentTab::autoFix(const MsgNameNotTitleCase& msg) {
  mCurrentPageIndex = 0;
  mNameParsed = msg.getFixedName();
  commitUiData();
  return true;
}

template <>
bool ComponentTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
  return true;
}

template <>
bool ComponentTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mChooseCategory = true;
  onDerivedUiDataChanged.notify();
  return true;
}

template <>
bool ComponentTab::autoFix(const MsgMissingComponentDefaultValue& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  onDerivedUiDataChanged.notify();

  // User has to answer the one-million-dollar question :-)
  const QString title = tr("Determine default value");
  const QString question =
      tr("Is this rather a (manufacturer-)specific component than a generic "
         "component?");
  const int answer = QMessageBox::question(
      qApp->activeWindow(), title, question,
      QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No |
          QMessageBox::StandardButton::Cancel,
      QMessageBox::StandardButton::Cancel);
  if (answer == QMessageBox::Yes) {
    mDefaultValue = "{{MPN or DEVICE or COMPONENT}}";
    commitUiData();
    refreshUiData();
    return true;
  } else if (answer == QMessageBox::No) {
    mDefaultValue = "{{MPN or DEVICE}}";
    commitUiData();
    refreshUiData();
    return true;
  } else {
    return false;  // Aborted
  }
}

template <>
bool ComponentTab::autoFix(const MsgMissingSymbolVariant& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<ComponentSymbolVariant> symbVar =
      std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), "",
                                               ElementName("default"), "");
  mUndoStack->execCmd(new CmdComponentSymbolVariantInsert(
      mComponent->getSymbolVariants(), symbVar));

  mCurrentPageIndex = 2;
  onDerivedUiDataChanged.notify();
  return true;
}

template <>
bool ComponentTab::autoFix(
    const MsgNonFunctionalComponentSignalInversionSign& msg) {
  std::shared_ptr<ComponentSignal> signal =
      mComponent->getSignals().get(msg.getSignal().get());
  std::unique_ptr<CmdComponentSignalEdit> cmd(
      new CmdComponentSignalEdit(*signal));
  cmd->setName(CircuitIdentifier("!" % signal->getName()->mid(1)));
  mUndoStack->execCmd(cmd.release());

  mCurrentPageIndex = 1;
  onDerivedUiDataChanged.notify();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ComponentTab::isWritable() const noexcept {
  return mIsNewElement || mComponent->getDirectory().isWritable();
}

bool ComponentTab::isInterfaceBroken() const noexcept {
  if ((mIsNewElement) || (mWizardMode)) return false;

  if (mComponent->isSchematicOnly() != mOriginalIsSchematicOnly) {
    return true;
  }
  if (mComponent->getSignals().getUuidSet() != mOriginalSignalUuids) {
    return true;
  }
  for (const ComponentSymbolVariant& original : mOriginalSymbolVariants) {
    const ComponentSymbolVariant* current =
        mComponent->getSymbolVariants().find(original.getUuid()).get();
    if (!current) return true;
    if (current->getSymbolItems().getUuidSet() !=
        original.getSymbolItems().getUuidSet()) {
      return true;
    }
    for (const ComponentSymbolVariantItem& originalItem :
         original.getSymbolItems()) {
      const ComponentSymbolVariantItem* currentItem =
          current->getSymbolItems().find(originalItem.getUuid()).get();
      if (!currentItem) return true;
      if (currentItem->getSymbolUuid() != originalItem.getSymbolUuid()) {
        return true;
      }
      if (currentItem->getPinSignalMap().getUuidSet() !=
          originalItem.getPinSignalMap().getUuidSet()) {
        return true;
      }
      for (const ComponentPinSignalMapItem& originalMap :
           originalItem.getPinSignalMap()) {
        const ComponentPinSignalMapItem* currentMap =
            currentItem->getPinSignalMap().find(originalMap.getUuid()).get();
        if (!currentMap) return true;
        if (currentMap->getSignalUuid() != originalMap.getSignalUuid()) {
          return true;
        }
      }
    }
  }
  return false;
}

void ComponentTab::refreshUiData() noexcept {
  mName = q2s(*mComponent->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mComponent->getNames().getDefaultValue();
  mDescription = q2s(mComponent->getDescriptions().getDefaultValue());
  mKeywords = q2s(mComponent->getKeywords().getDefaultValue());
  mAuthor = q2s(mComponent->getAuthor());
  mVersion = q2s(mComponent->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mComponent->getVersion();
  mDeprecated = mComponent->isDeprecated();
  mCategories->setCategories(mComponent->getCategories());
  mSchematicOnly = mComponent->isSchematicOnly();
  mPrefix = q2s(*mComponent->getPrefixes().getDefaultValue());
  validateComponentPrefix(s2q(mPrefix), mPrefixError);
  mPrefixParsed = mComponent->getPrefixes().getDefaultValue();
  mDefaultValue = q2s(mComponent->getDefaultValue());
  validateComponentDefaultValue(s2q(mDefaultValue), mDefaultValueError);

  if (auto dbRes = mComponent->getResources().value(0)) {
    mDatasheetUrl = q2s(dbRes->getUrl().toString());
  } else {
    mDatasheetUrl = slint::SharedString();
  }
  mDatasheetUrlError = slint::SharedString();

  // Update "interface broken" only when no command is active since it would
  // be annoying to get it during intermediate states.
  if (!mUndoStack->isCommandGroupActive()) {
    mIsInterfaceBroken = isInterfaceBroken();
  }

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void ComponentTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdComponentEdit> cmd(new CmdComponentEdit(*mComponent));
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mComponent->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mComponent->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mComponent->getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    cmd->setIsSchematicOnly(mSchematicOnly);
    cmd->setPrefix("", mPrefixParsed);
    const QString defaultValue = s2q(mDefaultValue);
    if (defaultValue != mComponent->getDefaultValue()) {
      cmd->setDefaultValue(defaultValue.trimmed());
    }

    try {
      ResourceList resources = mComponent->getResources();
      const ElementName name(
          cleanElementName("Datasheet " % s2q(mName).trimmed()));
      const QString dbUrlStr = s2q(mDatasheetUrl).trimmed();
      const QUrl dbUrl(dbUrlStr, QUrl::TolerantMode);
      std::shared_ptr<Resource> res = resources.value(0);
      if ((dbUrl.isValid()) && (!res)) {
        resources.append(
            std::make_shared<Resource>(name, "application/pdf", dbUrl));
      } else if ((!dbUrl.isValid()) && res) {
        resources.remove(res.get());
      } else if ((dbUrl.isValid()) && res &&
                 (dbUrlStr != res->getUrl().toString())) {
        res->setName(name);
        res->setUrl(dbUrl);
      }
      cmd->setResources(resources);
    } catch (const Exception& e) {
    }

    mUndoStack->execCmd(cmd.release());

    mAttributes->apply();
    mSignals->apply();
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool ComponentTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
    mComponent->setMessageApprovals(mComponent->getMessageApprovals() -
                                    mDisappearedApprovals);

    mComponent->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Component>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mComponent->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mComponent->saveTo(dir);
    }
    mComponent->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mManualModificationsMade = false;
    mOriginalIsSchematicOnly = mComponent->isSchematicOnly();
    mOriginalSignalUuids = mComponent->getSignals().getUuidSet();
    mOriginalSymbolVariants = mComponent->getSymbolVariants();
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();

    if (mWizardMode && (mCurrentPageIndex == 0)) {
      ++mCurrentPageIndex;
    }
    refreshUiData();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshUiData();
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

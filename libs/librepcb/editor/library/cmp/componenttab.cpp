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

#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel2.h"
#include "../cmd/cmdcomponentedit.h"
#include "../cmd/cmdcomponentsignaledit.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "../libraryeditor2.h"
#include "../libraryelementcategoriesmodel.h"
#include "componentsignallistmodel.h"
#include "componentvariantlistmodel.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentcheckmessages.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

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

ComponentTab::ComponentTab(LibraryEditor2& editor,
                           std::unique_ptr<Component> cmp, bool wizardMode,
                           QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mComponent(std::move(cmp)),
    mIsNewElement(isPathOutsideLibDir()),
    mWizardMode(wizardMode),
    mCurrentPageIndex(wizardMode ? 0 : 1),
    // mCompactLayout(false),
    mNameParsed(mComponent->getNames().getDefaultValue()),
    mVersionParsed(mComponent->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(new CategoryTreeModel2(editor.getWorkspace().getLibraryDb(),
                                           editor.getWorkspace().getSettings(),
                                           CategoryTreeModel2::Filter::CmpCat)),
    mSignals(new ComponentSignalListModel()),
    mVariants(new ComponentVariantListModel()),
    mOriginalIsSchematicOnly(mComponent->isSchematicOnly()),
    mOriginalSignalUuids(mComponent->getSignals().getUuidSet()),
    mOriginalSymbolVariants(mComponent->getSymbolVariants()) {
  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ComponentTab::refreshMetadata);

  // Connect models.
  mSignals->setSignalList(&mComponent->getSignals());
  mSignals->setUndoStack(mUndoStack.get());
  mVariants->setVariantList(&mComponent->getSymbolVariants());
  mVariants->setUndoStack(mUndoStack.get());
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &ComponentTab::commitMetadata, Qt::QueuedConnection);

  // Refresh content.
  refreshMetadata();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (mIsNewElement) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }
}

ComponentTab::~ComponentTab() noexcept {
  deactivate();

  mSignals->setSignalList(nullptr);
  mSignals->setUndoStack(nullptr);
  mVariants->setVariantList(nullptr);
  mVariants->setUndoStack(nullptr);

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
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      q2s(mComponent->getDirectory().getAbsPath().toStr()),  // Path
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
      mDatasheetUrl,  // Datasheet URL
      mDatasheetUrlError,  // Datasheet URL error
      mSchematicOnly,  // SChematic-only
      mPrefix,  // Prefix
      mPrefixError,  // Prefix error
      mDefaultValue,  // Default value
      mDefaultValueError,  // Default value error
      mSignals,  // Signals
      nullptr,  // Signal names
      mVariants,  // Variants
      ui::RuleCheckData{
          ui::RuleCheckType::ComponentCheck,  // Checks type
          ui::RuleCheckState::UpToDate,  // Checks state
          mCheckMessages,  // Checks messages
          mCheckMessages->getUnapprovedCount(),  // Checks unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Checks execution error
          !isWritable(),  // Checks read-only
      },
      isInterfaceBroken(),  // Interface broken
      // mCompactLayout,  // Compact layout
      mAddCategoryRequested ? "choose" : slint::SharedString(),  // New category
      mNewSignalName,  // New signal name
      mNewSignalNameError,  // New signal name error
      false,  // New signal commit
  };
}

void ComponentTab::setDerivedUiData(const ui::ComponentTabData& data) noexcept {
  // General
  if (data.page_index != mCurrentPageIndex) {
    mCurrentPageIndex = data.page_index;
    onUiDataChanged.notify();
  }
  // if (data.compact_layout != mCompactLayout) {
  //   mCompactLayout = data.compact_layout;
  //   onUiDataChanged.notify();
  // }

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
  mAddCategoryRequested = false;
  mDatasheetUrl = data.datasheet_url;
  validateUrl(s2q(mDatasheetUrl), mDatasheetUrlError, true);
  mSchematicOnly = data.schematic_only;
  mPrefix = data.prefix;
  validateComponentPrefix(s2q(mPrefix), mPrefixError);
  mDefaultValue = data.default_value;
  validateComponentDefaultValue(s2q(mDefaultValue), mDefaultValueError);

  mNewSignalName = data.new_signal_name;
  const QStringList newNames =
      Toolbox::expandRangesInString(s2q(mNewSignalName).trimmed());
  if (!newNames.value(0).isEmpty()) {
    validateCircuitIdentifier(newNames.value(0), mNewSignalNameError);
  } else {
    mNewSignalNameError = slint::SharedString();
  }
  if (data.new_signal_commit && (!newNames.value(0).isEmpty()) &&
      mNewSignalNameError.empty() && mSignals->add(newNames)) {
    mNewSignalName = slint::SharedString();
  }

  onDerivedUiDataChanged.notify();
}

void ComponentTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Apply: {
      commitMetadata();
      refreshMetadata();
      break;
    }
    case ui::TabAction::Save: {
      commitMetadata();
      save();
      break;
    }
    case ui::TabAction::Undo: {
      try {
        commitMetadata();
        mUndoStack->undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitMetadata();
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
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

bool ComponentTab::requestClose() noexcept {
  commitMetadata();

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
  if (autoFixHelper<MsgNoPinsInSymbolVariantConnected>(msg, checkOnly))
    return true;
  return false;
}

template <typename MessageType>
bool ComponentTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
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
void ComponentTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitMetadata();
}

template <>
void ComponentTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void ComponentTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mAddCategoryRequested = true;
  onDerivedUiDataChanged.notify();
}

template <>
void ComponentTab::autoFix(const MsgMissingComponentDefaultValue& msg) {
  Q_UNUSED(msg);
  // User has to answer the one-million-dollar question :-)
  QString title = tr("Determine default value");
  QString question =
      tr("Is this rather a (manufacturer-)specific component than a generic "
         "component?");
  int answer = QMessageBox::question(qApp->activeWindow(), title, question,
                                     QMessageBox::StandardButton::Yes |
                                         QMessageBox::StandardButton::No |
                                         QMessageBox::StandardButton::Cancel,
                                     QMessageBox::StandardButton::Cancel);
  if (answer == QMessageBox::Yes) {
    mDefaultValue = "{{MPN or DEVICE or COMPONENT}}";
    commitMetadata();
    refreshMetadata();
  } else if (answer == QMessageBox::No) {
    mDefaultValue = "{{MPN or DEVICE}}";
    commitMetadata();
    refreshMetadata();
  }
}

template <>
void ComponentTab::autoFix(const MsgMissingSymbolVariant& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<ComponentSymbolVariant> symbVar =
      std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), "",
                                               ElementName("default"), "");
  mUndoStack->execCmd(new CmdComponentSymbolVariantInsert(
      mComponent->getSymbolVariants(), symbVar));
}

template <>
void ComponentTab::autoFix(
    const MsgNonFunctionalComponentSignalInversionSign& msg) {
  std::shared_ptr<ComponentSignal> signal =
      mComponent->getSignals().get(msg.getSignal().get());
  std::unique_ptr<CmdComponentSignalEdit> cmd(
      new CmdComponentSignalEdit(*signal));
  cmd->setName(CircuitIdentifier("!" % signal->getName()->mid(1)));
  mUndoStack->execCmd(cmd.release());
}

template <>
void ComponentTab::autoFix(const MsgNoPinsInSymbolVariantConnected& msg) {
  // mUi->symbolVariantsEditorWidget->openEditor(msg.getSymbVar()->getUuid());
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

void ComponentTab::refreshMetadata() noexcept {
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
  mDefaultValue = q2s(mComponent->getDefaultValue());
  validateComponentDefaultValue(s2q(mDefaultValue), mDefaultValueError);

  if (auto dbRes = mComponent->getResources().value(0)) {
    mDatasheetUrl = q2s(dbRes->getUrl().toString());
  } else {
    mDatasheetUrl = slint::SharedString();
  }
  mDatasheetUrlError = slint::SharedString();

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void ComponentTab::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdComponentEdit> cmd(new CmdComponentEdit(*mComponent));
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mComponent->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    cmd->setIsSchematicOnly(mSchematicOnly);
    cmd->setPrefix("", ComponentPrefix(cleanComponentPrefix(s2q(mPrefix))));
    cmd->setDefaultValue(s2q(mDefaultValue).trimmed());

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

    mSignals->apply();
    mVariants->apply();
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool ComponentTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack).
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

    if (mWizardMode && (mCurrentPageIndex == 0)) {
      ++mCurrentPageIndex;
      mWizardMode = false;
      scheduleChecks();
    }
    refreshMetadata();

    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshMetadata();
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

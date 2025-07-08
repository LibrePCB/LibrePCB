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
#include "devicetab.h"

#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel.h"
#include "../cmd/cmddeviceedit.h"
#include "../libraryeditor.h"
#include "../libraryelementcategoriesmodel.h"
#include "../pkg/footprintgraphicsitem.h"
#include "../sym/symbolgraphicsitem.h"
#include "devicepinoutmodel.h"
#include "partlistmodel2.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/dev/devicecheckmessages.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
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

DeviceTab::DeviceTab(LibraryEditor& editor, std::unique_ptr<Device> dev,
                     bool wizardMode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mDevice(std::move(dev)),
    mIsNewElement(isPathOutsideLibDir()),
    mComponentScene(new GraphicsScene()),
    mPackageScene(new GraphicsScene()),
    mWizardMode(wizardMode),
    mCurrentPageIndex(wizardMode ? 0 : 1),
    // mCompactLayout(false),
    mNameParsed(mDevice->getNames().getDefaultValue()),
    mVersionParsed(mDevice->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                          editor.getWorkspace().getSettings(),
                                          CategoryTreeModel::Filter::CmpCat)),
    mPinout(new DevicePinoutModel()),
    mParts(new PartListModel2()),
    mOriginalComponentUuid(mDevice->getComponentUuid()),
    mOriginalPackageUuid(mDevice->getPackageUuid()),
    mOriginalPadSignalMap(mDevice->getPadSignalMap()) {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();

  // Setup component scene.
  mComponentScene.reset(new GraphicsScene());
  mComponentScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mComponentScene->setOverlayColors(
      theme.getColor(Theme::Color::sSchematicOverlays).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicOverlays).getSecondaryColor());
  mComponentScene->setSelectionRectColors(
      theme.getColor(Theme::Color::sSchematicSelection).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicSelection).getSecondaryColor());
  mComponentScene->setGridStyle(Theme::GridStyle::Lines);

  // Setup package scene.
  mPackageScene->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mPackageScene->setOverlayColors(
      theme.getColor(Theme::Color::sBoardOverlays).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardOverlays).getSecondaryColor());
  mPackageScene->setSelectionRectColors(
      theme.getColor(Theme::Color::sBoardSelection).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardSelection).getSecondaryColor());
  mPackageScene->setGridStyle(Theme::GridStyle::Lines);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &DeviceTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &DeviceTab::refreshMetadata);

  // Connect models.
  mPinout->setList(&mDevice->getPadSignalMap());
  mPinout->setUndoStack(mUndoStack.get());
  mParts->setList(&mDevice->getParts());
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &DeviceTab::commitMetadata, Qt::QueuedConnection);

  // Refresh content.
  refreshMetadata();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (mIsNewElement) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }
}

DeviceTab::~DeviceTab() noexcept {
  deactivate();

  mPinout->setList(nullptr);
  mPinout->setSignals(nullptr);
  mPinout->setPads(nullptr);
  mPinout->setUndoStack(nullptr);
  mParts->setList(nullptr);

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath DeviceTab::getDirectoryPath() const noexcept {
  return mDevice->getDirectory().getAbsPath();
}

ui::TabData DeviceTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::Device,  // Type
      q2s(*mDevice->getNames().getDefaultValue()),  // Title
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

ui::DeviceTabData DeviceTab::getDerivedUiData() const noexcept {
  // TODO: replace this by a map model.
  QStringList signalNames;
  if (mComponent) {
    for (const auto& sig : mComponent->getSignals()) {
      signalNames.append(*sig.getName());
    }
  }

  return ui::DeviceTabData{
      mEditor.getUiIndex(),  // Library index
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      q2s(mDevice->getDirectory().getAbsPath().toStr()),  // Path
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
      nullptr,  // Attributes
      mComponent ? q2s(*mComponent->getNames().getDefaultValue())
                 : slint::SharedString(),  // Component name
      mComponent ? q2s(mComponent->getDescriptions().getDefaultValue())
                 : slint::SharedString(),  // Component description
      mPackage ? q2s(*mPackage->getNames().getDefaultValue())
               : slint::SharedString(),  // Package name
      mPackage ? q2s(mPackage->getDescriptions().getDefaultValue())
               : slint::SharedString(),  // Package description
      q2s(signalNames),  // Component signal names
      mPinout,  // Pinout
      mParts,  // Parts
      ui::RuleCheckData{
          ui::RuleCheckType::DeviceCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Check execution error
          !isWritable(),  // Check read-only
      },
      isInterfaceBroken(),  // Interface broken
      // mCompactLayout,  // Compact layout
      mAddCategoryRequested ? "choose" : slint::SharedString(),  // New category
  };
}

void DeviceTab::setDerivedUiData(const ui::DeviceTabData& data) noexcept {
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

  onDerivedUiDataChanged.notify();
}

void DeviceTab::trigger(ui::TabAction a) noexcept {
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

slint::Image DeviceTab::renderScene(float width, float height,
                                    int scene) noexcept {
  if ((scene == 0) && mComponentScene) {
    SlintGraphicsView view(SlintGraphicsView::defaultSymbolSceneRect());
    view.setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
    return view.render(*mComponentScene, width, height);
  } else if ((scene == 1) && (mPackageScene)) {
    SlintGraphicsView view(SlintGraphicsView::defaultFootprintSceneRect());
    view.setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
    return view.render(*mPackageScene, width, height);
  } else {
    return slint::Image();
  }
}

bool DeviceTab::requestClose() noexcept {
  commitMetadata();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The device '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mDevice->getNames().getDefaultValue()),
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
    DeviceTab::runChecksImpl() {
  // Do not run checks during wizard mode as it would be too early.
  if (mWizardMode) {
    return std::nullopt;
  }

  return std::make_pair(mDevice->runChecks(), mDevice->getMessageApprovals());
}

bool DeviceTab::autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                            bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingCategories>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool DeviceTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

void DeviceTab::messageApprovalChanged(const SExpression& approval,
                                       bool approved) noexcept {
  if (mDevice->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void DeviceTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
void DeviceTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitMetadata();
}

template <>
void DeviceTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void DeviceTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mAddCategoryRequested = true;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool DeviceTab::isWritable() const noexcept {
  return mIsNewElement || mDevice->getDirectory().isWritable();
}

bool DeviceTab::isInterfaceBroken() const noexcept {
  if ((mIsNewElement) || (mWizardMode)) return false;

  if (mDevice->getComponentUuid() != mOriginalComponentUuid) {
    return true;
  }
  if (mDevice->getPackageUuid() != mOriginalPackageUuid) {
    return true;
  }
  if (mDevice->getPadSignalMap() != mOriginalPadSignalMap) {
    return true;
  }
  return false;
}

void DeviceTab::refreshMetadata() noexcept {
  mName = q2s(*mDevice->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mDevice->getNames().getDefaultValue();
  mDescription = q2s(mDevice->getDescriptions().getDefaultValue());
  mKeywords = q2s(mDevice->getKeywords().getDefaultValue());
  mAuthor = q2s(mDevice->getAuthor());
  mVersion = q2s(mDevice->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mDevice->getVersion();
  mDeprecated = mDevice->isDeprecated();
  mCategories->setCategories(mDevice->getCategories());

  if (auto dbRes = mDevice->getResources().value(0)) {
    mDatasheetUrl = q2s(dbRes->getUrl().toString());
  } else {
    mDatasheetUrl = slint::SharedString();
  }
  mDatasheetUrlError = slint::SharedString();

  const WorkspaceLibraryDb& db = mApp.getWorkspace().getLibraryDb();
  const QStringList& localeOrder =
      mApp.getWorkspace().getSettings().libraryLocaleOrder.get();
  if ((!mComponent) || (mComponent->getUuid() != mDevice->getComponentUuid())) {
    mPinout->setSignals(nullptr);
    mSymbolGraphicsItems.clear();
    mSymbols.clear();
    mComponent.reset();

    try {
      const FilePath fp = db.getLatest<Component>(mDevice->getComponentUuid());
      mComponent = Component::open(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(TransactionalFileSystem::openRO(fp))));
      mPinout->setSignals(&mComponent->getSignals());
      if (const auto& variant = mComponent->getSymbolVariants().value(0)) {
        for (const auto& gate : variant->getSymbolItems().values()) {
          const FilePath fp = db.getLatest<Symbol>(gate->getSymbolUuid());
          std::shared_ptr<Symbol> symbol(
              Symbol::open(std::unique_ptr<TransactionalDirectory>(
                               new TransactionalDirectory(
                                   TransactionalFileSystem::openRO(fp))))
                  .release());
          mSymbols.append(symbol);
          auto graphicsItem = std::make_shared<SymbolGraphicsItem>(
              *symbol, mApp.getPreviewLayers(), mComponent.get(), gate);
          graphicsItem->setPosition(gate->getSymbolPosition());
          graphicsItem->setRotation(gate->getSymbolRotation());
          mComponentScene->addItem(*graphicsItem);
          mSymbolGraphicsItems.append(graphicsItem);
        }
      }
    } catch (const Exception& e) {
      // TODO
    }
  }
  if ((!mPackage) || (mPackage->getUuid() != mDevice->getPackageUuid())) {
    mPinout->setPads(nullptr);
    mPackageGraphicsItem.reset();
    mPackage.reset();

    try {
      const FilePath fp = db.getLatest<Package>(mDevice->getPackageUuid());
      mPackage = Package::open(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(TransactionalFileSystem::openRO(fp))));
      mPinout->setPads(&mPackage->getPads());
      if (auto footprint = mPackage->getFootprints().value(0)) {
        mPackageGraphicsItem.reset(new FootprintGraphicsItem(
            footprint, mApp.getPreviewLayers(),
            Application::getDefaultStrokeFont(), &mPackage->getPads(),
            mComponent.get(), localeOrder));
        mPackageScene->addItem(*mPackageGraphicsItem);
      }
    } catch (const Exception& e) {
      // TODO
    }
  }

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void DeviceTab::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdDeviceEdit> cmd(new CmdDeviceEdit(*mDevice));
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mDevice->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());

    try {
      ResourceList resources = mDevice->getResources();
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
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool DeviceTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack).
    mDevice->setMessageApprovals(mDevice->getMessageApprovals() -
                                 mDisappearedApprovals);

    mDevice->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Component>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mDevice->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mDevice->saveTo(dir);
    }
    mDevice->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mManualModificationsMade = false;

    mOriginalComponentUuid = mDevice->getComponentUuid();
    mOriginalPackageUuid = mDevice->getPackageUuid();
    mOriginalPadSignalMap = mDevice->getPadSignalMap();

    if (mWizardMode && (mCurrentPageIndex < 2)) {
      ++mCurrentPageIndex;
      if (mCurrentPageIndex >= 2) {
        mWizardMode = false;
        scheduleChecks();
      }
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

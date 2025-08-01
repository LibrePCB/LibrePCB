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
#include "../../modelview/attributelistmodel.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmddeviceedit.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"
#include "../cmp/componentchooserdialog.h"
#include "../cmp/componentsignalnamelistmodel.h"
#include "../libraryeditor.h"
#include "../libraryelementcache.h"
#include "../libraryelementcategoriesmodel.h"
#include "../pkg/footprintgraphicsitem.h"
#include "../pkg/packagechooserdialog.h"
#include "../sym/symbolgraphicsitem.h"
#include "devicepinoutbuilder.h"
#include "devicepinoutlistmodel.h"
#include "partlistmodel.h"

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
                     Mode mode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mDevice(std::move(dev)),
    mIsNewElement(isPathOutsideLibDir()),
    mPinoutBuilder(
        new DevicePinoutBuilder(mDevice->getPadSignalMap(), *mUndoStack)),
    mSignalNames(new ComponentSignalNameListModel()),
    mComponentScene(new GraphicsScene()),
    mPackageScene(new GraphicsScene()),
    mWizardMode(mode != Mode::Open),
    mCurrentPageIndex(mWizardMode ? 0 : 1),
    mComponentSelected(true),
    mPackageSelected(true),
    mChooseCategory(false),
    mNameParsed(mDevice->getNames().getDefaultValue()),
    mVersionParsed(mDevice->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                          editor.getWorkspace().getSettings(),
                                          CategoryTreeModel::Filter::CmpCat)),
    mAttributes(new AttributeListModel()),
    mPinout(new DevicePinoutListModel()),
    mPinoutSorted(new slint::SortModel<ui::DevicePinoutData>(
        mPinout,
        [this](const ui::DevicePinoutData& a, const ui::DevicePinoutData& b) {
          return mCollator(a.pad_name.data(), b.pad_name.data());
        })),
    mParts(new PartListModel()),
    mIsInterfaceBroken(false),
    mOriginalComponentUuid(mDevice->getComponentUuid()),
    mOriginalPackageUuid(mDevice->getPackageUuid()),
    mOriginalPadSignalMap(mDevice->getPadSignalMap()) {
  mCollator.setNumericMode(true);
  mCollator.setCaseSensitivity(Qt::CaseInsensitive);
  mCollator.setIgnorePunctuation(false);

  // Invalidate referenced elements if this is new.
  if (mIsNewElement) {
    mComponentSelected = false;
    mPackageSelected = false;
  }

  // Setup component scene.
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  mComponentScene->setOriginCrossVisible(false);  // It's rather disruptive.
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
  mPackageScene->setOriginCrossVisible(false);  // It's rather disruptive.
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

  // Setup default manufacturer.
  mParts->setDefaultManufacturer(mEditor.getLibrary().getManufacturer());
  connect(&mEditor.getLibrary(), &Library::manufacturerChanged, mParts.get(),
          &PartListModel::setDefaultManufacturer);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &DeviceTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &DeviceTab::refreshUiData);

  // Connect models.
  mAttributes->setReferences(&mDevice->getAttributes(), mUndoStack.get());
  mPinout->setReferences(&mDevice->getPadSignalMap(),
                         mPackage ? &mPackage->getPads() : nullptr,
                         mSignalNames, mUndoStack.get());
  mParts->setReferences(&mDevice->getParts(), mUndoStack.get());
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &DeviceTab::commitUiData, Qt::QueuedConnection);

  // If a dependent library element failed to load, try again after changes
  // in the workspace libraries.
  connect(&mApp.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanSucceeded, this,
          &DeviceTab::refreshDependentElements, Qt::QueuedConnection);

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

DeviceTab::~DeviceTab() noexcept {
  deactivate();

  mParts->setReferences(nullptr, nullptr);
  mPinout->setReferences(nullptr, nullptr, nullptr, nullptr);
  mSignalNames->setReferences(nullptr, nullptr);
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
  const QString cmpName =
      mComponent ? *mComponent->getNames().getDefaultValue() : QString();
  const QString pkgName =
      mPackage ? *mPackage->getNames().getDefaultValue() : QString();

  // On the first page in wizard mode, show full descriptions for clarity.
  // Later, remove linebreaks to have more space for the parts table in the UI.
  const QString cmpDescription = (mWizardMode && (mCurrentPageIndex == 0))
      ? mComponentDescription
      : mComponentDescription.split("\n", Qt::SkipEmptyParts).join("; ");
  const QString pkgDescription = (mWizardMode && (mCurrentPageIndex == 0))
      ? mPackageDescription
      : mPackageDescription.split("\n", Qt::SkipEmptyParts).join("; ");

  // For performance reasons, we do not call several (rather expensive) getters
  // if the interactive pad assignment is active.
  const bool idle = (mPinoutBuilder->getCurrentPadNumber() <= 0);
  const bool hasUnconnectedPads =
      idle && mPinoutBuilder->hasUnconnectedPadsAndSignals();
  const bool hasAutoConnectablePads =
      idle && mPinoutBuilder->hasAutoConnectablePads();
  const bool areAllPadsUnconnected =
      idle && mPinoutBuilder->areAllPadsUnconnected();

  return ui::DeviceTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mDevice->getDirectory().getAbsPath().toStr()),  // Path
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
      mAttributes,  // Attributes
      mComponentSelected && (!mComponent),  // Component error
      q2s(cmpName),  // Component name
      q2s(cmpDescription),  // Component description
      mPackageSelected && (!mPackage),  // Package error
      q2s(pkgName),  // Package name
      q2s(pkgDescription),  // Package description
      mSignalNames,  // Signal names
      mPinoutSorted,  // Pinout
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
      mIsInterfaceBroken,  // Interface broken
      hasUnconnectedPads,  // Has unconnected pads
      hasAutoConnectablePads,  // Has auto-connectable pads
      areAllPadsUnconnected,  // All pads unconnected
      mPinoutBuilder->getCurrentPadNumber(),  // Interactive pad number
      q2s(mPinoutBuilder->getCurrentPadName()),  // Interactive pad name
      q2s(mPinoutBuilder->getSignalsFilter()),  // Interactive signals filter
      mPinoutBuilder->getFilteredSignals(),  // Interactive signals
      mPinoutBuilder->getCurrentSignalIndex(),  // Interactive signal index
      slint::SharedString(),  // New category
  };
}

void DeviceTab::setDerivedUiData(const ui::DeviceTabData& data) noexcept {
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

  // Interactive pinout
  mPinoutBuilder->setCurrentSignalIndex(data.interactive_pinout_signal_index);
  mPinoutBuilder->setSignalsFilter(s2q(data.interactive_pinout_filter));

  onDerivedUiDataChanged.notify();
}

void DeviceTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Abort: {
      mPinoutBuilder->exitInteractiveMode();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Accept: {
      mPinoutBuilder->commitInteractiveMode();
      onDerivedUiDataChanged.notify();
      break;
    }
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
        // Initialize device metadata from selected component & package.
        if (mComponent && mPackage) {
          std::optional<ElementName> name = parseElementName(
              QString("%1 (%2)").arg(*mComponent->getNames().getDefaultValue(),
                                     *mPackage->getNames().getDefaultValue()));
          if (!name) {
            name = mComponent->getNames().getDefaultValue();
          }

          try {
            std::unique_ptr<CmdDeviceEdit> cmd(new CmdDeviceEdit(*mDevice));
            cmd->setNames(LocalizedNameMap(*name));
            cmd->setDescriptions(mComponent->getDescriptions());
            cmd->setKeywords(mComponent->getKeywords());
            cmd->setCategories(mComponent->getCategories());
            cmd->setResources(mComponent->getResources());
            mUndoStack->execCmd(cmd.release());
          } catch (const Exception& e) {
            qCritical() << e.getMsg();
          }
          refreshUiData();
        }
      } else if (mWizardMode && (mCurrentPageIndex >= 1)) {
        ++mCurrentPageIndex;
        // If there are no pads or signals to assign, skip the pinout page.
        if ((mCurrentPageIndex == 2) &&
            (!mPinoutBuilder->hasUnconnectedPadsAndSignals())) {
          ++mCurrentPageIndex;
        }
        // If the package is nothing to assemble, skip the parts page.
        if ((mCurrentPageIndex == 3) && mPackage &&
            (mPackage->getAssemblyType(true) == Package::AssemblyType::None)) {
          ++mCurrentPageIndex;
        }
        if (mCurrentPageIndex >= 4) {
          mWizardMode = false;
          mCurrentPageIndex = 1;
          scheduleChecks();
        }
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
      if (auto dbRes = mDevice->getResources().value(0)) {
        DesktopServices::downloadAndOpenResourceAsync(
            mApp.getWorkspace().getSettings(), *dbRes->getName(),
            dbRes->getMediaType(), dbRes->getUrl(), qApp->activeWindow());
      }
      break;
    }
    case ui::TabAction::DeviceSelectComponent: {
      selectComponent();
      break;
    }
    case ui::TabAction::DeviceSelectPackage: {
      selectPackage();
      break;
    }
    case ui::TabAction::DevicePinoutReset: {
      mPinoutBuilder->resetAll();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutConnectAuto: {
      mPinoutBuilder->autoConnect();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutConnectInteractively: {
      mPinoutBuilder->startInteractiveMode();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutLoadFromFile: {
      mPinoutBuilder->loadFromFile();
      onDerivedUiDataChanged.notify();
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
  commitUiData();

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
  mCurrentPageIndex = 0;
  mNameParsed = msg.getFixedName();
  commitUiData();
}

template <>
void DeviceTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
}

template <>
void DeviceTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mChooseCategory = true;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool DeviceTab::isWritable() const noexcept {
  return mIsNewElement || mDevice->getDirectory().isWritable();
}

static QString cleanDescription(const LocalizedDescriptionMap& descs) noexcept {
  return descs.getDefaultValue().split("\nGenerated with").first().trimmed();
}

void DeviceTab::refreshUiData() noexcept {
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

  // Update "interface broken" only when no command is active since it would
  // be annoying to get it during intermediate states.
  if (!mUndoStack->isCommandGroupActive()) {
    mIsInterfaceBroken = false;
    if ((!mIsNewElement) && (!mWizardMode)) {
      if (mDevice->getComponentUuid() != mOriginalComponentUuid) {
        mIsInterfaceBroken = true;
      }
      if (mDevice->getPackageUuid() != mOriginalPackageUuid) {
        mIsInterfaceBroken = true;
      }
      if (mDevice->getPadSignalMap() != mOriginalPadSignalMap) {
        mIsInterfaceBroken = true;
      }
    }
  }

  // This also calls both UI data changed callbacks.
  refreshDependentElements();
}

void DeviceTab::refreshDependentElements() noexcept {
  if (mComponentSelected &&
      ((!mComponent) ||
       (mComponent->getUuid() != mDevice->getComponentUuid()))) {
    mSignalNames->setReferences(nullptr, nullptr);
    mSymbolGraphicsItems.clear();
    mSymbols.clear();
    mComponent.reset();

    try {
      mComponent = mApp.getLibraryElementCache().getComponent(
          mDevice->getComponentUuid(), true);  // can throw
      mSignalNames->setReferences(
          const_cast<ComponentSignalList*>(&mComponent->getSignals()),
          mUndoStack.get());
      if (const auto& variant = mComponent->getSymbolVariants().value(0)) {
        for (int i = 0; i < variant->getSymbolItems().count(); ++i) {
          const auto gate = variant->getSymbolItems().at(i);
          std::shared_ptr<const Symbol> symbol =
              mApp.getLibraryElementCache().getSymbol(gate->getSymbolUuid(),
                                                      true);  // can throw
          mSymbols.append(symbol);
          auto graphicsItem = std::make_shared<SymbolGraphicsItem>(
              const_cast<Symbol&>(*symbol), mApp.getPreviewLayers(),
              mComponent.get(), gate);
          graphicsItem->setPosition(gate->getSymbolPosition());
          graphicsItem->setRotation(gate->getSymbolRotation());
          mComponentScene->addItem(*graphicsItem);
          mSymbolGraphicsItems.append(graphicsItem);
        }
      }
      mComponentDescription = cleanDescription(mComponent->getDescriptions());
    } catch (const Exception& e) {
      mComponentDescription = e.getMsg();
    }

    mPinoutBuilder->setSignals(mComponent ? mComponent->getSignals()
                                          : ComponentSignalList());
  }

  if (mPackageSelected &&
      ((!mPackage) || (mPackage->getUuid() != mDevice->getPackageUuid()))) {
    mPinout->setReferences(&mDevice->getPadSignalMap(), nullptr, mSignalNames,
                           mUndoStack.get());
    mFootprintGraphicsItem.reset();
    mPackage.reset();

    try {
      mPackage = mApp.getLibraryElementCache().getPackage(
          mDevice->getPackageUuid(), true);  // can throw
      mPinout->setReferences(&mDevice->getPadSignalMap(), &mPackage->getPads(),
                             mSignalNames, mUndoStack.get());
      if (auto footprint = mPackage->getFootprints().value(0)) {
        mFootprintGraphicsItem.reset(new FootprintGraphicsItem(
            std::const_pointer_cast<Footprint>(footprint),
            mApp.getPreviewLayers(), Application::getDefaultStrokeFont(),
            &mPackage->getPads(), mComponent.get(),
            mApp.getWorkspace().getSettings().libraryLocaleOrder.get()));
        mPackageScene->addItem(*mFootprintGraphicsItem);
      }
      mPackageDescription = cleanDescription(mPackage->getDescriptions());
    } catch (const Exception& e) {
      mPackageDescription = e.getMsg();
    }

    mPinoutBuilder->setPads(mPackage ? mPackage->getPads() : PackagePadList());
  }

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void DeviceTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdDeviceEdit> cmd(new CmdDeviceEdit(*mDevice));
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

    mAttributes->apply();
    mParts->apply();
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool DeviceTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
    mDevice->setMessageApprovals(mDevice->getMessageApprovals() -
                                 mDisappearedApprovals);

    mDevice->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Device>();
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
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    refreshUiData();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshUiData();
    return false;
  }
}

void DeviceTab::selectComponent() noexcept {
  ComponentChooserDialog dialog(mApp.getWorkspace(), &mApp.getPreviewLayers(),
                                qApp->activeWindow());
  if (dialog.exec() != QDialog::Accepted) return;

  std::optional<Uuid> cmpUuid = dialog.getSelectedComponentUuid();
  if (cmpUuid && (*cmpUuid != mDevice->getComponentUuid())) {
    try {
      // Load component.
      std::shared_ptr<const Component> cmp =
          mApp.getLibraryElementCache().getComponent(*cmpUuid,
                                                     true);  // can throw
      mComponentSelected = true;

      // Edit device (clear the pinout).
      std::unique_ptr<UndoCommandGroup> cmdGroup(
          new UndoCommandGroup(tr("Change Component")));
      std::unique_ptr<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
      cmdDevEdit->setComponentUuid(*cmpUuid);
      cmdGroup->appendChild(cmdDevEdit.release());
      for (auto item : mDevice->getPadSignalMap().values()) {
        std::optional<Uuid> signalUuid = item->getSignalUuid();
        if (!signalUuid || !cmp->getSignals().contains(*signalUuid)) {
          std::unique_ptr<CmdDevicePadSignalMapItemEdit> cmdItem(
              new CmdDevicePadSignalMapItemEdit(item));
          cmdItem->setSignalUuid(std::nullopt);
          cmdGroup->appendChild(cmdItem.release());
        }
      }
      mUndoStack->execCmd(cmdGroup.release());
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }
}

void DeviceTab::selectPackage() noexcept {
  PackageChooserDialog dialog(mApp.getWorkspace(), &mApp.getPreviewLayers(),
                              qApp->activeWindow());
  if (dialog.exec() != QDialog::Accepted) return;

  std::optional<Uuid> pkgUuid = dialog.getSelectedPackageUuid();
  if (pkgUuid && (*pkgUuid != mDevice->getPackageUuid())) {
    try {
      // Load package.
      std::shared_ptr<const Package> pkg =
          mApp.getLibraryElementCache().getPackage(*pkgUuid,
                                                   true);  // can throw
      const QSet<Uuid> pads = pkg->getPads().getUuidSet();
      mPackageSelected = true;

      // Edit device (re-create empty pinout).
      std::unique_ptr<UndoCommandGroup> cmdGroup(
          new UndoCommandGroup(tr("Change Package")));
      std::unique_ptr<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
      cmdDevEdit->setPackageUuid(*pkgUuid);
      cmdGroup->appendChild(cmdDevEdit.release());
      for (const DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
        if (!pads.contains(item.getPadUuid())) {
          cmdGroup->appendChild(new CmdDevicePadSignalMapItemRemove(
              mDevice->getPadSignalMap(), &item));
        }
      }
      foreach (const Uuid& pad,
               pads - mDevice->getPadSignalMap().getUuidSet()) {
        cmdGroup->appendChild(new CmdDevicePadSignalMapItemInsert(
            mDevice->getPadSignalMap(),
            std::make_shared<DevicePadSignalMapItem>(pad, std::nullopt)));
      }
      mUndoStack->execCmd(cmdGroup.release());
      Q_ASSERT(mDevice->getPadSignalMap().getUuidSet() == pads);
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

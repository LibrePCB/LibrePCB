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
#include "../../mainwindow.h"
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
#include "../pkg/footprintpadgraphicsitem.h"
#include "../pkg/packagechooserdialog.h"
#include "../sym/symbolgraphicsitem.h"
#include "../sym/symbolpingraphicsitem.h"
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
#include <librepcb/core/workspace/colorrole.h>
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
    mMode(mode),
    mIsNewElement(isPathOutsideLibDir()),
    mPinoutBuilder(
        new DevicePinoutBuilder(mDevice->getPadSignalMap(), *mUndoStack)),
    mSignalNames(new ComponentSignalNameListModel()),
    mComponentView(
        new SlintGraphicsView(SlintGraphicsView::defaultSymbolSceneRect(),
                              SlintGraphicsView::defaultMargins())),
    mPackageView(
        new SlintGraphicsView(SlintGraphicsView::defaultFootprintSceneRect(),
                              SlintGraphicsView::defaultMargins())),
    mComponentScene(new GraphicsScene()),
    mPackageScene(new GraphicsScene()),
    mWizardMode(mode != Mode::Open),
    mCurrentPageIndex(mWizardMode ? 0 : 1),
    mComponentSelected(true),
    mPackageSelected(true),
    mChooseCategory(false),
    mFrameIndex(0),
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
  if (mode == Mode::New) {
    mComponentSelected = false;
    mPackageSelected = false;
  }

  // Setup component view + scene.
  {
    mComponentScene->setGridStyle(GridStyle::Lines);
    mComponentScene->setOriginCrossVisible(false);  // It's rather disruptive.
    connect(mComponentScene.get(), &GraphicsScene::changed, this,
            &DeviceTab::requestRepaint);
    connect(mComponentView.get(), &SlintGraphicsView::transformChanged, this,
            &DeviceTab::requestRepaint);
    connect(mComponentView.get(), &SlintGraphicsView::stateChanged, this,
            [this]() { onDerivedUiDataChanged.notify(); });
  }

  // Setup package view + scene.
  {
    mPackageScene->setGridStyle(GridStyle::Lines);
    mPackageScene->setOriginCrossVisible(false);  // It's rather disruptive.
    connect(mPackageScene.get(), &GraphicsScene::changed, this,
            &DeviceTab::requestRepaint);
    connect(mPackageView.get(), &SlintGraphicsView::transformChanged, this,
            &DeviceTab::requestRepaint);
    connect(mPackageView.get(), &SlintGraphicsView::stateChanged, this,
            [this]() { onDerivedUiDataChanged.notify(); });
  }

  // Apply workspace settings whenever they have been modified.
  connect(&mApp.getWorkspace().getSettings().useOpenGl,
          &WorkspaceSettingsItem::edited, this,
          &DeviceTab::applyWorkspaceSettings);
  connect(&mApp.getWorkspace().getSettings().schematicColorSchemes,
          &WorkspaceSettingsItem_ColorSchemes::colorsModified, this,
          &DeviceTab::applyWorkspaceSettings);
  connect(&mApp.getWorkspace().getSettings().boardColorSchemes,
          &WorkspaceSettingsItem_ColorSchemes::colorsModified, this,
          &DeviceTab::applyWorkspaceSettings);
  applyWorkspaceSettings();

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
  const ColorScheme& sch =
      mApp.getWorkspace().getSettings().schematicColorSchemes.getActive();
  const auto schBgColors = sch.getColors(ColorRole::schematicBackground());
  const ColorScheme& brd =
      mApp.getWorkspace().getSettings().boardColorSchemes.getActive();
  const auto brdBgColors = brd.getColors(ColorRole::boardBackground());

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
      q2s(schBgColors.primary),  // Component background color
      q2s(schBgColors.secondary),  // Component foreground color
      q2s(brdBgColors.primary),  // Package background color
      q2s(brdBgColors.secondary),  // Package foreground color
      mIsInterfaceBroken,  // Interface broken
      hasUnconnectedPads,  // Has unconnected pads
      hasAutoConnectablePads,  // Has auto-connectable pads
      areAllPadsUnconnected,  // All pads unconnected
      mPinoutBuilder->getCurrentPadNumber(),  // Interactive pad number
      q2s(mPinoutBuilder->getCurrentPadName()),  // Interactive pad name
      q2s(mPinoutBuilder->getSignalsFilter()),  // Interactive signals filter
      mPinoutBuilder->getFilteredSignals(),  // Interactive signals
      mPinoutBuilder->getCurrentSignalIndex(),  // Interactive signal index
      mFrameIndex,  // Frame index
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
  updateHighlightedPadsAndPins();

  onDerivedUiDataChanged.notify();
}

void DeviceTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Abort: {
      mPinoutBuilder->exitInteractiveMode();
      updateHighlightedPadsAndPins();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Accept: {
      mPinoutBuilder->commitInteractiveMode();
      updateHighlightedPadsAndPins();
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
        if ((mMode == Mode::New) && mComponent && mPackage) {
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
        QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitUiData();
        mUndoStack->redo();
      } catch (const Exception& e) {
        QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Close: {
      if (requestClose()) {
        WindowTab::trigger(a);
      }
      break;
    }
    case ui::TabAction::ZoomFit: {
      // Currently we can't distinguish in which view the action was
      // triggered, thus applying to both views.
      mComponentView->zoomToSceneRect(mComponentScene->itemsBoundingRect());
      mPackageView->zoomToSceneRect(mPackageScene->itemsBoundingRect());
      break;
    }
    case ui::TabAction::OpenDatasheet: {
      commitUiData();
      if (auto dbRes = mDevice->getResources().value(0)) {
        DesktopServices::downloadAndOpenResourceAsync(
            mApp.getWorkspace().getSettings(), *dbRes->getName(),
            dbRes->getMediaType(), dbRes->getUrl(), getWindow());
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
    case ui::TabAction::DeviceOpenComponent: {
      if (mWindow && mComponent) {
        mWindow->requestComponentTab(mComponent->getDirectory().getAbsPath());
      }
      break;
    }
    case ui::TabAction::DeviceOpenPackage: {
      if (mWindow && mPackage) {
        mWindow->requestPackageTab(mPackage->getDirectory().getAbsPath());
      }
      break;
    }
    case ui::TabAction::DevicePinoutReset: {
      mPinoutBuilder->resetAll();
      updateHighlightedPadsAndPins();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutConnectAuto: {
      mPinoutBuilder->autoConnect();
      updateHighlightedPadsAndPins();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutConnectInteractively: {
      mPinoutBuilder->startInteractiveMode();
      updateHighlightedPadsAndPins();
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::DevicePinoutLoadFromFile: {
      mPinoutBuilder->loadFromFile();
      updateHighlightedPadsAndPins();
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
  if (scene == 0) {
    return mComponentView->render(*mComponentScene, width, height);
  } else if (scene == 1) {
    return mPackageView->render(*mPackageScene, width, height);
  } else {
    return slint::Image();
  }
}

bool DeviceTab::processScenePointerEvent(const QPointF& pos,
                                         slint::private_api::PointerEvent e,
                                         int scene) noexcept {
  if (scene == 0) {
    return mComponentView->pointerEvent(pos, e);
  } else if (scene == 1) {
    return mPackageView->pointerEvent(pos, e);
  } else {
    return false;
  }
}

bool DeviceTab::processSceneScrolled(const QPointF& pos,
                                     slint::private_api::PointerScrollEvent e,
                                     int scene) noexcept {
  if (scene == 0) {
    return mComponentView->scrollEvent(pos, e);
  } else if (scene == 1) {
    return mPackageView->scrollEvent(pos, e);
  } else {
    return false;
  }
}

bool DeviceTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      getWindow(), tr("Save Changes?"),
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
      if (checkOnly) {
        return true;
      } else {
        return autoFix(*m);  // can throw
      }
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
bool DeviceTab::autoFix(const MsgNameNotTitleCase& msg) {
  mCurrentPageIndex = 0;
  mNameParsed = msg.getFixedName();
  commitUiData();
  return true;
}

template <>
bool DeviceTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
  return true;
}

template <>
bool DeviceTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mChooseCategory = true;
  onDerivedUiDataChanged.notify();
  return true;
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
    mSymbolVariant.reset();
    mComponent.reset();

    try {
      mComponent = mApp.getLibraryElementCache().getComponent(
          mDevice->getComponentUuid(), true);  // can throw
      mSymbolVariant = mComponent->getSymbolVariants().value(0);
      mSignalNames->setReferences(
          const_cast<ComponentSignalList*>(&mComponent->getSignals()),
          mUndoStack.get());
      if (mSymbolVariant) {
        for (int i = 0; i < mSymbolVariant->getSymbolItems().count(); ++i) {
          const auto gate = mSymbolVariant->getSymbolItems().at(i);
          std::shared_ptr<const Symbol> symbol =
              mApp.getLibraryElementCache().getSymbol(gate->getSymbolUuid(),
                                                      true);  // can throw
          mSymbols.append(symbol);
          auto graphicsItem = std::make_shared<SymbolGraphicsItem>(
              const_cast<Symbol&>(*symbol), mApp.getPreviewLayers(),
              mComponent.get(), gate,
              mApp.getWorkspace().getSettings().libraryLocaleOrder.get(), true);
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
    mFootprint.reset();
    mPackage.reset();

    try {
      mPackage = mApp.getLibraryElementCache().getPackage(
          mDevice->getPackageUuid(), true);  // can throw
      mFootprint = mPackage->getFootprints().value(0);
      mPinout->setReferences(&mDevice->getPadSignalMap(), &mPackage->getPads(),
                             mSignalNames, mUndoStack.get());
      if (mFootprint) {
        mFootprintGraphicsItem.reset(new FootprintGraphicsItem(
            std::const_pointer_cast<Footprint>(mFootprint),
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

  updatePreviewPinNumbers();
  updateHighlightedPadsAndPins();
  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void DeviceTab::commitUiData() noexcept {
  try {
    std::unique_ptr<CmdDeviceEdit> cmd(new CmdDeviceEdit(*mDevice));
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mDevice->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mDevice->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mDevice->getAuthor()) {
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
    QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
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
    QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
    refreshUiData();
    return false;
  }
}

void DeviceTab::selectComponent() noexcept {
  ComponentChooserDialog dialog(mApp.getWorkspace(), &mApp.getPreviewLayers(),
                                getWindow());
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
      QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
    }
  }
}

void DeviceTab::selectPackage() noexcept {
  PackageChooserDialog dialog(mApp.getWorkspace(), &mApp.getPreviewLayers(),
                              getWindow());
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
      QMessageBox::critical(getWindow(), tr("Error"), e.getMsg());
    }
  }
}

void DeviceTab::updatePreviewPinNumbers() noexcept {
  if ((!mComponent) || (!mSymbolVariant) ||
      (mSymbolGraphicsItems.count() !=
       mSymbolVariant->getSymbolItems().count()) ||
      (mSymbolGraphicsItems.count() != mSymbols.count())) {
    return;
  }

  QHash<Uuid, QStringList> signalNumbers;
  for (const auto& padMap : mDevice->getPadSignalMap()) {
    if (auto sigUuid = padMap.getSignalUuid()) {
      QString padName = "?";
      if (mPackage) {
        if (auto pad = mPackage->getPads().find(padMap.getPadUuid())) {
          padName = *pad->getName();
        }
      }
      signalNumbers[*sigUuid].append(padName);
    }
  }
  qDebug() << signalNumbers;

  for (int i = 0; i < mSymbolGraphicsItems.count(); ++i) {
    auto gate = mSymbolVariant->getSymbolItems().at(i);
    auto symbol = std::const_pointer_cast<Symbol>(mSymbols.at(i));
    auto graphicsItem = mSymbolGraphicsItems.at(i);
    for (auto pin : symbol->getPins().values()) {
      if (auto pinItem = graphicsItem->getGraphicsItem(pin)) {
        QString numbers;
        if (auto pinMap = gate->getPinSignalMap().find(pin->getUuid())) {
          if (auto sigUuid = pinMap->getSignalUuid()) {
            numbers = signalNumbers.value(*sigUuid).join(", ");
          }
        }
        if (numbers.isEmpty()) {
          numbers = "✖";
        }
        if (numbers.length() > 20) {
          numbers = numbers.left(19) + "…";
        }
        pinItem->setOverridePinNumber(numbers);
      }
    }
  }
}

void DeviceTab::updateHighlightedPadsAndPins() noexcept {
  setHighlightedPad(mPinoutBuilder->getCurrentPadUuid());
  setHighlightedSignalPins(mPinoutBuilder->getCurrentSignalUuid());
}

void DeviceTab::setHighlightedPad(const std::optional<Uuid>& pad) noexcept {
  if (!mFootprintGraphicsItem) return;
  mFootprintGraphicsItem->setSelectionRect(QRectF());
  if ((!pad) || (!mPackage) || (!mFootprint)) return;
  for (auto fptPad :
       std::const_pointer_cast<Footprint>(mFootprint)->getPads().values()) {
    if (fptPad->getPackagePadUuid() == pad) {
      if (auto item = mFootprintGraphicsItem->getGraphicsItem(fptPad)) {
        item->setSelected(true);
      }
    }
  }
}

void DeviceTab::setHighlightedSignalPins(
    const std::optional<Uuid>& signal) noexcept {
  if (mSymbolGraphicsItems.count() != mSymbols.count()) return;
  for (int i = 0; i < mSymbols.count(); ++i) {
    auto symbol = std::const_pointer_cast<Symbol>(mSymbols.at(i));
    auto graphicsItem = mSymbolGraphicsItems.at(i);
    graphicsItem->setSelectionRect(QRectF());
    if ((!signal) || (!symbol) || (!mComponent) || (!mSymbolVariant) ||
        (mSymbolVariant->getSymbolItems().count() != mSymbols.count())) {
      continue;
    }
    auto gate = mSymbolVariant->getSymbolItems().at(i);
    for (auto pin : symbol->getPins().values()) {
      if (auto pinMap = gate->getPinSignalMap().find(pin->getUuid())) {
        if (pinMap->getSignalUuid() == signal) {
          if (auto item = graphicsItem->getGraphicsItem(pin)) {
            item->setSelected(true);
          }
        }
      }
    }
  }
}

void DeviceTab::applyWorkspaceSettings() noexcept {
  {
    const ColorScheme& scheme =
        mApp.getWorkspace().getSettings().schematicColorSchemes.getActive();
    const auto background = scheme.getColors(ColorRole::schematicBackground());
    mComponentScene->setBackgroundColors(background.primary,
                                         background.secondary);
    const auto overlay = scheme.getColors(ColorRole::schematicOverlays());
    mComponentScene->setOverlayColors(overlay.primary, overlay.secondary);
    const auto selection = scheme.getColors(ColorRole::schematicSelection());
    mComponentScene->setSelectionRectColors(selection.primary,
                                            selection.secondary);
    mComponentView->setUseOpenGl(
        mApp.getWorkspace().getSettings().useOpenGl.get());
  }

  {
    const ColorScheme& scheme =
        mApp.getWorkspace().getSettings().boardColorSchemes.getActive();
    const auto background = scheme.getColors(ColorRole::boardBackground());
    mPackageScene->setBackgroundColors(background.primary,
                                       background.secondary);
    const auto overlay = scheme.getColors(ColorRole::boardOverlays());
    mPackageScene->setOverlayColors(overlay.primary, overlay.secondary);
    const auto selection = scheme.getColors(ColorRole::boardSelection());
    mPackageScene->setSelectionRectColors(selection.primary,
                                          selection.secondary);
    mPackageView->setUseOpenGl(
        mApp.getWorkspace().getSettings().useOpenGl.get());
  }

  onDerivedUiDataChanged.notify();
}

void DeviceTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

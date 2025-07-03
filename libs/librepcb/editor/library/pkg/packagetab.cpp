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
#include "packagetab.h"

#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../workspace/categorytreemodel2.h"
#include "../cmd/cmdlibraryelementedit.h"
#include "../libraryeditor2.h"
#include "../libraryelementcategoriesmodel.h"
#include "footprintgraphicsitem.h"
#include "graphics/graphicslayerlist.h"
#include "packagepadlistmodel2.h"
#include "utils/slinthelpers.h"
#include "utils/uihelpers.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/pkg/packagecheckmessages.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageTab::PackageTab(LibraryEditor2& editor, std::unique_ptr<Package> pkg,
                       bool wizardMode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mPackage(std::move(pkg)),
    mLayers(GraphicsLayerList::libraryLayers(
        &mEditor.getWorkspace().getSettings())),
    mView(new SlintGraphicsView(this)),
    mIsNewElement(isPathOutsideLibDir()),
    mWizardMode(wizardMode),
    mCurrentPageIndex(wizardMode ? 0 : 1),
    mGridStyle(mApp.getWorkspace()
                   .getSettings()
                   .themes.getActive()
                   .getBoardGridStyle()),
    mGridInterval(2540000),
    mUnit(LengthUnit::millimeters()),
    // mCompactLayout(false),
    mFrameIndex(0),
    mNameParsed(mPackage->getNames().getDefaultValue()),
    mVersionParsed(mPackage->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::PackageCategory)),
    mCategoriesTree(new CategoryTreeModel2(editor.getWorkspace().getLibraryDb(),
                                           editor.getWorkspace().getSettings(),
                                           CategoryTreeModel2::Filter::PkgCat)),
    mPads(new PackagePadListModel2()),
    // mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolLineWidth(mApp.getWorkspace().getSettings()),
    mToolSize(mApp.getWorkspace().getSettings()),
    mToolFilled(false),
    mToolGrabArea(false),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mOriginalPackagePadUuids(mPackage->getPads().getUuidSet()) {
  // Setup graphics view.
  mView->setEventHandler(this);
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &PackageTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          &PackageTab::notifyDerivedUiDataChanged);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &PackageTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &PackageTab::refreshMetadata);

  // Connect models.
  mPads->setList(&mPackage->getPads());
  mPads->setUndoStack(mUndoStack.get());
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &PackageTab::commitMetadata, Qt::QueuedConnection);

  // Setup messages.
  // connect(&mMsgImportPins, &DismissableMessageContext::visibilityChanged,
  // this,
  //        [this]() { onDerivedUiDataChanged.notify(); });

  // Load finite state machine (FSM).
  // SymbolEditorFsm::Context fsmContext{*mPackage, *mUndoStack, !isWritable(),
  //                                    mUnit, *this};
  // mFsm.reset(new SymbolEditorFsm(fsmContext));

  // Refresh content.
  refreshMetadata();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (mIsNewElement) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }
}

PackageTab::~PackageTab() noexcept {
  deactivate();

  // Clean up the state machine nicely to avoid unexpected behavior. Triggering
  // abort (Esc) two times is usually sufficient to leave any active tool, so
  // let's call it three times to be on the safe side. Unfortunately there's
  // no clean way to forcible and guaranteed leaving a tool.
  // mFsm->processAbortCommand();
  // mFsm->processAbortCommand();
  // mFsm->processAbortCommand();
  // mFsm.reset();

  mPads->setList(nullptr);
  mPads->setUndoStack(nullptr);

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();

  mView->setEventHandler(nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath PackageTab::getDirectoryPath() const noexcept {
  return mPackage->getDirectory().getAbsPath();
}

ui::TabData PackageTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());
  /*if ((!mWizardMode) && ((!mCompactLayout) || (mCurrentPageIndex == 1))) {
    features.grid = toFs(isWritable());
    features.zoom = toFs(true);
    features.import_graphics =
        toFs(mToolFeatures.testFlag(Feature::ImportGraphics));
    features.export_graphics = toFs(mTool == ui::EditorTool::Select);
    features.select = toFs(mToolFeatures.testFlag(Feature::Select));
    features.cut = toFs(mToolFeatures.testFlag(Feature::Cut));
    features.copy = toFs(mToolFeatures.testFlag(Feature::Copy));
    features.paste = toFs(mToolFeatures.testFlag(Feature::Paste));
    features.remove = toFs(mToolFeatures.testFlag(Feature::Remove));
    features.rotate = toFs(mToolFeatures.testFlag(Feature::Rotate));
    features.mirror = toFs(mToolFeatures.testFlag(Feature::Mirror));
    features.snap_to_grid = toFs(mToolFeatures.testFlag(Feature::SnapToGrid));
    features.edit_properties =
        toFs(mToolFeatures.testFlag(Feature::Properties));
  }*/

  return ui::TabData{
      ui::TabType::Package,  // Type
      q2s(*mPackage->getNames().getDefaultValue()),  // Title
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

ui::PackageTabData PackageTab::getDerivedUiData() const noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;

  return ui::PackageTabData{
      mEditor.getUiIndex(),  // Library index
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      q2s(mPackage->getDirectory().getAbsPath().toStr()),  // Path
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
      mPads,  // Package pads
      nullptr,  // Footprints
      ui::RuleCheckData{
          ui::RuleCheckType::SymbolCheck,  // Check type
          ui::RuleCheckState::UpToDate,  // Check state
          mCheckMessages,  // Check messages
          mCheckMessages->getUnapprovedCount(),  // Check unapproved count
          mCheckMessages->getErrorCount(),  // Check errors count
          mCheckError,  // Check execution error
          !isWritable(),  // Check read-only
      },
      q2s(bgColor),  // Background color
      q2s(fgColor),  // Foreground color
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      l2s(*mGridInterval),  // Grid interval
      l2s(mUnit),  // Unit
      isInterfaceBroken(),  // Interface broken
      // mMsgImportPins.getUiData(),  // Message "import pins"
      mTool,  // Tool
      q2s(mView->isPanning() ? Qt::ClosedHandCursor
                             : mToolCursorShape),  // Tool cursor
      q2s(mToolOverlayText),  // Tool overlay text
      ui::ComboBoxData{
          // Tool layer
          mToolLayers,  // Items
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current index
      },
      mToolLineWidth.getUiData(),  // Tool line width
      mToolSize.getUiData(),  // // Tool size
      ui::AngleEditData{
          // Tool angle
          l2s(mToolAngle),  // Angle
          false,  // Increase
          false,  // Decrease
      },
      mToolFilled,  // Tool filled
      mToolGrabArea,  // Tool grab area
      ui::LineEditData{
          // Tool value
          true,  // Enabled
          q2s(EditorToolbox::toSingleLine(mToolValue)),  // Text
          slint::SharedString(),  // Placeholder
          mToolValueSuggestions,  // Suggestions
      },
      l2s(mToolAlign.getH()),  // Tool horizontal alignment
      l2s(mToolAlign.getV()),  // Tool vertical alignment
      // mCompactLayout,  // Compact layout
      q2s(mSceneImagePos),  // Scene image position
      0,  // Frame index
      mAddCategoryRequested ? "choose" : slint::SharedString(),  // New category
  };
}

void PackageTab::setDerivedUiData(const ui::PackageTabData& data) noexcept {
  // General
  if (data.page_index != mCurrentPageIndex) {
    mCurrentPageIndex = data.page_index;
    onUiDataChanged.notify();
  }
  // if (data.compact_layout != mCompactLayout) {
  //   mCompactLayout = data.compact_layout;
  //   onUiDataChanged.notify();
  // }
  mSceneImagePos = s2q(data.scene_image_pos);

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

  // View
  mGridStyle = s2l(data.grid_style);
  if (auto interval = s2plength(data.grid_interval)) {
    setGridInterval(*interval);
  }
  if (mScene) {
    mScene->setGridStyle(mGridStyle);
    mScene->setGridInterval(mGridInterval);
  }
  const LengthUnit unit = s2l(data.unit);
  if (unit != mUnit) {
    mUnit = unit;
  }

  // Messages
  // mMsgImportPins.setUiData(data.import_pins_msg);

  // Tool
  /*if (const Layer* layer = mToolLayersQt.value(data.tool_layer.current_index))
  { emit layerRequested(*layer);
  }
  if (data.tool_angle.increase) {
    emit angleRequested(mToolAngle + Angle::deg45());
  } else if (data.tool_angle.decrease) {
    emit angleRequested(mToolAngle - Angle::deg45());
  } else {
    emit angleRequested(s2angle(data.tool_angle.value));
  }
  emit filledRequested(data.tool_filled);
  emit grabAreaRequested(data.tool_grab_area);
  mToolLineWidth.setUiData(data.tool_line_width);
  mToolSize.setUiData(data.tool_size);
  emit valueRequested(EditorToolbox::toMultiLine(s2q(data.tool_value.text)));
  emit hAlignRequested(s2l(data.tool_halign));
  emit vAlignRequested(s2l(data.tool_valign));*/

  requestRepaint();
}

void PackageTab::activate() noexcept {
  mScene.reset(new GraphicsScene(this));
  // mScene->setGridInterval(mBoard.getGridInterval());
  connect(mScene.get(), &GraphicsScene::changed, this,
          &PackageTab::requestRepaint);

  // mGraphicsItem.reset(new FootprintGraphicsItem(*mPackage, *mLayers));
  // mScene->addItem(*mGraphicsItem);

  applyTheme();
  requestRepaint();
}

void PackageTab::deactivate() noexcept {
  mGraphicsItem.reset();
  mScene.reset();
}

void PackageTab::trigger(ui::TabAction a) noexcept {
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
    case ui::TabAction::Print: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
      break;
    }
    case ui::TabAction::ExportImage: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
      //                          "image_export");
      break;
    }
    case ui::TabAction::ExportPdf: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf,
      // "pdf_export");
      break;
    }
    case ui::TabAction::SelectAll: {
      // mFsm->processSelectAll();
      break;
    }
    case ui::TabAction::Abort: {
      // mFsm->processAbortCommand();
      break;
    }
    case ui::TabAction::Cut: {
      // mFsm->processCut();
      break;
    }
    case ui::TabAction::Copy: {
      // mFsm->processCopy();
      break;
    }
    case ui::TabAction::Paste: {
      // mFsm->processPaste();
      break;
    }
    case ui::TabAction::Delete: {
      // mFsm->processRemove();
      break;
    }
    case ui::TabAction::RotateCcw: {
      // mFsm->processRotate(Angle::deg90());
      break;
    }
    case ui::TabAction::RotateCw: {
      // mFsm->processRotate(-Angle::deg90());
      break;
    }
    case ui::TabAction::MirrorHorizontally: {
      // mFsm->processMirror(Qt::Horizontal);
      break;
    }
    case ui::TabAction::MirrorVertically: {
      // mFsm->processMirror(Qt::Vertical);
      break;
    }
    /*case ui::TabAction::MoveLeft: {
      if (!mFsm->processMove(Point(-mBoard.getGridInterval(), 0))) {
        mView->scrollLeft();
      }
      break;
    }
    case ui::TabAction::MoveRight: {
      if (!mFsm->processMove(Point(*mBoard.getGridInterval(), 0))) {
        mView->scrollRight();
      }
      break;
    }
    case ui::TabAction::MoveUp: {
      if (!mFsm->processMove(Point(0, *mBoard.getGridInterval()))) {
        mView->scrollUp();
      }
      break;
    }
    case ui::TabAction::MoveDown: {
      if (!mFsm->processMove(Point(0, -mBoard.getGridInterval()))) {
        mView->scrollDown();
      }
      break;
    }*/
    case ui::TabAction::SnapToGrid: {
      // mFsm->processSnapToGrid();
      break;
    }
    case ui::TabAction::EditProperties: {
      // mFsm->processEditProperties();
      break;
    }
    /*case ui::TabAction::GridIntervalIncrease: {
      mBoard.setGridInterval(
          PositiveLength(mBoard.getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(mBoard.getGridInterval());
        requestRepaint();
      }
      break;
    }
    case ui::TabAction::GridIntervalDecrease: {
      if ((*mBoard.getGridInterval() % 2) == 0) {
        mBoard.setGridInterval(
            PositiveLength(mBoard.getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(mBoard.getGridInterval());
          requestRepaint();
        }
      }
      break;
    }*/
    case ui::TabAction::ZoomIn: {
      mView->zoomIn();
      break;
    }
    case ui::TabAction::ZoomOut: {
      mView->zoomOut();
      break;
    }
    case ui::TabAction::ZoomFit: {
      if (mScene) mView->zoomToSceneRect(mScene->itemsBoundingRect());
      break;
    }
    case ui::TabAction::ToolSelect: {
      // mFsm->processSelect();
      break;
    }
    case ui::TabAction::ToolPolygon: {
      // mFsm->processDrawPolygon();
      break;
    }
    case ui::TabAction::ToolText: {
      // mFsm->processAddText();
      break;
    }
    case ui::TabAction::ToolMeasure: {
      // mFsm->processMeasure();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image PackageTab::renderScene(float width, float height,
                                     int scene) noexcept {
  Q_UNUSED(scene);
  if (mScene) {
    return mView->render(*mScene, width, height);
  }
  return slint::Image();
}

bool PackageTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  return mView->pointerEvent(pos, e);
}

bool PackageTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView->scrollEvent(pos, e);
}

bool PackageTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  return mView->keyEvent(e);
}

bool PackageTab::requestClose() noexcept {
  commitMetadata();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The symbol '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mPackage->getNames().getDefaultValue()),
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
 *  IF_GraphicsViewEventHandler Methods
 ******************************************************************************/

bool PackageTab::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return false;  // mFsm->processKeyPressed(e);
}

bool PackageTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return false;  // mFsm->processKeyReleased(e);
}

bool PackageTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  // emit cursorCoordinatesChanged(e.scenePos, mBoard.getGridUnit());
  return false;  // mFsm->processGraphicsSceneMouseMoved(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return false;  // mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return false;  // mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return false;  // mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool PackageTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return false;  // mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
    PackageTab::runChecksImpl() {
  // Do not run checks during wizard mode as it would be too early.
  if (mWizardMode) {
    return std::nullopt;
  }

  // Do not run checks if a tool is active because it could lead to annoying,
  // flickering messages. For example when placing pins, they always overlap
  // right after placing them, so we have to wait until the user has moved the
  // cursor to place the pin at a different position.
  if (mTool != ui::EditorTool::Select) {
    return std::nullopt;
  }

  return std::make_pair(mPackage->runChecks(), mPackage->getMessageApprovals());
}

bool PackageTab::autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                             bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingCategories>(msg, checkOnly)) return true;
  // TODO
  return false;
}

template <typename MessageType>
bool PackageTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

void PackageTab::messageApprovalChanged(const SExpression& approval,
                                        bool approved) noexcept {
  if (mPackage->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void PackageTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
void PackageTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitMetadata();
}

template <>
void PackageTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void PackageTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mAddCategoryRequested = true;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageTab::isWritable() const noexcept {
  return mIsNewElement || mPackage->getDirectory().isWritable();
}

bool PackageTab::isInterfaceBroken() const noexcept {
  return (!mIsNewElement) && (!mWizardMode) &&
      (mPackage->getPads().getUuidSet() != mOriginalPackagePadUuids);
}

void PackageTab::refreshMetadata() noexcept {
  mName = q2s(*mPackage->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mPackage->getNames().getDefaultValue();
  mDescription = q2s(mPackage->getDescriptions().getDefaultValue());
  mKeywords = q2s(mPackage->getKeywords().getDefaultValue());
  mAuthor = q2s(mPackage->getAuthor());
  mVersion = q2s(mPackage->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mPackage->getVersion();
  mDeprecated = mPackage->isDeprecated();
  mCategories->setCategories(mPackage->getCategories());

  // mMsgImportPins.setActive(mPackage->isEmpty());

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void PackageTab::commitMetadata() noexcept {
  // Abort any active command as this would block the undo stack.
  // mFsm->processAbortCommand();
  // mFsm->processAbortCommand();
  // mFsm->processAbortCommand();

  try {
    std::unique_ptr<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mPackage, tr("Edit Symbol Properties")));
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mPackage->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool PackageTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack).
    mPackage->setMessageApprovals(mPackage->getMessageApprovals() -
                                  mDisappearedApprovals);

    mPackage->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Package>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mPackage->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mPackage->saveTo(dir);
    }
    mPackage->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mOriginalPackagePadUuids = mPackage->getPads().getUuidSet();
    mManualModificationsMade = false;

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

void PackageTab::setGridInterval(const PositiveLength& interval) noexcept {
  if (interval != mGridInterval) {
    mGridInterval = interval;
    // mFsm->processGridIntervalChanged(mGridInterval);
    if (mScene) {
      mScene->setGridInterval(mGridInterval);
      requestRepaint();
    }
  }
}

/*bool PackageTab::execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                         const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    QString symbolName =
        FilePath::cleanFileName(*mPackage->getNames().getDefaultValue(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    FilePath defaultFilePath(QDir::homePath() % "/" % symbolName % "_Symbol");

    // Copy symbol items to allow processing them in worker threads.
    QList<std::shared_ptr<GraphicsPagePainter>> pages = {
        std::make_shared<SymbolPainter>(*mPackage),
    };

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Board, output, pages, 0,
        *mPackage->getNames().getDefaultValue(), 0, defaultFilePath, mUnit,
        mApp.getWorkspace().getSettings().themes.getActive(),
        "symbol_editor/" % settingsKey, qApp->activeWindow());
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mApp.getWorkspace().getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
  return true;
}*/

void PackageTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

void PackageTab::applyTheme() noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();

  if (mScene) {
    mScene->setBackgroundColors(
        theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
    mScene->setOverlayColors(
        theme.getColor(Theme::Color::sBoardOverlays).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardOverlays).getSecondaryColor());
    mScene->setSelectionRectColors(
        theme.getColor(Theme::Color::sBoardSelection).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardSelection).getSecondaryColor());
    mScene->setGridStyle(mGridStyle);
  }

  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

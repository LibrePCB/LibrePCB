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

#include "../../3d/openglscenebuilder.h"
#include "../../3d/slintopenglview.h"
#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdholeedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdstroketextedit.h"
#include "../../dialogs/backgroundimagesetupdialog.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../widgets/unsignedlengthedit.h"
#include "../../workspace/categorytreemodel.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdfootprintedit.h"
#include "../cmd/cmdfootprintpadedit.h"
#include "../cmd/cmdpackageedit.h"
#include "../cmd/cmdpackagereload.h"
#include "../libraryeditor.h"
#include "../libraryelementcategoriesmodel.h"
#include "footprintgraphicsitem.h"
#include "footprintlistmodel.h"
#include "fsm/packageeditorfsm.h"
#include "fsm/packageeditorstate_addholes.h"
#include "fsm/packageeditorstate_addnames.h"
#include "fsm/packageeditorstate_addpads.h"
#include "fsm/packageeditorstate_addvalues.h"
#include "fsm/packageeditorstate_drawarc.h"
#include "fsm/packageeditorstate_drawcircle.h"
#include "fsm/packageeditorstate_drawline.h"
#include "fsm/packageeditorstate_drawpolygon.h"
#include "fsm/packageeditorstate_drawrect.h"
#include "fsm/packageeditorstate_drawtext.h"
#include "fsm/packageeditorstate_drawzone.h"
#include "fsm/packageeditorstate_measure.h"
#include "fsm/packageeditorstate_renumberpads.h"
#include "fsm/packageeditorstate_select.h"
#include "graphics/graphicslayerlist.h"
#include "packagemodellistmodel.h"
#include "packagepadlistmodel.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/pkg/footprintpainter.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/pkg/packagecheckmessages.h>
#include <librepcb/core/types/pcbcolor.h>
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

PackageTab::PackageTab(LibraryEditor& editor, std::unique_ptr<Package> pkg,
                       Mode mode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mPackage(std::move(pkg)),
    mLayers(GraphicsLayerList::libraryLayers(
        &mEditor.getWorkspace().getSettings())),
    mView(new SlintGraphicsView(SlintGraphicsView::defaultFootprintSceneRect(),
                                this)),
    mOpenGlSceneRebuildScheduled(false),
    mIsNewElement(isPathOutsideLibDir()),
    mWizardMode(mode != Mode::Open),
    mCurrentPageIndex(mWizardMode ? 0 : 2),
    mView3d(false),
    mGridStyle(mApp.getWorkspace()
                   .getSettings()
                   .themes.getActive()
                   .getBoardGridStyle()),
    mGridInterval(2540000),
    mUnit(LengthUnit::millimeters()),
    mChooseCategory(false),
    mOpenGlProjection(new OpenGlProjection()),
    mFrameIndex(0),
    mNameParsed(mPackage->getNames().getDefaultValue()),
    mVersionParsed(mPackage->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::PackageCategory)),
    mCategoriesTree(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                          editor.getWorkspace().getSettings(),
                                          CategoryTreeModel::Filter::PkgCat)),
    mAssemblyType(mPackage->getAssemblyType(false)),
    mPads(new PackagePadListModel()),
    mPadsSorted(new slint::SortModel<ui::PackagePadData>(
        mPads,
        [](const ui::PackagePadData& a, const ui::PackagePadData& b) {
          return a.sort_index < b.sort_index;
        })),
    mFootprints(new FootprintListModel()),
    mModels(new PackageModelListModel()),
    mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolLineWidth(mApp.getWorkspace().getSettings()),
    mToolSize(mApp.getWorkspace().getSettings()),
    mToolDrill(mApp.getWorkspace().getSettings()),
    mToolRatio(Ratio(0)),
    mToolFilled(false),
    mToolGrabArea(false),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolPackagePads(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolComponentSide(FootprintPad::ComponentSide::Top),
    mToolShape(ui::PadShape::Round),
    mToolFiducial(false),
    mToolPressFit(false),
    mToolZoneLayers(),
    mToolZoneRules(),
    mBackgroundImageGraphicsItem(new QGraphicsPixmapItem()),
    mIsInterfaceBroken(false),
    mOriginalPackagePadUuids(mPackage->getPads().getUuidSet()),
    mOriginalFootprints(mPackage->getFootprints()) {
  // Setup graphics view.
  mView->setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
  mView->setEventHandler(this);
  connect(
      &mApp.getWorkspace().getSettings().useOpenGl,
      &WorkspaceSettingsItem_GenericValue<bool>::edited, this, [this]() {
        mView->setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
      });
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &PackageTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          &PackageTab::notifyDerivedUiDataChanged);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this, [this]() {
    mAutoReloadOnFileModifications = false;  // Disable auto-reload.
    scheduleChecks();
    scheduleOpenGlSceneUpdate();
    refreshUiData();
  });

  // Connect models.
  mPads->setReferences(&mPackage->getPads(), mUndoStack.get());
  mFootprints->setReferences(mPackage.get(), mUndoStack.get());
  mModels->setReferences(mPackage.get(), mUndoStack.get());
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &PackageTab::commitUiData, Qt::QueuedConnection);
  connect(mFootprints.get(), &FootprintListModel::footprintAdded, this,
          &PackageTab::setCurrentFootprintIndex);

  // Setup background image.
  mBackgroundImageGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mBackgroundImageGraphicsItem->setTransformationMode(Qt::SmoothTransformation);
  mBackgroundImageGraphicsItem->setZValue(-1000);
  mBackgroundImageGraphicsItem->setOpacity(0.8);
  mBackgroundImageGraphicsItem->setVisible(false);
  mBackgroundImageSettings.tryLoadFromDir(getBackgroundImageCacheDir());
  applyBackgroundImageSettings();

  // Load finite state machine (FSM).
  PackageEditorFsm::Context fsmContext{*mPackage, *mUndoStack, !isWritable(),
                                       mUnit,     *mLayers,    *this,
                                       nullptr,   nullptr};
  mFsm.reset(new PackageEditorFsm(fsmContext));

  // Load the first footprint & 3D model.
  setCurrentFootprintIndex(0);

  // Refresh content.
  refreshUiData();
  scheduleChecks();

  // Setup file system watcher.
  updateWatchedFiles();

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

PackageTab::~PackageTab() noexcept {
  deactivate();

  // Clean up the state machine nicely to avoid unexpected behavior. Triggering
  // abort (Esc) two times is usually sufficient to leave any active tool, so
  // let's call it three times to be on the safe side. Unfortunately there's
  // no clean way to forcible and guaranteed leaving a tool.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm.reset();

  // Reset references to avoid dangling pointers as the UI might still have
  // shared pointers to these models.
  mPads->setReferences(nullptr, nullptr);
  mFootprints->setReferences(nullptr, nullptr);
  mModels->setReferences(nullptr, nullptr);
  mView->setEventHandler(nullptr);

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();
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
  if ((!mWizardMode) && (mCurrentPageIndex == 2) && (!mView3d) &&
      mFsm->getCurrentFootprint()) {
    features.grid = toFs(isWritable());
    features.zoom = toFs(true);
    features.background_image = toFs(true);
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
    features.flip = toFs(mToolFeatures.testFlag(Feature::Flip));
    features.move_align = toFs(mToolFeatures.testFlag(Feature::MoveAlign));
    features.snap_to_grid = toFs(mToolFeatures.testFlag(Feature::SnapToGrid));
    features.edit_properties =
        toFs(mToolFeatures.testFlag(Feature::Properties));
  }

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
  const QColor bgColor = mView3d
      ? SlintOpenGlView::getBackgroundColor()
      : theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;
  const bool refreshing =
      (mOpenGlSceneBuilder && mOpenGlSceneBuilder->isBusy());
  QStringList errors = mOpenGlSceneBuilderErrors;
  if (mOpenGlView) errors += mOpenGlView->getOpenGlErrors();

  return ui::PackageTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mPackage->getDirectory().getAbsPath().toStr()),  // Path
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      mView3d,  // View 3D
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
      l2s(mAssemblyType),  // Assembly type
      mPadsSorted,  // Package pads
      mNewPadName,  // New pad name
      mNewPadNameError,  // New pad name error
      mFootprints,  // Footprints
      mPackage->getFootprints().indexOf(
          mFsm->getCurrentFootprint().get()),  // Footprint index
      mModels,  // Models
      mPackage->getModels().indexOf(mCurrentModel.get()),  // Model index
      ui::RuleCheckData{
          ui::RuleCheckType::PackageCheck,  // Check type
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
      mBackgroundImageGraphicsItem->isVisible(),  // Background image set
      mAlpha.value(OpenGlObject::Type::SolderResist, 1),  // Solder resist alpha
      mAlpha.value(OpenGlObject::Type::Silkscreen, 1),  // Silkscreen alpha
      mAlpha.value(OpenGlObject::Type::SolderPaste, 1),  // Solder paste alpha
      mAlpha.value(OpenGlObject::Type::Device, 1),  // Devices alpha
      refreshing,  // Refreshing
      q2s(errors.join("\n\n")),  // Error
      !mModifiedWatchedFiles.isEmpty(),  // Watched files modified
      mIsInterfaceBroken,  // Interface broken
      mTool,  // Tool
      q2s(((mView3d && mOpenGlView) ? mOpenGlView->isPanning()
                                    : mView->isPanning())
              ? Qt::ClosedHandCursor
              : mToolCursorShape),  // Tool cursor
      q2s(mToolOverlayText),  // Tool overlay text
      ui::ComboBoxData{
          // Tool layer
          mToolLayers,  // Items
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current index
      },
      mToolLineWidth.getUiData(),  // Tool line width
      mToolSize.getUiData(),  // Tool size
      mToolDrill.getUiData(),  // Tool drill
      ui::AngleEditData{
          // Tool angle
          l2s(mToolAngle),  // Angle
          false,  // Increase
          false,  // Decrease
      },
      ui::RatioEditData{
          // Tool ratio
          l2s(*mToolRatio),  // Ratio
          l2s(Ratio::fromPercent(0)),  // Minimum
          l2s(Ratio::fromPercent(100)),  // Maximum
          (*mToolRatio) < Ratio::fromPercent(100),  // Can increase
          (*mToolRatio) > Ratio::fromPercent(0),  // Can decrease
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
      ui::ComboBoxData{
          // Tool package pad
          mToolPackagePads,  // Items
          static_cast<int>(
              mToolPackagePadsQt.indexOf(mToolPackagePad)),  // Current index
      },
      mToolComponentSide == FootprintPad::ComponentSide::Bottom,  // Tool bottom
      mToolShape,  // Tool shape
      mToolFiducial,  // Tool fiducial
      mToolPressFit,  // Tool press fit
      mToolZoneLayers.testFlag(Zone::Layer::Top),  // Tool layer top
      mToolZoneLayers.testFlag(Zone::Layer::Inner),  // Tool layer inner
      mToolZoneLayers.testFlag(Zone::Layer::Bottom),  // Tool layer bottom
      mToolZoneRules.testFlag(Zone::Rule::NoCopper),  // Tool no copper
      mToolZoneRules.testFlag(Zone::Rule::NoPlanes),  // Tool no planes
      mToolZoneRules.testFlag(Zone::Rule::NoExposure),  // Tool no exposure
      mToolZoneRules.testFlag(Zone::Rule::NoDevices),  // Tool no devices
      q2s(mSceneImagePos),  // Scene image position
      mFrameIndex,  // Frame index
      slint::SharedString(),  // New category
      slint::SharedString(),  // New footprint
  };
}

void PackageTab::setDerivedUiData(const ui::PackageTabData& data) noexcept {
  // General
  setCurrentModelIndex(data.model_index);
  setCurrentFootprintIndex(data.footprint_index);  // May also change the model.
  mSceneImagePos = s2q(data.scene_image_pos);

  // Page change
  if (data.page_index != mCurrentPageIndex) {
    mCurrentPageIndex = data.page_index;
    onUiDataChanged.notify();  // Some tab features will change!
  }
  if (data.view_3d != mView3d) {
    mView3d = data.view_3d;
    autoSelectCurrentModelIndex();
    updateOpenGlScene();
    onUiDataChanged.notify();  // Some tab features will change!
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
  if (auto at = s2assemblyType(data.assembly_type)) {
    mAssemblyType = *at;
  }

  // New pad
  if (data.new_pad_name != mNewPadName) {
    mNewPadName = data.new_pad_name;
    const QString name = s2q(mNewPadName);
    const QStringList names = Toolbox::expandRangesInString(name);
    const bool duplicate =
        std::any_of(names.begin(), names.end(), [this](const QString& n) {
          return mPackage->getPads().contains(cleanCircuitIdentifier(n));
        });
    if (!name.trimmed().isEmpty()) {
      validateCircuitIdentifier(names.value(0), mNewPadNameError, duplicate);
    } else {
      mNewPadNameError = slint::SharedString();
    }
  }

  // New footprint
  if (!data.new_footprint.empty()) {
    mFootprints->add(s2q(data.new_footprint));
  }

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
  mAlpha[OpenGlObject::Type::SolderResist] =
      qBound(0.0f, data.solderresist_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::Silkscreen] =
      qBound(0.0f, data.silkscreen_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::SolderPaste] =
      qBound(0.0f, data.solderpaste_alpha, 1.0f);
  mAlpha[OpenGlObject::Type::Device] = qBound(0.0f, data.devices_alpha, 1.0f);
  if (mOpenGlView) {
    mOpenGlView->setAlpha(mAlpha);
  }

  // Tool
  if (const Layer* layer = mToolLayersQt.value(data.tool_layer.current_index)) {
    emit layerRequested(*layer);
  }
  if (data.tool_angle.increase) {
    emit angleRequested(mToolAngle + Angle::deg45());
  } else if (data.tool_angle.decrease) {
    emit angleRequested(mToolAngle - Angle::deg45());
  } else {
    emit angleRequested(s2angle(data.tool_angle.value));
  }
  if (data.tool_ratio.increase) {
    emit ratioRequested(UnsignedLimitedRatio(std::min(
        *mToolRatio + Ratio::fromPercent(1), Ratio::fromPercent(100))));
  } else if (data.tool_ratio.decrease) {
    emit ratioRequested(UnsignedLimitedRatio(
        std::max(*mToolRatio - Ratio::fromPercent(1), Ratio::fromPercent(0))));
  } else {
    const Ratio ratio = s2ratio(data.tool_ratio.value);
    if ((ratio >= Ratio::fromPercent(0)) &&
        (ratio <= Ratio::fromPercent(100))) {
      emit ratioRequested(UnsignedLimitedRatio(ratio));
    }
  }
  emit filledRequested(data.tool_filled);
  emit grabAreaRequested(data.tool_grab_area);
  // Note: We set the drill before width/height to let the FSM decrease the
  // drill if width or height are set to a smaller value. This clipping does
  // not work in both directions yet because we don't know if the user edited
  // the drill or width/height.
  mToolDrill.setUiData(data.tool_drill);
  mToolLineWidth.setUiData(data.tool_line_width);
  mToolSize.setUiData(data.tool_size);
  emit valueRequested(EditorToolbox::toMultiLine(s2q(data.tool_value.text)));
  emit hAlignRequested(s2l(data.tool_halign));
  emit vAlignRequested(s2l(data.tool_valign));
  emit packagePadRequested(
      mToolPackagePadsQt.value(data.tool_package_pad.current_index));
  emit componentSideRequested(data.tool_bottom
                                  ? FootprintPad::ComponentSide::Bottom
                                  : FootprintPad::ComponentSide::Top);
  emit shapeRequested(data.tool_shape);
  emit pressFitRequested(data.tool_pressfit);
  emit zoneLayerRequested(Zone::Layer::Top, data.tool_layer_top);
  emit zoneLayerRequested(Zone::Layer::Inner, data.tool_layer_inner);
  emit zoneLayerRequested(Zone::Layer::Bottom, data.tool_layer_bottom);
  emit zoneRuleRequested(Zone::Rule::NoCopper, data.tool_no_copper);
  emit zoneRuleRequested(Zone::Rule::NoPlanes, data.tool_no_planes);
  emit zoneRuleRequested(Zone::Rule::NoExposure, data.tool_no_exposures);
  emit zoneRuleRequested(Zone::Rule::NoDevices, data.tool_no_devices);

  requestRepaint();
}

void PackageTab::activate() noexcept {
  mScene.reset(new GraphicsScene(this));
  mScene->setGridInterval(mGridInterval);
  connect(mScene.get(), &GraphicsScene::changed, this,
          &PackageTab::requestRepaint);

  mScene->addItem(*mBackgroundImageGraphicsItem);
  if (auto item = mFsm->getCurrentGraphicsItem()) {
    mScene->addItem(*item);
  }

  mOpenGlView.reset(new SlintOpenGlView(*mOpenGlProjection, this));
  mOpenGlView->setAlpha(mAlpha);
  connect(mOpenGlView.get(), &SlintOpenGlView::stateChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(mOpenGlView.get(), &SlintOpenGlView::contentChanged, this,
          &PackageTab::requestRepaint);

  mOpenGlSceneBuilder.reset(new OpenGlSceneBuilder());
  connect(mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectAdded,
          mOpenGlView.get(), &SlintOpenGlView::addObject);
  connect(mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::objectRemoved,
          mOpenGlView.get(), &SlintOpenGlView::removeObject);
  connect(mOpenGlSceneBuilder.get(), &OpenGlSceneBuilder::finished, this,
          [this](QStringList errors) {
            mOpenGlSceneBuilderErrors = errors;
            requestRepaint();
          });

  mOpenGlSceneRebuildTimer.reset(new QTimer(this));
  mOpenGlSceneRebuildTimer->setSingleShot(true);
  connect(mOpenGlSceneRebuildTimer.get(), &QTimer::timeout, this,
          &PackageTab::updateOpenGlScene);

  applyTheme();
  scheduleOpenGlSceneUpdate();
  updateOpenGlScene();
  requestRepaint();
}

void PackageTab::deactivate() noexcept {
  if (mOpenGlView) {
    *mOpenGlProjection = mOpenGlView->getProjection();
  }
  mOpenGlSceneRebuildTimer.reset();
  mOpenGlSceneBuilder.reset();
  mOpenGlView.reset();

  // Currently we don't reset the graphics item because the FSM has a handle to
  // it anyway so it won't be freed.
  auto item = mFsm->getCurrentGraphicsItem();
  if (mScene && item) {
    mScene->removeItem(*item);
  }
  if (mScene && (mBackgroundImageGraphicsItem->scene() == mScene.get())) {
    mScene->removeItem(*mBackgroundImageGraphicsItem);
  }
  mScene.reset();
}

void PackageTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Accept: {
      mFsm->processAcceptCommand();
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
        save();
      } else if (mWizardMode && (mCurrentPageIndex == 1)) {
        mWizardMode = false;
        ++mCurrentPageIndex;
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
    case ui::TabAction::ReloadFromDisk: {
      try {
        reloadFromDisk();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
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
    case ui::TabAction::Print: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
      break;
    }
    case ui::TabAction::ExportImage: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                               "image_export");
      break;
    }
    case ui::TabAction::ExportPdf: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
      break;
    }
    case ui::TabAction::ImportDxf: {
      mFsm->processStartDxfImport();
      break;
    }
    case ui::TabAction::SelectAll: {
      mFsm->processSelectAll();
      break;
    }
    case ui::TabAction::Abort: {
      mFsm->processAbortCommand();
      break;
    }
    case ui::TabAction::Cut: {
      mFsm->processCut();
      break;
    }
    case ui::TabAction::Copy: {
      mFsm->processCopy();
      break;
    }
    case ui::TabAction::Paste: {
      mFsm->processPaste();
      break;
    }
    case ui::TabAction::Delete: {
      mFsm->processRemove();
      break;
    }
    case ui::TabAction::RotateCcw: {
      mFsm->processRotate(Angle::deg90());
      break;
    }
    case ui::TabAction::RotateCw: {
      mFsm->processRotate(-Angle::deg90());
      break;
    }
    case ui::TabAction::MirrorHorizontally: {
      mFsm->processMirror(Qt::Horizontal);
      break;
    }
    case ui::TabAction::MirrorVertically: {
      mFsm->processMirror(Qt::Vertical);
      break;
    }
    case ui::TabAction::FlipHorizontally: {
      mFsm->processFlip(Qt::Horizontal);
      break;
    }
    case ui::TabAction::FlipVertically: {
      mFsm->processFlip(Qt::Vertical);
      break;
    }
    case ui::TabAction::MoveAlign: {
      mFsm->processMoveAlign();
      break;
    }
    case ui::TabAction::MoveLeft: {
      if (!mFsm->processMove(Point(-mGridInterval, 0))) {
        mView->scrollLeft();
      }
      break;
    }
    case ui::TabAction::MoveRight: {
      if (!mFsm->processMove(Point(*mGridInterval, 0))) {
        mView->scrollRight();
      }
      break;
    }
    case ui::TabAction::MoveUp: {
      if (!mFsm->processMove(Point(0, *mGridInterval))) {
        mView->scrollUp();
      }
      break;
    }
    case ui::TabAction::MoveDown: {
      if (!mFsm->processMove(Point(0, -mGridInterval))) {
        mView->scrollDown();
      }
      break;
    }
    case ui::TabAction::SnapToGrid: {
      mFsm->processSnapToGrid();
      break;
    }
    case ui::TabAction::EditProperties: {
      mFsm->processEditProperties();
      break;
    }
    case ui::TabAction::GridIntervalIncrease: {
      setGridInterval(PositiveLength(mGridInterval * 2));
      break;
    }
    case ui::TabAction::GridIntervalDecrease: {
      if ((*mGridInterval % 2) == 0) {
        setGridInterval(PositiveLength(mGridInterval / 2));
      }
      break;
    }
    case ui::TabAction::ZoomIn: {
      if (mView3d && mOpenGlView) {
        mOpenGlView->zoomIn();
      } else if ((!mView3d) && mView) {
        mView->zoomIn();
      }
      break;
    }
    case ui::TabAction::ZoomOut: {
      if (mView3d && mOpenGlView) {
        mOpenGlView->zoomOut();
      } else if ((!mView3d) && mView) {
        mView->zoomOut();
      }
      break;
    }
    case ui::TabAction::ZoomFit: {
      if (mView3d && mOpenGlView) {
        mOpenGlView->zoomAll();
      } else if ((!mView3d) && mView && mScene) {
        mView->zoomToSceneRect(mScene->itemsBoundingRect());
      }
      break;
    }
    case ui::TabAction::ToggleBackgroundImage: {
      toggleBackgroundImage();
      break;
    }
    case ui::TabAction::PackageAddPads: {
      if (mPads->add(s2q(mNewPadName))) {
        mNewPadName = slint::SharedString();
        mNewPadNameError = slint::SharedString();
        onDerivedUiDataChanged.notify();
      }
      break;
    }
    case ui::TabAction::PackageAddModel: {
      if (auto index = mModels->add()) {
        setCurrentModelIndex(*index);
      }
      break;
    }
    case ui::TabAction::PackageGenerateOutline: {
      mFsm->processGenerateOutline();
      break;
    }
    case ui::TabAction::PackageGenerateCourtyard: {
      mFsm->processGenerateCourtyard();
      break;
    }
    case ui::TabAction::ToolSelect: {
      mFsm->processStartSelecting();
      break;
    }
    case ui::TabAction::ToolLine: {
      mFsm->processStartDrawLines();
      break;
    }
    case ui::TabAction::ToolRect: {
      mFsm->processStartDrawRects();
      break;
    }
    case ui::TabAction::ToolPolygon: {
      mFsm->processStartDrawPolygons();
      break;
    }
    case ui::TabAction::ToolCircle: {
      mFsm->processStartDrawCircles();
      break;
    }
    case ui::TabAction::ToolArc: {
      mFsm->processStartDrawArcs();
      break;
    }
    case ui::TabAction::ToolName: {
      mFsm->processStartAddingNames();
      break;
    }
    case ui::TabAction::ToolValue: {
      mFsm->processStartAddingValues();
      break;
    }
    case ui::TabAction::ToolText: {
      mFsm->processStartDrawTexts();
      break;
    }
    case ui::TabAction::ToolPadTht: {
      mFsm->processStartAddingFootprintThtPads();
      break;
    }
    case ui::TabAction::ToolPadSmt: {
      mFsm->processStartAddingFootprintSmtPads(
          FootprintPad::Function::StandardPad);
      break;
    }
    case ui::TabAction::ToolPadThermal: {
      mFsm->processStartAddingFootprintSmtPads(
          FootprintPad::Function::ThermalPad);
      break;
    }
    case ui::TabAction::ToolPadBga: {
      mFsm->processStartAddingFootprintSmtPads(FootprintPad::Function::BgaPad);
      break;
    }
    case ui::TabAction::ToolPadEdgeConnector: {
      mFsm->processStartAddingFootprintSmtPads(
          FootprintPad::Function::EdgeConnectorPad);
      break;
    }
    case ui::TabAction::ToolPadTestPoint: {
      mFsm->processStartAddingFootprintSmtPads(FootprintPad::Function::TestPad);
      break;
    }
    case ui::TabAction::ToolPadLocalFiducial: {
      mFsm->processStartAddingFootprintSmtPads(
          FootprintPad::Function::LocalFiducial);
      break;
    }
    case ui::TabAction::ToolPadGlobalFiducial: {
      mFsm->processStartAddingFootprintSmtPads(
          FootprintPad::Function::GlobalFiducial);
      break;
    }
    case ui::TabAction::ToolZone: {
      mFsm->processStartDrawZones();
      break;
    }
    case ui::TabAction::ToolHole: {
      mFsm->processStartAddingHoles();
      break;
    }
    case ui::TabAction::ToolRenumberPads: {
      mFsm->processStartReNumberPads();
      break;
    }
    case ui::TabAction::ToolMeasure: {
      mFsm->processStartMeasure();
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
  if ((!mView3d) && mScene) {
    return mView->render(*mScene, width, height);
  } else if (mView3d && mOpenGlView) {
    return mOpenGlView->render(width, height);
  }
  return slint::Image();
}

bool PackageTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  if (mView3d && mOpenGlView) {
    return mOpenGlView->pointerEvent(pos, e);
  } else {
    return mView->pointerEvent(pos, e);
  }
}

bool PackageTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  if (mView3d && mOpenGlView) {
    return mOpenGlView->scrollEvent(pos, e);
  } else {
    return mView->scrollEvent(pos, e);
  }
}

bool PackageTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  return mView->keyEvent(e);
}

bool PackageTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The package '%1' contains unsaved changes.\n"
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
  return mFsm->processKeyPressed(e);
}

bool PackageTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool PackageTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  emit cursorCoordinatesChanged(e.scenePos, mUnit);
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool PackageTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool PackageTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  PackageEditorFsmAdapter
 ******************************************************************************/

GraphicsScene* PackageTab::fsmGetGraphicsScene() noexcept {
  return mScene.get();
}

PositiveLength PackageTab::fsmGetGridInterval() const noexcept {
  return mGridInterval;
}

void PackageTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mToolCursorShape = *shape;
  } else {
    mToolCursorShape = Qt::ArrowCursor;
  }
  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmSetViewGrayOut(bool grayOut) noexcept {
  if (mScene) {
    mScene->setGrayOut(grayOut);
  }
}

void PackageTab::fsmSetViewInfoBoxText(const QString& text) noexcept {
  QString t = text;
  t.replace("&nbsp;", " ");
  t.replace("<br>", "\n");
  t.replace("<b>", "");
  t.replace("</b>", "");

  if (t != mToolOverlayText) {
    mToolOverlayText = t;
    onDerivedUiDataChanged.notify();
  }
}

void PackageTab::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  if (mScene) {
    mScene->setRulerPositions(pos);
  }
}

void PackageTab::fsmSetSceneCursor(const Point& pos, bool cross,
                                   bool circle) noexcept {
  if (mScene) {
    mScene->setSceneCursor(pos, cross, circle);
  }
}

QPainterPath PackageTab::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return mView->calcPosWithTolerance(pos, multiplier);
}

Point PackageTab::fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mView->mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void PackageTab::fsmSetStatusBarMessage(const QString& message,
                                        int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void PackageTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    onUiDataChanged.notify();
  }
}

void PackageTab::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mTool = ui::EditorTool::Select;
  fsmSetFeatures(Features());
  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_Select& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Select;
  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawLine& state) noexcept {
  mTool = ui::EditorTool::Line;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawLine::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &PackageTab::layerRequested, &state,
                                      &PackageEditorState_DrawLine::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_line/line_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawLine::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawLine::setLineWidth));

  // Angle
  auto setAngle = [this](const Angle& angle) {
    mToolAngle = angle;
    onDerivedUiDataChanged.notify();
  };
  setAngle(state.getAngle());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawLine::angleChanged, this, setAngle));
  mFsmStateConnections.append(connect(this, &PackageTab::angleRequested, &state,
                                      &PackageEditorState_DrawLine::setAngle));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawRect& state) noexcept {
  mTool = ui::EditorTool::Rect;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawRect::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &PackageTab::layerRequested, &state,
                                      &PackageEditorState_DrawRect::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_rect/line_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawRect::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawRect::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawRect::filledChanged, this, setFilled));
  mFsmStateConnections.append(connect(this, &PackageTab::filledRequested,
                                      &state,
                                      &PackageEditorState_DrawRect::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawRect::grabAreaChanged, this,
              setGrabArea));
  mFsmStateConnections.append(
      connect(this, &PackageTab::grabAreaRequested, &state,
              &PackageEditorState_DrawRect::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawPolygon& state) noexcept {
  mTool = ui::EditorTool::Polygon;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawPolygon::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &PackageTab::layerRequested, &state,
              &PackageEditorState_DrawPolygon::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_polygon/line_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawPolygon::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawPolygon::setLineWidth));

  // Angle
  auto setAngle = [this](const Angle& angle) {
    mToolAngle = angle;
    onDerivedUiDataChanged.notify();
  };
  setAngle(state.getAngle());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawPolygon::angleChanged, this, setAngle));
  mFsmStateConnections.append(
      connect(this, &PackageTab::angleRequested, &state,
              &PackageEditorState_DrawPolygon::setAngle));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawPolygon::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &PackageTab::filledRequested, &state,
              &PackageEditorState_DrawPolygon::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawPolygon::grabAreaChanged, this,
              setGrabArea));
  mFsmStateConnections.append(
      connect(this, &PackageTab::grabAreaRequested, &state,
              &PackageEditorState_DrawPolygon::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawCircle& state) noexcept {
  mTool = ui::EditorTool::Circle;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawCircle::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &PackageTab::layerRequested, &state,
              &PackageEditorState_DrawCircle::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_circle/line_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawCircle::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawCircle::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawCircle::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &PackageTab::filledRequested, &state,
              &PackageEditorState_DrawCircle::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawCircle::grabAreaChanged, this,
              setGrabArea));
  mFsmStateConnections.append(
      connect(this, &PackageTab::grabAreaRequested, &state,
              &PackageEditorState_DrawCircle::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawArc& state) noexcept {
  mTool = ui::EditorTool::Arc;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawArc::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &PackageTab::layerRequested, &state,
                                      &PackageEditorState_DrawArc::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_arc/line_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawArc::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawArc::setLineWidth));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_AddNames& state) noexcept {
  mTool = ui::EditorTool::Name;

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "package_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &PackageEditorState_DrawText::setHeight));

  // Stroke width
  mToolLineWidth.configure(state.getStrokeWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_text/stroke_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_AddNames::strokeWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_AddNames::setStrokeWidth));

  // Horizontal alignment
  auto setHAlign = [this](const HAlign& align) {
    mToolAlign.setH(align);
    onDerivedUiDataChanged.notify();
  };
  setHAlign(state.getHAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::hAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::vAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_AddValues& state) noexcept {
  mTool = ui::EditorTool::Value;

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "package_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &PackageEditorState_DrawText::setHeight));

  // Stroke width
  mToolLineWidth.configure(state.getStrokeWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_text/stroke_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_AddValues::strokeWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_AddValues::setStrokeWidth));

  // Horizontal alignment
  auto setHAlign = [this](const HAlign& align) {
    mToolAlign.setH(align);
    onDerivedUiDataChanged.notify();
  };
  setHAlign(state.getHAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::hAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::vAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawText& state) noexcept {
  mTool = ui::EditorTool::Text;

  // Layers
  mToolLayersQt = Layer::sorted(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &PackageTab::layerRequested, &state,
                                      &PackageEditorState_DrawText::setLayer));

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "package_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &PackageEditorState_DrawText::setHeight));

  // Stroke width
  mToolLineWidth.configure(state.getStrokeWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/draw_text/stroke_width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_DrawText::strokeWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &PackageEditorState_DrawText::setStrokeWidth));

  // Text
  auto setText = [this](const QString& text) {
    mToolValue = text;
    onDerivedUiDataChanged.notify();
  };
  setText(state.getText());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::textChanged, this, setText));
  mFsmStateConnections.append(connect(this, &PackageTab::valueRequested, &state,
                                      &PackageEditorState_DrawText::setText));

  // Text suggestions
  mToolValueSuggestions->clear();
  for (const QString& v : state.getTextSuggestions()) {
    mToolValueSuggestions->push_back(q2s(v));
  }

  // Horizontal alignment
  auto setHAlign = [this](const HAlign& align) {
    mToolAlign.setH(align);
    onDerivedUiDataChanged.notify();
  };
  setHAlign(state.getHAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::hAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &PackageTab::vAlignRequested,
                                      &state,
                                      &PackageEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_AddPads& state) noexcept {
  mTool = state.getType() == PackageEditorState_AddPads::PadType::THT
      ? ui::EditorTool::PadTht
      : ui::EditorTool::PadSmt;

  // Package pads
  mToolPackagePadsQt = {std::nullopt};
  mToolPackagePads->set_vector({q2s(tr("(unconnected)"))});
  if (!state.getFunctionIsFiducial()) {
    for (auto pad : mPackage->getPads()) {
      mToolPackagePadsQt.append(pad.getUuid());
      mToolPackagePads->push_back(q2s(*pad.getName()));
    }
  }

  // Package pad
  mToolPackagePad = std::nullopt;
  if (!state.getFunctionIsFiducial()) {
    auto setPackagePad = [this](const std::optional<Uuid>& pad) {
      mToolPackagePad = pad;
      onDerivedUiDataChanged.notify();
    };
    setPackagePad(state.getPackagePad());
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::packagePadChanged, this,
                setPackagePad));
    mFsmStateConnections.append(
        connect(this, &PackageTab::packagePadRequested, &state,
                &PackageEditorState_AddPads::setPackagePad));
  }

  // Component side
  if (state.getType() == PackageEditorState_AddPads::PadType::SMT) {
    auto setComponentSide = [this](FootprintPad::ComponentSide side) {
      mToolComponentSide = side;
      onDerivedUiDataChanged.notify();
    };
    setComponentSide(state.getComponentSide());
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::componentSideChanged, this,
                setComponentSide));
    mFsmStateConnections.append(
        connect(this, &PackageTab::componentSideRequested, &state,
                &PackageEditorState_AddPads::setComponentSide));
  }

  // Shape
  auto getCurrentShape = [](PackageEditorState_AddPads& s) {
    if (s.getShape() == FootprintPad::Shape::RoundedOctagon) {
      return ui::PadShape::Octagon;
    } else if (s.getShape() == FootprintPad::Shape::Custom) {
      return ui::PadShape::Octagon;  // Not currect but should never be the
                                     // case.
    } else if (*s.getRadius() == Ratio::fromPercent(0)) {
      return ui::PadShape::Rect;
    } else if (*s.getRadius() == Ratio::fromPercent(100)) {
      return ui::PadShape::Round;
    } else {
      return ui::PadShape::RoundedRect;
    }
  };
  mToolShape = getCurrentShape(state);
  mFsmStateConnections.append(connect(
      this, &PackageTab::shapeRequested, &state,
      [this, &state](ui::PadShape shape) {
        if (shape != mToolShape) {
          switch (shape) {
            case ui::PadShape::Round:
              state.setShape(FootprintPad::Shape::RoundedRect);
              state.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
              break;
            case ui::PadShape::RoundedRect:
              state.setShape(FootprintPad::Shape::RoundedRect);
              state.setRadius(
                  UnsignedLimitedRatio(FootprintPad::getRecommendedRadius(
                      state.getWidth(), state.getHeight())));
              break;
            case ui::PadShape::Rect:
              state.setShape(FootprintPad::Shape::RoundedRect);
              state.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
              break;
            case ui::PadShape::Octagon:
              state.setShape(FootprintPad::Shape::RoundedOctagon);
              state.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
              break;
            default:
              break;
          }
          mToolShape = shape;
        }
      }));

  // Width / size
  mToolLineWidth.configure(state.getWidth(),
                           LengthEditContext::Steps::generic(),
                           "package_editor/add_pads/width");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_AddPads::widthChanged,
              &mToolLineWidth, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedPositive, &state,
              &PackageEditorState_AddPads::setWidth));
  if (state.getFunctionIsFiducial()) {
    mFsmStateConnections.append(
        connect(&mToolLineWidth, &LengthEditContext::valueChangedPositive,
                &state, &PackageEditorState_AddPads::setHeight));
  }

  // Height
  if (!state.getFunctionIsFiducial()) {
    mToolSize.configure(state.getHeight(), LengthEditContext::Steps::generic(),
                        "package_editor/add_pads/height");
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::heightChanged, &mToolSize,
                &LengthEditContext::setValuePositive));
    mFsmStateConnections.append(
        connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
                &PackageEditorState_AddPads::setHeight));
  }

  // Fiducial clearance
  const auto clearance = state.getStopMaskConfig().getOffset();
  if (state.getFunctionIsFiducial() && clearance && (clearance >= 0)) {
    mToolSize.configure(UnsignedLength(*clearance),
                        LengthEditContext::Steps::generic(),
                        "package_editor/add_pads/fiducial_clearance");
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::copperClearanceChanged,
                &mToolSize, &LengthEditContext::setValueUnsigned));
    mFsmStateConnections.append(
        connect(&mToolSize, &LengthEditContext::valueChangedUnsigned, &state,
                [&state](const UnsignedLength& value) {
                  state.setCopperClearance(value);
                  state.setStopMaskConfig(MaskConfig::manual(*value));
                }));
  }

  // Drill
  if (auto drill = state.getDrillDiameter()) {
    mToolDrill.configure(*drill, LengthEditContext::Steps::drillDiameter(),
                         "package_editor/add_pads/drill_diameter");
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::drillDiameterChanged,
                &mToolDrill, &LengthEditContext::setValuePositive));
    mFsmStateConnections.append(
        connect(&mToolDrill, &LengthEditContext::valueChangedPositive, &state,
                &PackageEditorState_AddPads::setDrillDiameter));
  }

  // Radius
  auto setRadius = [this](const UnsignedLimitedRatio& radius) {
    mToolRatio = radius;
    onDerivedUiDataChanged.notify();
  };
  setRadius(state.getRadius());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_AddPads::radiusChanged, this, setRadius));
  mFsmStateConnections.append(connect(this, &PackageTab::ratioRequested, &state,
                                      &PackageEditorState_AddPads::setRadius));

  // Fiducial
  mToolFiducial = state.getFunctionIsFiducial();

  // Press-fit
  if (state.getType() == PackageEditorState_AddPads::PadType::THT) {
    auto setFunction = [this](FootprintPad::Function function) {
      mToolPressFit = function == FootprintPad::Function::PressFitPad;
      onDerivedUiDataChanged.notify();
    };
    setFunction(state.getFunction());
    mFsmStateConnections.append(
        connect(&state, &PackageEditorState_AddPads::functionChanged, this,
                setFunction));
    mFsmStateConnections.append(connect(
        this, &PackageTab::pressFitRequested, &state, [&state](bool pressFit) {
          state.setFunction(pressFit ? FootprintPad::Function::PressFitPad
                                     : FootprintPad::Function::StandardPad);
        }));
  }

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_DrawZone& state) noexcept {
  mTool = ui::EditorTool::Zone;

  // Layers
  auto setLayers = [this](Zone::Layers layers) {
    mToolZoneLayers = layers;
    onDerivedUiDataChanged.notify();
  };
  setLayers(state.getLayers());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawZone::layersChanged, this, setLayers));
  mFsmStateConnections.append(connect(this, &PackageTab::zoneLayerRequested,
                                      &state,
                                      &PackageEditorState_DrawZone::setLayer));

  // Rules
  auto setRules = [this](Zone::Rules rules) {
    mToolZoneRules = rules;
    onDerivedUiDataChanged.notify();
  };
  setRules(state.getRules());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawZone::rulesChanged, this, setRules));
  mFsmStateConnections.append(connect(this, &PackageTab::zoneRuleRequested,
                                      &state,
                                      &PackageEditorState_DrawZone::setRule));

  // Angle
  auto setAngle = [this](const Angle& angle) {
    mToolAngle = angle;
    onDerivedUiDataChanged.notify();
  };
  setAngle(state.getAngle());
  mFsmStateConnections.append(connect(
      &state, &PackageEditorState_DrawZone::angleChanged, this, setAngle));
  mFsmStateConnections.append(connect(this, &PackageTab::angleRequested, &state,
                                      &PackageEditorState_DrawZone::setAngle));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_AddHoles& state) noexcept {
  mTool = ui::EditorTool::Hole;

  // Drill
  mToolDrill.configure(state.getDiameter(),
                       LengthEditContext::Steps::drillDiameter(),
                       "package_editor/add_hole/diameter");
  mFsmStateConnections.append(
      connect(&state, &PackageEditorState_AddHoles::diameterChanged,
              &mToolDrill, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolDrill, &LengthEditContext::valueChangedPositive, &state,
              &PackageEditorState_AddHoles::setDiameter));

  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_ReNumberPads& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::RenumberPads;
  onDerivedUiDataChanged.notify();
}

void PackageTab::fsmToolEnter(PackageEditorState_Measure& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Measure;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PackageTab::watchedFilesModifiedChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

void PackageTab::reloadFromDisk() {
  const std::optional<Uuid> currentFpt = mFsm->getCurrentFootprint()
      ? std::make_optional(mFsm->getCurrentFootprint()->getUuid())
      : std::optional<Uuid>();
  const std::optional<Uuid> currentModel = mCurrentModel
      ? std::make_optional(mCurrentModel->getUuid())
      : std::optional<Uuid>();

  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mUndoStack->execCmd(new CmdPackageReload(*mPackage));
  mUndoStack->setClean();
  memorizeInterface();
  updateWatchedFiles();
  mManualModificationsMade = false;
  mAutoReloadOnFileModifications = true;  // Enable auto-reload.

  if (currentFpt && mPackage->getFootprints().find(*currentFpt)) {
    setCurrentFootprintIndex(mPackage->getFootprints().indexOf(*currentFpt));
  }
  if (currentModel && mPackage->getModels().find(*currentModel)) {
    setCurrentModelIndex(mPackage->getModels().indexOf(*currentModel));
  }

  // This is actually already called by the undo stack change, but the
  // memorized interface is updated afterwards, so we need to call it again.
  refreshUiData();
}

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
  if (autoFixHelper<MsgDeprecatedAssemblyType>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgSuspiciousAssemblyType>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingPackageOutline>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingCourtyard>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMinimumWidthViolation>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingFootprint>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingFootprintModel>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingFootprintName>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingFootprintValue>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgFootprintOriginNotInCenter>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgWrongFootprintTextLayer>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgUnusedCustomPadOutline>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgInvalidCustomPadOutline>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgPadStopMaskOff>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgSmtPadWithSolderPaste>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgThtPadWithSolderPaste>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgPadWithCopperClearance>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgFiducialClearanceLessThanStopMask>(msg, checkOnly))
    return true;
  if (autoFixHelper<MsgHoleWithoutStopMask>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgUnspecifiedPadFunction>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgSuspiciousPadFunction>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool PackageTab::autoFixHelper(
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
bool PackageTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitUiData();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mChooseCategory = true;
  onDerivedUiDataChanged.notify();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgDeprecatedAssemblyType& msg) {
  Q_UNUSED(msg);
  std::unique_ptr<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
  cmd->setAssemblyType(mPackage->guessAssemblyType());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgSuspiciousAssemblyType& msg) {
  Q_UNUSED(msg);
  std::unique_ptr<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
  cmd->setAssemblyType(mPackage->guessAssemblyType());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingPackageOutline& msg) {
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processGenerateOutline();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingCourtyard& msg) {
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processGenerateCourtyard();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMinimumWidthViolation& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));

  QDialog dlg(qApp->activeWindow());
  dlg.setWindowTitle(tr("New Line Width"));
  QVBoxLayout* vLayout = new QVBoxLayout(&dlg);
  UnsignedLengthEdit* edtWidth = new UnsignedLengthEdit(&dlg);
  edtWidth->configure(mUnit, LengthEditBase::Steps::generic(),
                      "package_editor/fix_minimum_width_dialog");
  edtWidth->setValue(UnsignedLength(200000));
  edtWidth->setFocus();
  vLayout->addWidget(edtWidth);
  QDialogButtonBox* btnBox = new QDialogButtonBox(&dlg);
  btnBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  vLayout->addWidget(btnBox);
  if (dlg.exec() != QDialog::Accepted) {
    return false;
  }

  if (auto p = footprint->getPolygons().find(msg.getPolygon().get())) {
    std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(*p));
    cmd->setLineWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.release());
  } else if (auto c = footprint->getCircles().find(msg.getCircle().get())) {
    std::unique_ptr<CmdCircleEdit> cmd(new CmdCircleEdit(*c));
    cmd->setLineWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.release());
  } else if (auto t =
                 footprint->getStrokeTexts().find(msg.getStrokeText().get())) {
    std::unique_ptr<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(*t));
    cmd->setStrokeWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.release());
  } else {
    throw LogicError(__FILE__, __LINE__,
                     "Whoops, not implemented! Please open a bug report.");
  }
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingFootprint& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<Footprint> fpt = std::make_shared<Footprint>(
      Uuid::createRandom(), ElementName("default"), "");
  mUndoStack->execCmd(new CmdFootprintInsert(mPackage->getFootprints(), fpt));
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingFootprintModel& msg) {
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mCurrentPageIndex = 2;
  mView3d = true;
  onDerivedUiDataChanged.notify();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingFootprintName& msg) {
  Q_UNUSED(msg);
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processStartAddingNames();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgMissingFootprintValue& msg) {
  Q_UNUSED(msg);
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processStartAddingValues();
  return true;
}

template <>
bool PackageTab::autoFix(const MsgFootprintOriginNotInCenter& msg) {
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processSelectAll();
  mFsm->processMove(-msg.getCenter());
  mFsm->processAbortCommand();  // Clear selection.
  return true;
}

template <>
bool PackageTab::autoFix(const MsgWrongFootprintTextLayer& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<StrokeText> text =
      footprint->getStrokeTexts().get(msg.getText().get());
  std::unique_ptr<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(*text));
  cmd->setLayer(msg.getExpectedLayer(), false);
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgUnusedCustomPadOutline& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setCustomShapeOutline(Path());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgInvalidCustomPadOutline& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setShape(FootprintPad::Shape::RoundedRect, false);
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgPadStopMaskOff& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setStopMaskConfig(MaskConfig::automatic(), false);
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgSmtPadWithSolderPaste& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setSolderPasteConfig(MaskConfig::off());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgThtPadWithSolderPaste& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setSolderPasteConfig(MaskConfig::off());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgPadWithCopperClearance& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setCopperClearance(UnsignedLength(0), false);
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgFiducialClearanceLessThanStopMask& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  const std::optional<Length> offset = pad->getStopMaskConfig().getOffset();
  if (offset && (*offset > 0)) {
    std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
    cmd->setCopperClearance(UnsignedLength(*offset), false);
    mUndoStack->execCmd(cmd.release());
    return true;
  }
  return false;
}

template <>
bool PackageTab::autoFix(const MsgHoleWithoutStopMask& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  setCurrentFootprintIndex(mPackage->getFootprints().indexOf(footprint.get()));
  std::shared_ptr<Hole> hole = footprint->getHoles().get(msg.getHole().get());
  std::unique_ptr<CmdHoleEdit> cmd(new CmdHoleEdit(*hole));
  cmd->setStopMaskConfig(MaskConfig::automatic());
  mUndoStack->execCmd(cmd.release());
  return true;
}

template <>
bool PackageTab::autoFix(const MsgUnspecifiedPadFunction& msg) {
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  return fixPadFunction(msg);
}

template <>
bool PackageTab::autoFix(const MsgSuspiciousPadFunction& msg) {
  setCurrentFootprintIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  return fixPadFunction(msg);
}

template <typename MessageType>
bool PackageTab::fixPadFunction(const MessageType& msg) {
  QMenu menu(qApp->activeWindow());
  QAction* aAll = menu.addAction(tr("Apply to all unspecified pads"));
  aAll->setCheckable(true);
  menu.addSeparator();
  for (int i = 0; i < static_cast<int>(FootprintPad::Function::_COUNT); ++i) {
    const FootprintPad::Function value = static_cast<FootprintPad::Function>(i);
    if (value != FootprintPad::Function::Unspecified) {
      QAction* action =
          menu.addAction(FootprintPad::getFunctionDescriptionTr(value));
      action->setData(QVariant::fromValue(value));
    }
  }

  QAction* action = nullptr;
  const QPoint pos = QCursor::pos();
  do {
    action = menu.exec(pos);
  } while (action == aAll);

  if (action && action->data().isValid() &&
      action->data().canConvert<FootprintPad::Function>()) {
    if (aAll->isChecked()) {
      UndoStackTransaction transaction(*mUndoStack,
                                       tr("Fix Unspecified Pad Functions"));
      for (auto& footprint : mPackage->getFootprints()) {
        for (auto& pad : footprint.getPads()) {
          if (pad.getFunction() == FootprintPad::Function::Unspecified) {
            std::unique_ptr<CmdFootprintPadEdit> cmd(
                new CmdFootprintPadEdit(pad));
            cmd->setFunction(action->data().value<FootprintPad::Function>(),
                             false);
            transaction.append(cmd.release());
          }
        }
      }
      transaction.commit();
      return true;
    } else {
      std::shared_ptr<Footprint> footprint =
          mPackage->getFootprints().get(msg.getFootprint().get());
      std::shared_ptr<FootprintPad> pad =
          footprint->getPads().get(msg.getPad().get());
      std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
      cmd->setFunction(action->data().value<FootprintPad::Function>(), false);
      mUndoStack->execCmd(cmd.release());
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageTab::setCurrentFootprintIndex(int index) noexcept {
  std::shared_ptr<Footprint> footprint = mPackage->getFootprints().value(index);
  if ((!mFsm) || (footprint == mFsm->getCurrentFootprint())) {
    return;
  }

  std::shared_ptr<FootprintGraphicsItem> item = mFsm->getCurrentGraphicsItem();
  if (mScene && item) {
    mScene->removeItem(*item);
  }

  if (footprint) {
    item.reset(new FootprintGraphicsItem(footprint, *mLayers,
                                         Application::getDefaultStrokeFont(),
                                         &mPackage->getPads()));
    if (mScene) mScene->addItem(*item);
  } else {
    item.reset();
  }

  mFsm->processChangeCurrentFootprint(footprint, item);

  autoSelectCurrentModelIndex();
  scheduleOpenGlSceneUpdate();
  onDerivedUiDataChanged.notify();  // Footprint index may have changed
}

void PackageTab::setCurrentModelIndex(int index) noexcept {
  std::shared_ptr<PackageModel> model = mPackage->getModels().value(index);
  if (model == mCurrentModel) {
    return;
  }

  mCurrentModel = model;
  scheduleOpenGlSceneUpdate();
  onDerivedUiDataChanged.notify();  // Model index may have changed
}

void PackageTab::autoSelectCurrentModelIndex() noexcept {
  std::shared_ptr<Footprint> footprint =
      mFsm ? mFsm->getCurrentFootprint() : nullptr;
  if (!footprint) return;

  if ((!mCurrentModel) ||
      (!footprint->getModels().contains(mCurrentModel->getUuid()))) {
    for (int i = 0; i < mPackage->getModels().count(); ++i) {
      if (footprint->getModels().contains(
              mPackage->getModels().at(i)->getUuid())) {
        setCurrentModelIndex(i);
        return;
      }
    }
    // No 3D model for this footprint.
    setCurrentModelIndex(-1);
  }
}

bool PackageTab::isWritable() const noexcept {
  return mIsNewElement || mPackage->getDirectory().isWritable();
}

void PackageTab::refreshUiData() noexcept {
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
  mAssemblyType = mPackage->getAssemblyType(false);

  // Update "interface broken" only when no command is active since it would
  // be annoying to get it during intermediate states.
  if (!mUndoStack->isCommandGroupActive()) {
    mIsInterfaceBroken = false;
    if ((!mIsNewElement) && (!mWizardMode)) {
      if (mPackage->getPads().getUuidSet() != mOriginalPackagePadUuids) {
        mIsInterfaceBroken = true;
      }
      for (const Footprint& original : mOriginalFootprints) {
        auto fpt = mPackage->getFootprints().find(original.getUuid());
        if ((!fpt) ||
            (fpt->getPads().getUuidSet() != original.getPads().getUuidSet())) {
          mIsInterfaceBroken = true;
        }
      }
    }
  }

  // If the currently displayed footprint was deleted, switch to another one.
  // Or if the first footprint was added, load it.
  if (mFsm &&
      (!mPackage->getFootprints().contains(
          mFsm->getCurrentFootprint().get()))) {
    setCurrentFootprintIndex(0);
  }

  // If the current 3D model is not available for the selected footprint,
  // switch to the first available 3D model. If no 3D model is available,
  // deselect it to make the user aware of the missing 3D model.
  autoSelectCurrentModelIndex();

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void PackageTab::commitUiData() noexcept {
  // Abort any active command as this would block the undo stack.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();

  try {
    std::unique_ptr<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mPackage->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mPackage->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mPackage->getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    cmd->setAssemblyType(mAssemblyType);
    mUndoStack->execCmd(cmd.release());  // can throw

    mPads->apply();  // can throw
    mFootprints->apply();  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool PackageTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
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
    mManualModificationsMade = false;
    memorizeInterface();
    updateWatchedFiles();
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    if (mWizardMode && (mCurrentPageIndex == 0)) {
      ++mCurrentPageIndex;
      scheduleChecks();
    }
    refreshUiData();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshUiData();
    return false;
  }
}

void PackageTab::memorizeInterface() noexcept {
  mOriginalPackagePadUuids = mPackage->getPads().getUuidSet();
  mOriginalFootprints = mPackage->getFootprints();
}

void PackageTab::updateWatchedFiles() noexcept {
  QSet<QString> files = {"package.lp"};
  for (const PackageModel& model : mPackage->getModels()) {
    files.insert(model.getFileName());
  }
  setWatchedFiles(mPackage->getDirectory(), files);
}

void PackageTab::setGridInterval(const PositiveLength& interval) noexcept {
  if (interval != mGridInterval) {
    mGridInterval = interval;
    mFsm->processGridIntervalChanged(mGridInterval);
    if (mScene) {
      mScene->setGridInterval(mGridInterval);
      requestRepaint();
    }
  }
}

bool PackageTab::execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                          const QString& settingsKey) noexcept {
  try {
    // Get current footprint.
    std::shared_ptr<const Footprint> footprint = mFsm->getCurrentFootprint();

    // Determine default file path.
    QString packageName =
        FilePath::cleanFileName(*mPackage->getNames().getDefaultValue(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    if ((mPackage->getFootprints().count() > 1) && (footprint)) {
      packageName += "_" % footprint->getNames().getDefaultValue();
    }
    FilePath defaultFilePath(QDir::homePath() % "/" % packageName %
                             "_Footprint");

    // Copy package items to allow processing them in worker threads.
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    if (footprint) {
      pages.append(std::make_shared<FootprintPainter>(*footprint));
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Board, output, pages, 0,
        *mPackage->getNames().getDefaultValue(), 0, defaultFilePath, mUnit,
        mApp.getWorkspace().getSettings().themes.getActive(),
        "package_editor/" % settingsKey, qApp->activeWindow());
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
}

void PackageTab::scheduleOpenGlSceneUpdate() noexcept {
  mOpenGlSceneRebuildScheduled = true;
  if (mOpenGlSceneRebuildTimer) {
    mOpenGlSceneRebuildTimer->start(100);
  }
}

void PackageTab::updateOpenGlScene() noexcept {
  if ((!mOpenGlSceneRebuildScheduled) || (!mView3d) || (!mOpenGlSceneBuilder) ||
      (mOpenGlSceneBuilder->isBusy())) {
    return;
  }

  if (mOpenGlSceneRebuildTimer) {
    mOpenGlSceneRebuildTimer->stop();
  }

  if (auto footprint = mFsm->getCurrentFootprint()) {
    std::shared_ptr<SceneData3D> data = std::make_shared<SceneData3D>(
        std::make_shared<TransactionalDirectory>(mPackage->getDirectory()),
        true);
    data->setSolderResist(&PcbColor::green());
    data->setSilkscreen(&PcbColor::white());
    data->setSilkscreenLayersTop(
        {&Layer::topLegend(), &Layer::topNames(), &Layer::topValues()});
    data->setSilkscreenLayersBot(
        {&Layer::botLegend(), &Layer::botNames(), &Layer::botValues()});
    for (const FootprintPad& pad : footprint->getPads()) {
      const Transform transform(pad.getPosition(), pad.getRotation(), false);
      auto geometries = pad.buildPreviewGeometries();
      for (auto it = geometries.begin(); it != geometries.end(); it++) {
        foreach (const PadGeometry& geometry, it.value()) {
          foreach (const Path& outline, geometry.toOutlines()) {
            data->addArea(*it.key(), outline, transform);
          }
          for (const PadHole& hole : geometry.getHoles()) {
            data->addHole(hole.getPath(), hole.getDiameter(), true, false,
                          transform);
          }
        }
      }
    }
    for (const Polygon& polygon : footprint->getPolygons()) {
      data->addPolygon(polygon, Transform());
    }
    for (const Circle& circle : footprint->getCircles()) {
      data->addCircle(circle, Transform());
    }
    for (const StrokeText& text : footprint->getStrokeTexts()) {
      data->addStroke(text.getLayer(),
                      text.generatePaths(Application::getDefaultStrokeFont()),
                      *text.getStrokeWidth(), Transform(text));
    }
    for (const Hole& hole : footprint->getHoles()) {
      data->addHole(hole.getPath(), hole.getDiameter(), false, false,
                    Transform());
      if (auto offset = hole.getPreviewStopMaskOffset()) {
        const Length width = hole.getDiameter() + (*offset) + (*offset);
        for (const Layer* layer :
             {&Layer::topStopMask(), &Layer::botStopMask()}) {
          data->addStroke(*layer, {*hole.getPath()}, width, Transform());
        }
      }
    }
    if (mCurrentModel) {
      data->addDevice(mPackage->getUuid(), Transform(),
                      mCurrentModel->getFileName(),
                      footprint->getModelPosition(),
                      footprint->getModelRotation(), QString());
    }
    mOpenGlSceneBuilder->start(data);
  } else {
    mOpenGlSceneBuilderErrors = {tr("Please select a footprint.")};
  }

  mOpenGlSceneRebuildScheduled = false;
  onDerivedUiDataChanged.notify();
}

bool PackageTab::toggleBackgroundImage() noexcept {
  if (mBackgroundImageGraphicsItem->isVisible()) {
    mBackgroundImageSettings.enabled = false;
  } else {
    // Show dialog.
    BackgroundImageSetupDialog dlg("package_editor", qApp->activeWindow());
    if (!mBackgroundImageSettings.image.isNull()) {
      dlg.setData(mBackgroundImageSettings.image,
                  mBackgroundImageSettings.rotation,
                  mBackgroundImageSettings.references);
    }
    if (dlg.exec() != QDialog::Accepted) {
      return mBackgroundImageGraphicsItem->isVisible();  // Aborted.
    }

    mBackgroundImageSettings.image = dlg.getImage();
    mBackgroundImageSettings.rotation = dlg.getRotation();
    mBackgroundImageSettings.references = dlg.getReferences();
    mBackgroundImageSettings.enabled =
        (!mBackgroundImageSettings.image.isNull()) &&
        (mBackgroundImageSettings.references.count() >= 2);
  }

  // Store & apply new settings.
  mBackgroundImageSettings.saveToDir(getBackgroundImageCacheDir());
  applyBackgroundImageSettings();
  return mBackgroundImageGraphicsItem->isVisible();
}

void PackageTab::applyBackgroundImageSettings() noexcept {
  BackgroundImageSettings& s = mBackgroundImageSettings;

  const bool enable = s.enabled && (!s.image.isNull());
  mBackgroundImageGraphicsItem->setVisible(enable);

  if (enable) {
    // Make the image background transparent.
    const Theme& theme =
        mEditor.getWorkspace().getSettings().themes.getActive();
    mBackgroundImageGraphicsItem->setPixmap(s.buildPixmap(
        theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor()));

    // Apply the transform.
    mBackgroundImageGraphicsItem->setTransform(s.calcTransform());
    if (s.references.count() >= 1) {
      mBackgroundImageGraphicsItem->setPos(
          s.references.first().second.toPxQPointF());
    }
  }
}

FilePath PackageTab::getBackgroundImageCacheDir() const noexcept {
  return Application::getCacheDir()
      .getPathTo("backgrounds")
      .getPathTo(mPackage->getUuid().toStr());
}

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

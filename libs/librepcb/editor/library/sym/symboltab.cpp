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
#include "symboltab.h"

#include "../../cmd/cmdtextedit.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdlibraryelementedit.h"
#include "../cmd/cmdsymbolpinedit.h"
#include "../libraryeditor.h"
#include "../libraryelementcategoriesmodel.h"
#include "fsm/symboleditorfsm.h"
#include "fsm/symboleditorstate_addnames.h"
#include "fsm/symboleditorstate_addpins.h"
#include "fsm/symboleditorstate_addvalues.h"
#include "fsm/symboleditorstate_drawarc.h"
#include "fsm/symboleditorstate_drawcircle.h"
#include "fsm/symboleditorstate_drawline.h"
#include "fsm/symboleditorstate_drawpolygon.h"
#include "fsm/symboleditorstate_drawrect.h"
#include "fsm/symboleditorstate_drawtext.h"
#include "graphics/graphicslayerlist.h"
#include "symbolgraphicsitem.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/library/sym/symbolcheckmessages.h>
#include <librepcb/core/library/sym/symbolpainter.h>
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

SymbolTab::SymbolTab(LibraryEditor& editor, std::unique_ptr<Symbol> sym,
                     Mode mode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mSymbol(std::move(sym)),
    mLayers(GraphicsLayerList::libraryLayers(
        &mEditor.getWorkspace().getSettings())),
    mView(new SlintGraphicsView(SlintGraphicsView::defaultSymbolSceneRect(),
                                this)),
    mIsNewElement(isPathOutsideLibDir()),
    mMsgImportPins(mApp.getWorkspace(), "EMPTY_SYMBOL_IMPORT_PINS"),
    mWizardMode(mode != Mode::Open),
    mCurrentPageIndex(mWizardMode ? 0 : 1),
    mGridStyle(mApp.getWorkspace()
                   .getSettings()
                   .themes.getActive()
                   .getSchematicGridStyle()),
    mGridInterval(2540000),
    mUnit(LengthUnit::millimeters()),
    mChooseCategory(false),
    mCompactLayout(false),
    mFrameIndex(0),
    mNameParsed(mSymbol->getNames().getDefaultValue()),
    mVersionParsed(mSymbol->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(new CategoryTreeModel(editor.getWorkspace().getLibraryDb(),
                                          editor.getWorkspace().getSettings(),
                                          CategoryTreeModel::Filter::CmpCat)),
    mToolFeatures(),
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
    mIsInterfaceBroken(false),
    mOriginalSymbolPinUuids(mSymbol->getPins().getUuidSet()) {
  // Setup graphics view.
  mView->setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
  mView->setEventHandler(this);
  connect(
      &mApp.getWorkspace().getSettings().useOpenGl,
      &WorkspaceSettingsItem_GenericValue<bool>::edited, this, [this]() {
        mView->setUseOpenGl(mApp.getWorkspace().getSettings().useOpenGl.get());
      });
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &SymbolTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          &SymbolTab::notifyDerivedUiDataChanged);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &SymbolTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &SymbolTab::refreshUiData);

  // Connect models.
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &SymbolTab::commitUiData, Qt::QueuedConnection);

  // Setup messages.
  connect(&mMsgImportPins, &DismissableMessageContext::visibilityChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });

  // Load finite state machine (FSM).
  SymbolEditorFsm::Context fsmContext{*mSymbol, *mUndoStack, !isWritable(),
                                      mUnit, *this};
  mFsm.reset(new SymbolEditorFsm(fsmContext));

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

SymbolTab::~SymbolTab() noexcept {
  deactivate();

  // Clean up the state machine nicely to avoid unexpected behavior. Triggering
  // abort (Esc) two times is usually sufficient to leave any active tool, so
  // let's call it three times to be on the safe side. Unfortunately there's
  // no clean way to forcible and guaranteed leaving a tool.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm.reset();

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

FilePath SymbolTab::getDirectoryPath() const noexcept {
  return mSymbol->getDirectory().getAbsPath();
}

ui::TabData SymbolTab::getUiData() const noexcept {
  const bool writable = isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());
  if ((!mWizardMode) && ((!mCompactLayout) || (mCurrentPageIndex == 1))) {
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
  }

  return ui::TabData{
      ui::TabType::Symbol,  // Type
      q2s(*mSymbol->getNames().getDefaultValue()),  // Title
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

ui::SymbolTabData SymbolTab::getDerivedUiData() const noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;

  return ui::SymbolTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mSymbol->getDirectory().getAbsPath().toStr()),  // Path
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
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      l2s(*mGridInterval),  // Grid interval
      l2s(mUnit),  // Unit
      mIsInterfaceBroken,  // Interface broken
      mMsgImportPins.getUiData(),  // Message "import pins"
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
      mCompactLayout,  // Compact layout
      q2s(mSceneImagePos),  // Scene image position
      mFrameIndex,  // Frame index
      slint::SharedString(),  // New category
  };
}

void SymbolTab::setDerivedUiData(const ui::SymbolTabData& data) noexcept {
  // General
  if (data.page_index != mCurrentPageIndex) {
    mCurrentPageIndex = data.page_index;
    onUiDataChanged.notify();
  }
  if (data.compact_layout != mCompactLayout) {
    mCompactLayout = data.compact_layout;
    onUiDataChanged.notify();
  }
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
  mChooseCategory = data.choose_category;

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
  mMsgImportPins.setUiData(data.import_pins_msg);

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
  emit filledRequested(data.tool_filled);
  emit grabAreaRequested(data.tool_grab_area);
  mToolLineWidth.setUiData(data.tool_line_width);
  mToolSize.setUiData(data.tool_size);
  emit valueRequested(EditorToolbox::toMultiLine(s2q(data.tool_value.text)));
  emit hAlignRequested(s2l(data.tool_halign));
  emit vAlignRequested(s2l(data.tool_valign));

  requestRepaint();
}

void SymbolTab::activate() noexcept {
  mScene.reset(new GraphicsScene(this));
  mScene->setGridInterval(mGridInterval);
  connect(mScene.get(), &GraphicsScene::changed, this,
          &SymbolTab::requestRepaint);

  mGraphicsItem.reset(new SymbolGraphicsItem(*mSymbol, *mLayers));
  mScene->addItem(*mGraphicsItem);

  applyTheme();
  requestRepaint();
}

void SymbolTab::deactivate() noexcept {
  mGraphicsItem.reset();
  mScene.reset();
}

void SymbolTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Next: {
      if (mWizardMode) {
        mWizardMode = false;
        mCurrentPageIndex = 1;
        scheduleChecks();
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
    case ui::TabAction::SymbolImportPins: {
      mFsm->processStartAddingSymbolPins(true);
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
    case ui::TabAction::ToolPin: {
      mFsm->processStartAddingSymbolPins(false);
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

slint::Image SymbolTab::renderScene(float width, float height,
                                    int scene) noexcept {
  Q_UNUSED(scene);
  if (mScene) {
    return mView->render(*mScene, width, height);
  }
  return slint::Image();
}

bool SymbolTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  return mView->pointerEvent(pos, e);
}

bool SymbolTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView->scrollEvent(pos, e);
}

bool SymbolTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  return mView->keyEvent(e);
}

bool SymbolTab::requestClose() noexcept {
  commitUiData();

  if ((!hasUnsavedChanges()) || (!isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The symbol '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mSymbol->getNames().getDefaultValue()),
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

bool SymbolTab::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyPressed(e);
}

bool SymbolTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool SymbolTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  emit cursorCoordinatesChanged(e.scenePos, mUnit);
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool SymbolTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  SymbolEditorFsmAdapter
 ******************************************************************************/

GraphicsScene* SymbolTab::fsmGetGraphicsScene() noexcept {
  return mScene.get();
}

SymbolGraphicsItem* SymbolTab::fsmGetGraphicsItem() noexcept {
  return mGraphicsItem.get();
}

PositiveLength SymbolTab::fsmGetGridInterval() const noexcept {
  return mGridInterval;
}

void SymbolTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mToolCursorShape = *shape;
  } else {
    mToolCursorShape = Qt::ArrowCursor;
  }
  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmSetViewGrayOut(bool grayOut) noexcept {
  if (mScene) {
    mScene->setGrayOut(grayOut);
  }
}

void SymbolTab::fsmSetViewInfoBoxText(const QString& text) noexcept {
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

void SymbolTab::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  if (mScene) {
    mScene->setRulerPositions(pos);
  }
}

void SymbolTab::fsmSetSceneCursor(const Point& pos, bool cross,
                                  bool circle) noexcept {
  if (mScene) {
    mScene->setSceneCursor(pos, cross, circle);
  }
}

QPainterPath SymbolTab::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return mView->calcPosWithTolerance(pos, multiplier);
}

Point SymbolTab::fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mView->mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void SymbolTab::fsmSetStatusBarMessage(const QString& message,
                                       int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void SymbolTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    onUiDataChanged.notify();
  }
}

void SymbolTab::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mTool = ui::EditorTool::Select;
  fsmSetFeatures(Features());
  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_Select& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Select;
  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawLine& state) noexcept {
  mTool = ui::EditorTool::Line;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawLine::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SymbolTab::layerRequested, &state,
                                      &SymbolEditorState_DrawLine::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "symbol_editor/draw_line/line_width");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawLine::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_DrawLine::setLineWidth));

  // Angle
  auto setAngle = [this](const Angle& angle) {
    mToolAngle = angle;
    onDerivedUiDataChanged.notify();
  };
  setAngle(state.getAngle());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawLine::angleChanged, this, setAngle));
  mFsmStateConnections.append(connect(this, &SymbolTab::angleRequested, &state,
                                      &SymbolEditorState_DrawLine::setAngle));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawRect& state) noexcept {
  mTool = ui::EditorTool::Rect;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawRect::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SymbolTab::layerRequested, &state,
                                      &SymbolEditorState_DrawRect::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "symbol_editor/draw_rect/line_width");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawRect::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_DrawRect::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawRect::filledChanged, this, setFilled));
  mFsmStateConnections.append(connect(this, &SymbolTab::filledRequested, &state,
                                      &SymbolEditorState_DrawRect::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawRect::grabAreaChanged, this, setGrabArea));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::grabAreaRequested, &state,
              &SymbolEditorState_DrawRect::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawPolygon& state) noexcept {
  mTool = ui::EditorTool::Polygon;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawPolygon::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::layerRequested, &state,
              &SymbolEditorState_DrawPolygon::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "symbol_editor/draw_polygon/line_width");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_DrawPolygon::setLineWidth));

  // Angle
  auto setAngle = [this](const Angle& angle) {
    mToolAngle = angle;
    onDerivedUiDataChanged.notify();
  };
  setAngle(state.getAngle());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawPolygon::angleChanged, this, setAngle));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::angleRequested, &state,
              &SymbolEditorState_DrawPolygon::setAngle));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawPolygon::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::filledRequested, &state,
              &SymbolEditorState_DrawPolygon::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::grabAreaChanged, this,
              setGrabArea));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::grabAreaRequested, &state,
              &SymbolEditorState_DrawPolygon::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawCircle& state) noexcept {
  mTool = ui::EditorTool::Circle;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawCircle::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SymbolTab::layerRequested, &state,
                                      &SymbolEditorState_DrawCircle::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "symbol_editor/draw_circle/line_width");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_DrawCircle::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawCircle::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::filledRequested, &state,
              &SymbolEditorState_DrawCircle::setFilled));

  // Grab area
  auto setGrabArea = [this](bool grabArea) {
    mToolGrabArea = grabArea;
    onDerivedUiDataChanged.notify();
  };
  setGrabArea(state.getGrabArea());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::grabAreaChanged, this,
              setGrabArea));
  mFsmStateConnections.append(
      connect(this, &SymbolTab::grabAreaRequested, &state,
              &SymbolEditorState_DrawCircle::setGrabArea));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawArc& state) noexcept {
  mTool = ui::EditorTool::Arc;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawArc::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SymbolTab::layerRequested, &state,
                                      &SymbolEditorState_DrawArc::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "symbol_editor/draw_arc/line_width");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawArc::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_DrawArc::setLineWidth));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_AddNames& state) noexcept {
  mTool = ui::EditorTool::Name;

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "symbol_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &SymbolEditorState_DrawText::setHeight));

  // Horizontal alignment
  auto setHAlign = [this](const HAlign& align) {
    mToolAlign.setH(align);
    onDerivedUiDataChanged.notify();
  };
  setHAlign(state.getHAlign());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::hAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::vAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_AddValues& state) noexcept {
  mTool = ui::EditorTool::Value;

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "symbol_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &SymbolEditorState_DrawText::setHeight));

  // Horizontal alignment
  auto setHAlign = [this](const HAlign& align) {
    mToolAlign.setH(align);
    onDerivedUiDataChanged.notify();
  };
  setHAlign(state.getHAlign());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::hAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::vAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_DrawText& state) noexcept {
  mTool = ui::EditorTool::Text;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
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
      &state, &SymbolEditorState_DrawText::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SymbolTab::layerRequested, &state,
                                      &SymbolEditorState_DrawText::setLayer));

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "symbol_editor/draw_text/height");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &SymbolEditorState_DrawText::setHeight));

  // Text
  auto setText = [this](const QString& text) {
    mToolValue = text;
    onDerivedUiDataChanged.notify();
  };
  setText(state.getText());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::textChanged, this, setText));
  mFsmStateConnections.append(connect(this, &SymbolTab::valueRequested, &state,
                                      &SymbolEditorState_DrawText::setText));

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
      &state, &SymbolEditorState_DrawText::hAlignChanged, this, setHAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::hAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setHAlign));

  // Vertical alignment
  auto setVAlign = [this](const VAlign& align) {
    mToolAlign.setV(align);
    onDerivedUiDataChanged.notify();
  };
  setVAlign(state.getVAlign());
  mFsmStateConnections.append(connect(
      &state, &SymbolEditorState_DrawText::vAlignChanged, this, setVAlign));
  mFsmStateConnections.append(connect(this, &SymbolTab::vAlignRequested, &state,
                                      &SymbolEditorState_DrawText::setVAlign));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_AddPins& state) noexcept {
  mTool = ui::EditorTool::Pin;

  // Name
  auto setName = [this](const CircuitIdentifier& name) {
    mToolValue = *name;
    onDerivedUiDataChanged.notify();
  };
  setName(state.getName());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_AddPins::nameChanged, this, setName));
  mFsmStateConnections.append(connect(
      this, &SymbolTab::valueRequested, &state, [&state](const QString& text) {
        if (auto name = parseCircuitIdentifier(cleanCircuitIdentifier(text))) {
          state.setName(*name);
        }
      }));

  // Length
  mToolSize.configure(state.getLength(), LengthEditContext::Steps::pinLength(),
                      "symbol_editor/add_pins/length");
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_AddPins::lengthChanged, &mToolSize,
              &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedUnsigned, &state,
              &SymbolEditorState_AddPins::setLength));

  onDerivedUiDataChanged.notify();
}

void SymbolTab::fsmToolEnter(SymbolEditorState_Measure& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Measure;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
    SymbolTab::runChecksImpl() {
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

  return std::make_pair(mSymbol->runChecks(), mSymbol->getMessageApprovals());
}

bool SymbolTab::autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                            bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingCategories>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingSymbolName>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingSymbolValue>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgWrongSymbolTextLayer>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgSymbolPinNotOnGrid>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgNonFunctionalSymbolPinInversionSign>(msg, checkOnly))
    return true;
  if (autoFixHelper<MsgSymbolOriginNotInCenter>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool SymbolTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

void SymbolTab::messageApprovalChanged(const SExpression& approval,
                                       bool approved) noexcept {
  if (mSymbol->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void SymbolTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Rule check autofixes
 ******************************************************************************/

template <>
void SymbolTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitUiData();
}

template <>
void SymbolTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitUiData();
}

template <>
void SymbolTab::autoFix(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCurrentPageIndex = 0;
  mChooseCategory = true;
  onDerivedUiDataChanged.notify();
}

template <>
void SymbolTab::autoFix(const MsgMissingSymbolName& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingNames();
}

template <>
void SymbolTab::autoFix(const MsgMissingSymbolValue& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingValues();
}

template <>
void SymbolTab::autoFix(const MsgWrongSymbolTextLayer& msg) {
  std::shared_ptr<Text> text = mSymbol->getTexts().get(msg.getText().get());
  std::unique_ptr<CmdTextEdit> cmd(new CmdTextEdit(*text));
  cmd->setLayer(msg.getExpectedLayer(), false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolTab::autoFix(const MsgSymbolPinNotOnGrid& msg) {
  std::shared_ptr<SymbolPin> pin = mSymbol->getPins().get(msg.getPin().get());
  Point newPos = pin->getPosition().mappedToGrid(msg.getGridInterval());
  std::unique_ptr<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(pin));
  cmd->setPosition(newPos, false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolTab::autoFix(const MsgNonFunctionalSymbolPinInversionSign& msg) {
  std::shared_ptr<SymbolPin> pin = mSymbol->getPins().get(msg.getPin().get());
  std::unique_ptr<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(pin));
  cmd->setName(CircuitIdentifier("!" % pin->getName()->mid(1)), false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolTab::autoFix(const MsgSymbolOriginNotInCenter& msg) {
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processSelectAll();
  mFsm->processMove(-msg.getCenter().mappedToGrid(mGridInterval));
  mFsm->processAbortCommand();  // Clear selection.
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolTab::isWritable() const noexcept {
  return mIsNewElement || mSymbol->getDirectory().isWritable();
}

void SymbolTab::refreshUiData() noexcept {
  mName = q2s(*mSymbol->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mSymbol->getNames().getDefaultValue();
  mDescription = q2s(mSymbol->getDescriptions().getDefaultValue());
  mKeywords = q2s(mSymbol->getKeywords().getDefaultValue());
  mAuthor = q2s(mSymbol->getAuthor());
  mVersion = q2s(mSymbol->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mSymbol->getVersion();
  mDeprecated = mSymbol->isDeprecated();
  mCategories->setCategories(mSymbol->getCategories());

  mMsgImportPins.setActive(mSymbol->isEmpty());

  // Update "interface broken" only when no command is active since it would
  // be annoying to get it during intermediate states.
  if (!mUndoStack->isCommandGroupActive()) {
    mIsInterfaceBroken = (!mIsNewElement) && (!mWizardMode) &&
        (mSymbol->getPins().getUuidSet() != mOriginalSymbolPinUuids);
  }

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void SymbolTab::commitUiData() noexcept {
  // Abort any active command as this would block the undo stack.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();

  try {
    std::unique_ptr<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mSymbol, tr("Edit Symbol Properties")));
    cmd->setName(QString(), mNameParsed);
    const QString description = s2q(mDescription);
    if (description != mSymbol->getDescriptions().getDefaultValue()) {
      cmd->setDescription(QString(), description.trimmed());
    }
    const QString keywords = s2q(mKeywords);
    if (keywords != mSymbol->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    const QString author = s2q(mAuthor);
    if (author != mSymbol->getAuthor()) {
      cmd->setAuthor(author.trimmed());
    }
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool SymbolTab::save() noexcept {
  try {
    // Remove obsolete message approvals (bypassing the undo stack). Since
    // the checks are run asynchronously, the approvals may be outdated, so
    // we first run the checks once synchronosuly.
    runChecks();
    mSymbol->setMessageApprovals(mSymbol->getMessageApprovals() -
                                 mDisappearedApprovals);

    mSymbol->save();
    if (isPathOutsideLibDir()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Symbol>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mSymbol->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mSymbol->saveTo(dir);
    }
    mSymbol->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mManualModificationsMade = false;
    mOriginalSymbolPinUuids = mSymbol->getPins().getUuidSet();
    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    if (mWizardMode && (mCurrentPageIndex == 0)) {
      ++mCurrentPageIndex;
      mWizardMode = false;
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

void SymbolTab::setGridInterval(const PositiveLength& interval) noexcept {
  if (interval != mGridInterval) {
    mGridInterval = interval;
    mFsm->processGridIntervalChanged(mGridInterval);
    if (mScene) {
      mScene->setGridInterval(mGridInterval);
      requestRepaint();
    }
  }
}

bool SymbolTab::execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                         const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    QString symbolName =
        FilePath::cleanFileName(*mSymbol->getNames().getDefaultValue(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    FilePath defaultFilePath(QDir::homePath() % "/" % symbolName % "_Symbol");

    // Copy symbol items to allow processing them in worker threads.
    QList<std::shared_ptr<GraphicsPagePainter>> pages = {
        std::make_shared<SymbolPainter>(*mSymbol),
    };

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Schematic, output, pages, 0,
        *mSymbol->getNames().getDefaultValue(), 0, defaultFilePath, mUnit,
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
}

void SymbolTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

void SymbolTab::applyTheme() noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();

  if (mScene) {
    mScene->setBackgroundColors(
        theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
    mScene->setOverlayColors(
        theme.getColor(Theme::Color::sSchematicOverlays).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicOverlays).getSecondaryColor());
    mScene->setSelectionRectColors(
        theme.getColor(Theme::Color::sSchematicSelection).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicSelection).getSecondaryColor());
    mScene->setGridStyle(mGridStyle);
  }

  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

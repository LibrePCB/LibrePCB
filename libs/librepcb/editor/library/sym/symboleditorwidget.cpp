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
#include "symboleditorwidget.h"

#include "../../cmd/cmdtextedit.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../editorcommandset.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/graphicsscene.h"
#include "../../library/cmd/cmdlibraryelementedit.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/halignactiongroup.h"
#include "../../utils/toolbarproxy.h"
#include "../../utils/valignactiongroup.h"
#include "../../widgets/angleedit.h"
#include "../../widgets/layercombobox.h"
#include "../../widgets/positivelengthedit.h"
#include "../../widgets/statusbar.h"
#include "../../widgets/unsignedlengthedit.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdsymbolpinedit.h"
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
#include "symbolgraphicsitem.h"
#include "ui_symboleditorwidget.h"

#include <librepcb/core/library/cmp/cmpsigpindisplaytype.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/library/sym/symbolcheckmessages.h>
#include <librepcb/core/library/sym/symbolpainter.h>
#include <librepcb/core/workspace/workspace.h>
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

SymbolEditorWidget::SymbolEditorWidget(const Context& context,
                                       const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::SymbolEditorWidget),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setReadOnly(mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  setWindowIcon(QIcon(":/img/library/symbol.png"));

  // Setup graphics scene.
  const Theme& theme = mContext.workspace.getSettings().themes.getActive();
  mGraphicsScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mGraphicsScene->setOverlayColors(
      theme.getColor(Theme::Color::sSchematicOverlays).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicOverlays).getSecondaryColor());
  mGraphicsScene->setSelectionRectColors(
      theme.getColor(Theme::Color::sSchematicSelection).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicSelection).getSecondaryColor());
  mGraphicsScene->setGridStyle(theme.getBoardGridStyle());

  // Setup graphics view.
  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->graphicsView->setInfoBoxColors(
      theme.getColor(Theme::Color::sSchematicInfoBox).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicInfoBox).getSecondaryColor());
  mUi->graphicsView->setUseOpenGl(
      mContext.workspace.getSettings().useOpenGl.get());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  mUi->graphicsView->addAction(
      EditorCommandSet::instance().commandToolBarFocus.createAction(
          this, this,
          [this]() {
            mCommandToolBarProxy->startTabFocusCycle(*mUi->graphicsView);
          },
          EditorCommand::ActionFlag::WidgetShortcut));

  // Apply grid properties unit from workspace settings
  setGridProperties(PositiveLength(2540000),
                    mContext.workspace.getSettings().defaultLengthUnit.get(),
                    theme.getBoardGridStyle());

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(new CategoryListEditorWidget(
      mContext.workspace, CategoryListEditorWidget::Categories::Component,
      this));
  mCategoriesEditorWidget->setReadOnly(mContext.readOnly);
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mSymbol = Symbol::open(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem)));  // can throw
  updateMetadata();

  // Show "interface broken" warning when related properties are modified.
  mOriginalSymbolPinUuids = mSymbol->getPins().getUuidSet();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &SymbolEditorWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &SymbolEditorWidget::commitMetadata);
  connect(mCategoriesEditorWidget.data(), &CategoryListEditorWidget::edited,
          this, &SymbolEditorWidget::commitMetadata);

  // Load graphics items recursively.
  mGraphicsItem.reset(new SymbolGraphicsItem(*mSymbol, mContext.layers));
  mGraphicsScene->addItem(*mGraphicsItem);
  mUi->graphicsView->zoomAll();

  // Load finite state machine (FSM).
  SymbolEditorFsm::Context fsmContext{
      *mSymbol, *mUndoStack, mContext.readOnly, mLengthUnit, *this,
  };
  mFsm.reset(new SymbolEditorFsm(fsmContext));

  // Last but not least, connect the graphics scene events with the FSM.
  mUi->graphicsView->setEventHandlerObject(this);
}

SymbolEditorWidget::~SymbolEditorWidget() noexcept {
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
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature> SymbolEditorWidget::getAvailableFeatures()
    const noexcept {
  return mFeatures;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SymbolEditorWidget::connectEditor(
    UndoStackActionGroup& undoStackActionGroup,
    ExclusiveActionGroup& toolsActionGroup, QToolBar& commandToolBar,
    StatusBar& statusBar) noexcept {
  EditorWidgetBase::connectEditor(undoStackActionGroup, toolsActionGroup,
                                  commandToolBar, statusBar);

  bool enabled = !mContext.readOnly;
  mToolsActionGroup->setActionEnabled(Tool::SELECT, true);
  mToolsActionGroup->setActionEnabled(Tool::ADD_PINS, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_NAMES, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_VALUES, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_LINE, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_RECT, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_POLYGON, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_CIRCLE, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_ARC, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_TEXT, enabled);
  mToolsActionGroup->setActionEnabled(Tool::MEASURE, true);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentTool());

  mStatusBar->setField(StatusBar::AbsolutePosition, true);
  mStatusBar->setLengthUnit(mLengthUnit);
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mStatusBar, &StatusBar::setAbsoluteCursorPosition);
}

void SymbolEditorWidget::disconnectEditor() noexcept {
  mStatusBar->setField(StatusBar::AbsolutePosition, false);
  disconnect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
             mStatusBar, &StatusBar::setAbsoluteCursorPosition);

  EditorWidgetBase::disconnectEditor();
}

/*******************************************************************************
 *  SymbolEditorFsmAdapter
 ******************************************************************************/

GraphicsScene* SymbolEditorWidget::fsmGetGraphicsScene() noexcept {
  return mGraphicsScene.data();
}

SymbolGraphicsItem* SymbolEditorWidget::fsmGetGraphicsItem() noexcept {
  return mGraphicsItem.data();
}

PositiveLength SymbolEditorWidget::fsmGetGridInterval() const noexcept {
  return mGraphicsScene->getGridInterval();
}

void SymbolEditorWidget::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mUi->graphicsView->setCursor(*shape);
  } else {
    mUi->graphicsView->unsetCursor();
  }
}

void SymbolEditorWidget::fsmSetViewGrayOut(bool grayOut) noexcept {
  mGraphicsScene->setGrayOut(grayOut);
}

void SymbolEditorWidget::fsmSetViewInfoBoxText(const QString& text) noexcept {
  mUi->graphicsView->setInfoBoxText(text);
}

void SymbolEditorWidget::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  mGraphicsScene->setRulerPositions(pos);
}

void SymbolEditorWidget::fsmSetSceneCursor(const Point& pos, bool cross,
                                           bool circle) noexcept {
  mGraphicsScene->setSceneCursor(pos, cross, circle);
}

QPainterPath SymbolEditorWidget::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return mUi->graphicsView->calcPosWithTolerance(pos, multiplier);
}

Point SymbolEditorWidget::fsmMapGlobalPosToScenePos(
    const QPoint& pos) const noexcept {
  return mUi->graphicsView->mapGlobalPosToScenePos(pos);
}

void SymbolEditorWidget::fsmSetStatusBarMessage(const QString& message,
                                                int timeoutMs) noexcept {
  setStatusBarMessage(message, timeoutMs);
}

void SymbolEditorWidget::fsmSetFeatures(Features features) noexcept {
  QSet<EditorWidgetBase::Feature> editorFeatures = {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Close,
      EditorWidgetBase::Feature::GraphicsView,
      EditorWidgetBase::Feature::ExportGraphics,
  };
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Select)) {
    editorFeatures |= EditorWidgetBase::Feature::SelectGraphics;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Cut)) {
    editorFeatures |= EditorWidgetBase::Feature::Cut;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Copy)) {
    editorFeatures |= EditorWidgetBase::Feature::Copy;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Paste)) {
    editorFeatures |= EditorWidgetBase::Feature::Paste;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Remove)) {
    editorFeatures |= EditorWidgetBase::Feature::Remove;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Rotate)) {
    editorFeatures |= EditorWidgetBase::Feature::Rotate;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Mirror)) {
    editorFeatures |= EditorWidgetBase::Feature::Mirror;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::SnapToGrid)) {
    editorFeatures |= EditorWidgetBase::Feature::SnapToGrid;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::Properties)) {
    editorFeatures |= EditorWidgetBase::Feature::Properties;
  }
  if (features.testFlag(SymbolEditorFsmAdapter::Feature::ImportGraphics)) {
    editorFeatures |= EditorWidgetBase::Feature::ImportGraphics;
  }

  if (editorFeatures != mFeatures) {
    mFeatures = editorFeatures;
    emit availableFeaturesChanged(mFeatures);
  }
}

void SymbolEditorWidget::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mCommandToolBarProxy->clear();
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(-1);
  fsmSetFeatures(Features());
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_Select& state) noexcept {
  Q_UNUSED(state);
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::SELECT);
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawLine& state) noexcept {
  if ((!mToolsActionGroup) || (!mCommandToolBarProxy)) return;

  mToolsActionGroup->setCurrentAction(Tool::DRAW_LINE);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawLine::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawLine::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Line width
  mCommandToolBarProxy->addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                          "symbol_editor/draw_polygon/line_width");
  edtLineWidth->setValue(state.getLineWidth());
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawLine::lineWidthChanged,
              edtLineWidth.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(
      connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, &state,
              &SymbolEditorState_DrawLine::setLineWidth));
  mCommandToolBarProxy->addWidget(std::move(edtLineWidth));

  // Arc angle
  mCommandToolBarProxy->addLabel(tr("Arc Angle:"), 10);
  std::unique_ptr<AngleEdit> edtAngle(new AngleEdit());
  edtAngle->setSingleStep(90.0);  // [°]
  edtAngle->setValue(state.getAngle());
  mFsmStateConnections.append(connect(&state,
                                      &SymbolEditorState_DrawLine::angleChanged,
                                      edtAngle.get(), &AngleEdit::setValue));
  mFsmStateConnections.append(connect(edtAngle.get(), &AngleEdit::valueChanged,
                                      &state,
                                      &SymbolEditorState_DrawLine::setAngle));
  mCommandToolBarProxy->addWidget(std::move(edtAngle));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawRect& state) noexcept {
  if ((!mToolsActionGroup) || (!mCommandToolBarProxy)) return;

  mToolsActionGroup->setCurrentAction(Tool::DRAW_RECT);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawRect::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawRect::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Line width
  mCommandToolBarProxy->addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                          "symbol_editor/draw_polygon/line_width");
  edtLineWidth->setValue(state.getLineWidth());
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawRect::lineWidthChanged,
              edtLineWidth.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(
      connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, &state,
              &SymbolEditorState_DrawRect::setLineWidth));
  mCommandToolBarProxy->addWidget(std::move(edtLineWidth));

  // Fill
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
  fillCheckBox->setChecked(state.getFilled());
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  QString toolTip = tr("Fill polygon, if closed");
  if (!cmd.fillToggle.getKeySequences().isEmpty()) {
    toolTip += " (" %
        cmd.fillToggle.getKeySequences().first().toString(
            QKeySequence::NativeText) %
        ")";
  }
  fillCheckBox->setToolTip(toolTip);
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawRect::filledChanged,
              fillCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(connect(fillCheckBox.get(), &QCheckBox::toggled,
                                      &state,
                                      &SymbolEditorState_DrawRect::setFilled));
  mCommandToolBarProxy->addWidget(std::move(fillCheckBox), 10);

  // Grab area
  std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
  grabAreaCheckBox->setChecked(state.getGrabArea());
  grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
      grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
  toolTip = tr("Use polygon as grab area");
  if (!cmd.grabAreaToggle.getKeySequences().isEmpty()) {
    toolTip += " (" %
        cmd.grabAreaToggle.getKeySequences().first().toString(
            QKeySequence::NativeText) %
        ")";
  }
  grabAreaCheckBox->setToolTip(toolTip);
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawRect::grabAreaChanged,
              grabAreaCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(
      connect(grabAreaCheckBox.get(), &QCheckBox::toggled, &state,
              &SymbolEditorState_DrawRect::setGrabArea));
  mCommandToolBarProxy->addWidget(std::move(grabAreaCheckBox));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawPolygon& state) noexcept {
  if ((!mToolsActionGroup) || (!mCommandToolBarProxy)) return;

  mToolsActionGroup->setCurrentAction(Tool::DRAW_POLYGON);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawPolygon::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Line width
  mCommandToolBarProxy->addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                          "symbol_editor/draw_polygon/line_width");
  edtLineWidth->setValue(state.getLineWidth());
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::lineWidthChanged,
              edtLineWidth.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(
      connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, &state,
              &SymbolEditorState_DrawPolygon::setLineWidth));
  mCommandToolBarProxy->addWidget(std::move(edtLineWidth));

  // Arc angle
  mCommandToolBarProxy->addLabel(tr("Arc Angle:"), 10);
  std::unique_ptr<AngleEdit> edtAngle(new AngleEdit());
  edtAngle->setSingleStep(90.0);  // [°]
  edtAngle->setValue(state.getAngle());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::angleChanged,
              edtAngle.get(), &AngleEdit::setValue));
  mFsmStateConnections.append(
      connect(edtAngle.get(), &AngleEdit::valueChanged, &state,
              &SymbolEditorState_DrawPolygon::setAngle));
  mCommandToolBarProxy->addWidget(std::move(edtAngle));

  // Fill
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
  fillCheckBox->setChecked(state.getFilled());
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  QString toolTip = tr("Fill polygon, if closed");
  if (!cmd.fillToggle.getKeySequences().isEmpty()) {
    toolTip += " (" %
        cmd.fillToggle.getKeySequences().first().toString(
            QKeySequence::NativeText) %
        ")";
  }
  fillCheckBox->setToolTip(toolTip);
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::filledChanged,
              fillCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(
      connect(fillCheckBox.get(), &QCheckBox::toggled, &state,
              &SymbolEditorState_DrawPolygon::setFilled));
  mCommandToolBarProxy->addWidget(std::move(fillCheckBox), 10);

  // Grab area
  std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
  grabAreaCheckBox->setChecked(state.getGrabArea());
  grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
      grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
  toolTip = tr("Use polygon as grab area");
  if (!cmd.grabAreaToggle.getKeySequences().isEmpty()) {
    toolTip += " (" %
        cmd.grabAreaToggle.getKeySequences().first().toString(
            QKeySequence::NativeText) %
        ")";
  }
  grabAreaCheckBox->setToolTip(toolTip);
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawPolygon::grabAreaChanged,
              grabAreaCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(
      connect(grabAreaCheckBox.get(), &QCheckBox::toggled, &state,
              &SymbolEditorState_DrawPolygon::setGrabArea));
  mCommandToolBarProxy->addWidget(std::move(grabAreaCheckBox));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawCircle& state) noexcept {
  Q_UNUSED(state);
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::DRAW_CIRCLE);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawCircle::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Line width
  mCommandToolBarProxy->addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                          "symbol_editor/draw_circle/line_width");
  edtLineWidth->setValue(state.getLineWidth());
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::lineWidthChanged,
              edtLineWidth.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(
      connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, &state,
              &SymbolEditorState_DrawCircle::setLineWidth));
  mCommandToolBarProxy->addWidget(std::move(edtLineWidth));

  // Fill
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
  fillCheckBox->setChecked(state.getFilled());
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::filledChanged,
              fillCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(
      connect(fillCheckBox.get(), &QCheckBox::toggled, &state,
              &SymbolEditorState_DrawCircle::setFilled));
  mCommandToolBarProxy->addWidget(std::move(fillCheckBox), 10);

  // Grab area
  std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
  grabAreaCheckBox->setChecked(state.getGrabArea());
  grabAreaCheckBox->addAction(cmd.grabAreaToggle.createAction(
      grabAreaCheckBox.get(), grabAreaCheckBox.get(), &QCheckBox::toggle));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawCircle::grabAreaChanged,
              grabAreaCheckBox.get(), &QCheckBox::setChecked));
  mFsmStateConnections.append(
      connect(grabAreaCheckBox.get(), &QCheckBox::toggled, &state,
              &SymbolEditorState_DrawCircle::setGrabArea));
  mCommandToolBarProxy->addWidget(std::move(grabAreaCheckBox));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawArc& state) noexcept {
  if ((!mToolsActionGroup) || (!mCommandToolBarProxy)) return;

  mToolsActionGroup->setCurrentAction(Tool::DRAW_ARC);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawArc::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawArc::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Line width
  mCommandToolBarProxy->addLabel(tr("Line Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLineWidth(new UnsignedLengthEdit());
  edtLineWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                          "symbol_editor/draw_polygon/line_width");
  edtLineWidth->setValue(state.getLineWidth());
  edtLineWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepUp));
  edtLineWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtLineWidth.get(), edtLineWidth.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawArc::lineWidthChanged,
              edtLineWidth.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(
      connect(edtLineWidth.get(), &UnsignedLengthEdit::valueChanged, &state,
              &SymbolEditorState_DrawArc::setLineWidth));
  mCommandToolBarProxy->addWidget(std::move(edtLineWidth));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_AddNames& state) noexcept {
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::ADD_NAMES);

  // Height
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(mLengthUnit, LengthEditBase::Steps::textHeight(),
                       "symbol_editor/draw_text/height");
  edtHeight->setValue(state.getHeight());
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged,
              edtHeight.get(), &PositiveLengthEdit::setValue));
  mFsmStateConnections.append(connect(edtHeight.get(),
                                      &PositiveLengthEdit::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHeight));
  mCommandToolBarProxy->addWidget(std::move(edtHeight));

  // Horizontal alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  hAlignActionGroup->setValue(state.getHAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::hAlignChanged,
              hAlignActionGroup.get(), &HAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(hAlignActionGroup.get(),
                                      &HAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHAlign));
  mCommandToolBarProxy->addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  vAlignActionGroup->setValue(state.getVAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::vAlignChanged,
              vAlignActionGroup.get(), &VAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(vAlignActionGroup.get(),
                                      &VAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setVAlign));
  mCommandToolBarProxy->addActionGroup(std::move(vAlignActionGroup));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_AddValues& state) noexcept {
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::ADD_VALUES);

  // Height
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(mLengthUnit, LengthEditBase::Steps::textHeight(),
                       "symbol_editor/draw_text/height");
  edtHeight->setValue(state.getHeight());
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged,
              edtHeight.get(), &PositiveLengthEdit::setValue));
  mFsmStateConnections.append(connect(edtHeight.get(),
                                      &PositiveLengthEdit::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHeight));
  mCommandToolBarProxy->addWidget(std::move(edtHeight));

  // Horizontal alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  hAlignActionGroup->setValue(state.getHAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::hAlignChanged,
              hAlignActionGroup.get(), &HAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(hAlignActionGroup.get(),
                                      &HAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHAlign));
  mCommandToolBarProxy->addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  vAlignActionGroup->setValue(state.getVAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::vAlignChanged,
              vAlignActionGroup.get(), &VAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(vAlignActionGroup.get(),
                                      &VAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setVAlign));
  mCommandToolBarProxy->addActionGroup(std::move(vAlignActionGroup));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_DrawText& state) noexcept {
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::DRAW_TEXT);

  // Layer
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mCommandToolBarProxy->addLabel(tr("Layer:"));
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::layerChanged,
              layerComboBox.get(), &LayerComboBox::setCurrentLayer));
  mFsmStateConnections.append(
      connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
              &SymbolEditorState_DrawText::setLayer));
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Text
  mCommandToolBarProxy->addLabel(tr("Text:"), 10);
  std::unique_ptr<QComboBox> textComboBox(new QComboBox());
  textComboBox->setEditable(true);
  textComboBox->addItems(state.getTextSuggestions());
  QPointer<QComboBox> textComboBoxPtr = textComboBox.get();
  auto setText = [textComboBoxPtr](const QString& text) {
    if (textComboBoxPtr) {
      int index = textComboBoxPtr->findText(text);
      if (index >= 0) {
        textComboBoxPtr->setCurrentIndex(index);
      } else {
        textComboBoxPtr->setCurrentText(text);
      }
    }
  };
  setText(state.getText());
  mFsmStateConnections.append(connect(&state,
                                      &SymbolEditorState_DrawText::textChanged,
                                      textComboBox.get(), setText));
  mFsmStateConnections.append(connect(textComboBox.get(),
                                      &QComboBox::currentTextChanged, &state,
                                      &SymbolEditorState_DrawText::setText));
  mCommandToolBarProxy->addWidget(std::move(textComboBox));

  // Height
  mCommandToolBarProxy->addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(mLengthUnit, LengthEditBase::Steps::textHeight(),
                       "symbol_editor/draw_text/height");
  edtHeight->setValue(state.getHeight());
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::heightChanged,
              edtHeight.get(), &PositiveLengthEdit::setValue));
  mFsmStateConnections.append(connect(edtHeight.get(),
                                      &PositiveLengthEdit::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHeight));
  mCommandToolBarProxy->addWidget(std::move(edtHeight));

  // Horizontal alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<HAlignActionGroup> hAlignActionGroup(new HAlignActionGroup());
  hAlignActionGroup->setValue(state.getHAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::hAlignChanged,
              hAlignActionGroup.get(), &HAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(hAlignActionGroup.get(),
                                      &HAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setHAlign));
  mCommandToolBarProxy->addActionGroup(std::move(hAlignActionGroup));

  // Vertical alignment
  mCommandToolBarProxy->addSeparator();
  std::unique_ptr<VAlignActionGroup> vAlignActionGroup(new VAlignActionGroup());
  vAlignActionGroup->setValue(state.getVAlign());
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_DrawText::vAlignChanged,
              vAlignActionGroup.get(), &VAlignActionGroup::setValue));
  mFsmStateConnections.append(connect(vAlignActionGroup.get(),
                                      &VAlignActionGroup::valueChanged, &state,
                                      &SymbolEditorState_DrawText::setVAlign));
  mCommandToolBarProxy->addActionGroup(std::move(vAlignActionGroup));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_AddPins& state) noexcept {
  Q_UNUSED(state);
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::ADD_PINS);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Name
  mCommandToolBarProxy->addLabel(tr("Name:"));
  std::unique_ptr<QLineEdit> nameLineEdit(new QLineEdit());
  nameLineEdit->setMaxLength(20);
  nameLineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  nameLineEdit->setText(*state.getName());
  QPointer<QLineEdit> nameLineEditPtr = nameLineEdit.get();
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_AddPins::nameChanged, nameLineEditPtr,
              [nameLineEditPtr](const CircuitIdentifier& name) {
                if (nameLineEditPtr) nameLineEditPtr->setText(*name);
              }));
  mFsmStateConnections.append(connect(
      nameLineEdit.get(), &QLineEdit::textEdited, &state,
      [&state](const QString& text) {
        if (auto name = parseCircuitIdentifier(cleanCircuitIdentifier(text))) {
          state.setName(*name);
        }
      }));
  mCommandToolBarProxy->addWidget(std::move(nameLineEdit));

  // Length
  mCommandToolBarProxy->addLabel(tr("Length:"), 10);
  std::unique_ptr<UnsignedLengthEdit> edtLength(new UnsignedLengthEdit());
  edtLength->configure(mLengthUnit, LengthEditBase::Steps::pinLength(),
                       "symbol_editor/add_pins/length");
  edtLength->setValue(state.getLength());
  edtLength->addAction(cmd.sizeIncrease.createAction(
      edtLength.get(), edtLength.get(), &UnsignedLengthEdit::stepUp));
  edtLength->addAction(cmd.sizeDecrease.createAction(
      edtLength.get(), edtLength.get(), &UnsignedLengthEdit::stepDown));
  mFsmStateConnections.append(
      connect(&state, &SymbolEditorState_AddPins::lengthChanged,
              edtLength.get(), &UnsignedLengthEdit::setValue));
  mFsmStateConnections.append(connect(edtLength.get(),
                                      &UnsignedLengthEdit::valueChanged, &state,
                                      &SymbolEditorState_AddPins::setLength));
  mCommandToolBarProxy->addWidget(std::move(edtLength));

  // Mass import
  std::unique_ptr<QToolButton> toolButtonImport(new QToolButton());
  toolButtonImport->setIcon(QIcon(":/img/actions/import.png"));
  toolButtonImport->setText(tr("Mass Import"));
  toolButtonImport->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  mFsmStateConnections.append(
      connect(toolButtonImport.get(), &QToolButton::clicked, &state,
              &SymbolEditorState_AddPins::processImportPins));
  mCommandToolBarProxy->addWidget(std::move(toolButtonImport));
}

void SymbolEditorWidget::fsmToolEnter(
    SymbolEditorState_Measure& state) noexcept {
  Q_UNUSED(state);
  if (mToolsActionGroup) mToolsActionGroup->setCurrentAction(Tool::MEASURE);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool SymbolEditorWidget::save() noexcept {
  // Remove obsolete message approvals (bypassing the undo stack).
  mSymbol->setMessageApprovals(mSymbol->getMessageApprovals() -
                               mDisappearedApprovals);

  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mSymbol->save();  // can throw
    mFileSystem->save();  // can throw
    mOriginalSymbolPinUuids = mSymbol->getPins().getUuidSet();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool SymbolEditorWidget::selectAll() noexcept {
  return mFsm->processSelectAll();
}

bool SymbolEditorWidget::cut() noexcept {
  return mFsm->processCut();
}

bool SymbolEditorWidget::copy() noexcept {
  return mFsm->processCopy();
}

bool SymbolEditorWidget::paste() noexcept {
  return mFsm->processPaste();
}

bool SymbolEditorWidget::move(Qt::ArrowType direction) noexcept {
  Point delta;
  switch (direction) {
    case Qt::LeftArrow: {
      delta.setX(-mGraphicsScene->getGridInterval());
      break;
    }
    case Qt::RightArrow: {
      delta.setX(*mGraphicsScene->getGridInterval());
      break;
    }
    case Qt::UpArrow: {
      delta.setY(*mGraphicsScene->getGridInterval());
      break;
    }
    case Qt::DownArrow: {
      delta.setY(-mGraphicsScene->getGridInterval());
      break;
    }
    default: {
      qWarning() << "Unhandled switch-case in SymbolEditorWidget::move():"
                 << direction;
      break;
    }
  }
  return mFsm->processMove(delta);
}

bool SymbolEditorWidget::rotate(const Angle& rotation) noexcept {
  return mFsm->processRotate(rotation);
}

bool SymbolEditorWidget::mirror(Qt::Orientation orientation) noexcept {
  return mFsm->processMirror(orientation);
}

bool SymbolEditorWidget::snapToGrid() noexcept {
  return mFsm->processSnapToGrid();
}

bool SymbolEditorWidget::remove() noexcept {
  return mFsm->processRemove();
}

bool SymbolEditorWidget::editProperties() noexcept {
  return mFsm->processEditProperties();
}

bool SymbolEditorWidget::zoomIn() noexcept {
  mUi->graphicsView->zoomIn();
  return true;
}

bool SymbolEditorWidget::zoomOut() noexcept {
  mUi->graphicsView->zoomOut();
  return true;
}

bool SymbolEditorWidget::zoomAll() noexcept {
  mUi->graphicsView->zoomAll();
  return true;
}

bool SymbolEditorWidget::abortCommand() noexcept {
  return mFsm->processAbortCommand();
}

bool SymbolEditorWidget::importDxf() noexcept {
  return mFsm->processStartDxfImport();
}

bool SymbolEditorWidget::editGridProperties() noexcept {
  GridSettingsDialog dialog(mGraphicsScene->getGridInterval(), mLengthUnit,
                            mGraphicsScene->getGridStyle(), this);
  connect(&dialog, &GridSettingsDialog::gridPropertiesChanged, this,
          &SymbolEditorWidget::setGridProperties);
  dialog.exec();
  return true;
}

bool SymbolEditorWidget::increaseGridInterval() noexcept {
  const Length interval = mGraphicsScene->getGridInterval() * 2;
  setGridProperties(PositiveLength(interval), mLengthUnit,
                    mGraphicsScene->getGridStyle());
  return true;
}

bool SymbolEditorWidget::decreaseGridInterval() noexcept {
  const Length interval = *mGraphicsScene->getGridInterval();
  if ((interval % 2) == 0) {
    setGridProperties(PositiveLength(interval / 2), mLengthUnit,
                      mGraphicsScene->getGridStyle());
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolEditorWidget::updateMetadata() noexcept {
  setWindowTitle(*mSymbol->getNames().getDefaultValue());
  mUi->edtName->setText(*mSymbol->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mSymbol->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mSymbol->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mSymbol->getAuthor());
  mUi->edtVersion->setText(mSymbol->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mSymbol->isDeprecated());
  mUi->lstMessages->setApprovals(mSymbol->getMessageApprovals());
  mCategoriesEditorWidget->setUuids(mSymbol->getCategories());
}

QString SymbolEditorWidget::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mSymbol, tr("Edit symbol metadata")));
    try {
      // throws on invalid name
      cmd->setName("", ElementName(mUi->edtName->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    cmd->setKeywords("", mUi->edtKeywords->text().trimmed());
    try {
      // throws on invalid version
      cmd->setVersion(Version::fromString(mUi->edtVersion->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setDeprecated(mUi->cbxDeprecated->isChecked());
    cmd->setCategories(mCategoriesEditorWidget->getUuids());

    // Commit all changes.
    mUndoStack->execCmd(cmd.release());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool SymbolEditorWidget::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyPressed(e);
}

bool SymbolEditorWidget::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool SymbolEditorWidget::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool SymbolEditorWidget::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SymbolEditorWidget::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool SymbolEditorWidget::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool SymbolEditorWidget::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

bool SymbolEditorWidget::toolChangeRequested(Tool newTool,
                                             const QVariant& mode) noexcept {
  Q_UNUSED(mode);
  switch (newTool) {
    case Tool::SELECT:
      return mFsm->processStartSelecting();
    case Tool::ADD_PINS:
      return mFsm->processStartAddingSymbolPins(false);
    case Tool::ADD_NAMES:
      return mFsm->processStartAddingNames();
    case Tool::ADD_VALUES:
      return mFsm->processStartAddingValues();
    case Tool::DRAW_LINE:
      return mFsm->processStartDrawLines();
    case Tool::DRAW_RECT:
      return mFsm->processStartDrawRects();
    case Tool::DRAW_POLYGON:
      return mFsm->processStartDrawPolygons();
    case Tool::DRAW_CIRCLE:
      return mFsm->processStartDrawCircles();
    case Tool::DRAW_ARC:
      return mFsm->processStartDrawArcs();
    case Tool::DRAW_TEXT:
      return mFsm->processStartDrawTexts();
    case Tool::MEASURE:
      return mFsm->processStartMeasure();
    default:
      return false;
  }
}

bool SymbolEditorWidget::isInterfaceBroken() const noexcept {
  return mSymbol->getPins().getUuidSet() != mOriginalSymbolPinUuids;
}

bool SymbolEditorWidget::runChecks(RuleCheckMessageList& msgs) const {
  if ((mFsm->getCurrentTool() != NONE) && (mFsm->getCurrentTool() != SELECT)) {
    // Do not run checks if a tool is active because it could lead to annoying,
    // flickering messages. For example when placing pins, they always overlap
    // right after placing them, so we have to wait until the user has moved the
    // cursor to place the pin at a different position.
    return false;
  }
  msgs = mSymbol->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void SymbolEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void SymbolEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void SymbolEditorWidget::fixMsg(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCategoriesEditorWidget->openAddCategoryDialog();
}

template <>
void SymbolEditorWidget::fixMsg(const MsgMissingSymbolName& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingNames();
}

template <>
void SymbolEditorWidget::fixMsg(const MsgMissingSymbolValue& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingValues();
}

template <>
void SymbolEditorWidget::fixMsg(const MsgWrongSymbolTextLayer& msg) {
  std::shared_ptr<Text> text = mSymbol->getTexts().get(msg.getText().get());
  std::unique_ptr<CmdTextEdit> cmd(new CmdTextEdit(*text));
  cmd->setLayer(msg.getExpectedLayer(), false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolEditorWidget::fixMsg(const MsgSymbolPinNotOnGrid& msg) {
  std::shared_ptr<SymbolPin> pin = mSymbol->getPins().get(msg.getPin().get());
  Point newPos = pin->getPosition().mappedToGrid(msg.getGridInterval());
  std::unique_ptr<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(pin));
  cmd->setPosition(newPos, false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolEditorWidget::fixMsg(
    const MsgNonFunctionalSymbolPinInversionSign& msg) {
  std::shared_ptr<SymbolPin> pin = mSymbol->getPins().get(msg.getPin().get());
  std::unique_ptr<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(pin));
  cmd->setName(CircuitIdentifier("!" % pin->getName()->mid(1)), false);
  mUndoStack->execCmd(cmd.release());
}

template <>
void SymbolEditorWidget::fixMsg(const MsgSymbolOriginNotInCenter& msg) {
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processSelectAll();
  mFsm->processMove(
      -msg.getCenter().mappedToGrid(mGraphicsScene->getGridInterval()));
  mFsm->processAbortCommand();  // Clear selection.
}

template <typename MessageType>
bool SymbolEditorWidget::fixMsgHelper(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool SymbolEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingSymbolName>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingSymbolValue>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgWrongSymbolTextLayer>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgSymbolPinNotOnGrid>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgNonFunctionalSymbolPinInversionSign>(msg, applyFix))
    return true;
  if (fixMsgHelper<MsgSymbolOriginNotInCenter>(msg, applyFix)) return true;
  return false;
}

void SymbolEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  setMessageApproved(*mSymbol, msg, approve);
  updateMetadata();
}

bool SymbolEditorWidget::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
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
        *mSymbol->getNames().getDefaultValue(), 0, defaultFilePath,
        mContext.workspace.getSettings().defaultLengthUnit.get(),
        mContext.workspace.getSettings().themes.getActive(),
        "symbol_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mContext.workspace.getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
  return true;
}

void SymbolEditorWidget::setGridProperties(const PositiveLength& interval,
                                           const LengthUnit& unit,
                                           Theme::GridStyle style) noexcept {
  mGraphicsScene->setGridInterval(interval);
  mGraphicsScene->setGridStyle(style);
  mLengthUnit = unit;
  if (mStatusBar) {
    mStatusBar->setLengthUnit(unit);
  }
  if (mFsm) {
    // Re-calculate "snap to grid" feature!
    mFsm->processGridIntervalChanged(interval);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

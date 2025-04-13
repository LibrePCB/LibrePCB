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
#include "schematictab.h"

#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../projecteditor2.h"
#include "../projectsmodel.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/graphics/graphicslayer.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorfsm.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorstate_addcomponent.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorstate_addtext.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorstate_drawpolygon.h>
#include <librepcb/editor/project/schematic/schematicgraphicsscene.h>
#include <librepcb/editor/undostack.h>
#include <librepcb/editor/workspace/desktopservices.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static QString toSingleLine(const QString& s) {
  return QString(s).replace("\n", "\\n");
}

static QString toMultiLine(const QString& s) {
  return s.trimmed().replace("\\n", "\n");
}

static ui::WireMode l2s(SchematicEditorState_DrawWire::WireMode v) noexcept {
  if (v == SchematicEditorState_DrawWire::WireMode::HV) {
    return ui::WireMode::HV;
  } else if (v == SchematicEditorState_DrawWire::WireMode::VH) {
    return ui::WireMode::VH;
  } else if (v == SchematicEditorState_DrawWire::WireMode::Deg9045) {
    return ui::WireMode::Deg9045;
  } else if (v == SchematicEditorState_DrawWire::WireMode::Deg4590) {
    return ui::WireMode::Deg4590;
  } else if (v == SchematicEditorState_DrawWire::WireMode::Straight) {
    return ui::WireMode::Straight;
  } else {
    return ui::WireMode::HV;
  }
}

static SchematicEditorState_DrawWire::WireMode s2l(ui::WireMode v) noexcept {
  if (v == ui::WireMode::HV) {
    return SchematicEditorState_DrawWire::WireMode::HV;
  } else if (v == ui::WireMode::VH) {
    return SchematicEditorState_DrawWire::WireMode::VH;
  } else if (v == ui::WireMode::Deg9045) {
    return SchematicEditorState_DrawWire::WireMode::Deg9045;
  } else if (v == ui::WireMode::Deg4590) {
    return SchematicEditorState_DrawWire::WireMode::Deg4590;
  } else if (v == ui::WireMode::Straight) {
    return SchematicEditorState_DrawWire::WireMode::Straight;
  } else {
    return SchematicEditorState_DrawWire::WireMode::HV;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicTab::SchematicTab(GuiApplication& app,
                           std::shared_ptr<ProjectEditor2> prj,
                           int schematicIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, parent),
    onDerivedUiDataChanged(*this),
    mEditor(prj),
    mObjIndex(schematicIndex),
    mGridStyle(Theme::GridStyle::None),
    mSceneImagePos(),
    mFrameIndex(0),
    mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolOverlayText(),
    mToolWireMode(SchematicEditorState_DrawWire::WireMode::HV),
    mToolLayersQt(),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolLineWidth(0),
    mToolLineWidthUnit(
        app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mToolSize(1),
    mToolSizeUnit(app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mToolFilled(false),
    mToolValue(),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolAttributeValue(),
    mToolAttributeValuePlaceholder(),
    mToolAttributeUnitsQt(),
    mToolAttributeUnits(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolAttributeUnit(nullptr),
    mFsm() {
  // Connect undo stack.
  connect(&mEditor->getUndoStack(), &UndoStack::stateModified, this,
          [this]() { onUiDataChanged.notify(); });
  connect(mEditor.get(), &ProjectEditor2::manualModificationsMade, this,
          [this]() { onUiDataChanged.notify(); });

  // Refresh UI when ERC is completed to update execution error.
  connect(mEditor.get(), &ProjectEditor2::ercFinished, this,
          [this]() { onUiDataChanged.notify(); });
  connect(mEditor->getErcMessages().get(),
          &RuleCheckMessagesModel::unapprovedCountChanged, this,
          [this]() { onUiDataChanged.notify(); });

  // Build the whole schematic editor finite state machine.
  Q_ASSERT(qApp->activeWindow());
  SchematicEditorFsm::Context fsmContext{
      mApp.getWorkspace(),
      mEditor->getProject(),
      prj->getUndoStack(),
      *qApp->activeWindow(),
      *this,
  };
  mFsm.reset(new SchematicEditorFsm(fsmContext));

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this,
          &SchematicTab::applyTheme);
  applyTheme();
}

SchematicTab::~SchematicTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData SchematicTab::getUiData() const noexcept {
  auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex);

  ui::RuleCheckState ercState;
  if (!mEditor->getErcMessages()) {
    ercState = ui::RuleCheckState::NotRunYet;
  } else {
    ercState = ui::RuleCheckState::UpToDate;
  }
  auto ercMessages = mEditor->getErcMessages();

  return ui::TabData{
      ui::TabType::Schematic,  // Type
      q2s(sch ? *sch->getName() : QString()),  // Title
      ercState,  // Rule check state
      ercMessages,  // Rule check messages
      ercMessages ? ercMessages->getUnapprovedCount()
                  : 0,  // Rule check unapproved messages
      q2s(mEditor->getErcExecutionError()),  // Rule check execution error
      mEditor->getProject().getDirectory().isWritable(),  // Can save
      true,  // Can export graphics
      mEditor->getUndoStack().canUndo(),  // Can undo
      q2s(mEditor->getUndoStack().getUndoCmdText()),  // Undo text
      mEditor->getUndoStack().canRedo(),  // Can redo
      q2s(mEditor->getUndoStack().getRedoCmdText()),  // Redo text
      mToolFeatures.testFlag(Feature::Cut),  // Can cut
      mToolFeatures.testFlag(Feature::Copy),  // Can copy
      mToolFeatures.testFlag(Feature::Paste),  // Can paste
      mToolFeatures.testFlag(Feature::Remove),  // Can remove
      mToolFeatures.testFlag(Feature::Rotate),  // Can rotate
      mToolFeatures.testFlag(Feature::Mirror),  // Can mirror
      mToolFeatures.testFlag(Feature::MoveAlign),  // Can move/align
      mToolFeatures.testFlag(Feature::SnapToGrid),  // Can snap to grid
      mToolFeatures.testFlag(Feature::ResetTexts),  // Can reset texts
      mToolFeatures.testFlag(Feature::LockPlacement),  // Can lock placement
      mToolFeatures.testFlag(Feature::Properties),  // Can edit properties
      ui::Action::None,
  };
}

ui::SchematicTabData SchematicTab::getDerivedUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex);
  auto pinNumbersLayer =
      mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers);

  QString gridIntervalStr;
  if (sch) {
    const LengthUnit& unit = sch->getGridUnit();
    gridIntervalStr = Toolbox::floatToString(
        unit.convertToUnit(*sch->getGridInterval()), 10, QLocale());
  }

  return ui::SchematicTabData{
      mApp.getProjects().getIndexOf(mEditor),  // Project index
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      q2s(gridIntervalStr),  // Grid interval
      sch ? l2s(sch->getGridUnit())
          : ui::LengthUnit::Millimeters,  // Length unit
      pinNumbersLayer && pinNumbersLayer->isVisible(),  // Show pin numbers
      mTool,  // Tool
      q2s(mToolCursorShape),  // Tool cursor
      q2s(mToolOverlayText),  // Tool overlay text
      l2s(mToolWireMode),  // Tool wire mode
      ui::ComboBoxData{
          // Tool layer
          mToolLayers,  // Items
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current index
      },
      ui::LengthEditData{
          // Tool line width
          l2s(*mToolLineWidth),  // Value
          l2s(mToolLineWidthUnit),  // Unit
          *mToolLineWidth % 2 == 0,  // Can decrease
          false,  // Increase
          false,  // Decrease
      },
      ui::LengthEditData{
          // Tool size
          l2s(*mToolSize),  // Value
          l2s(mToolSizeUnit),  // Unit
          *mToolSize % 2 == 0,  // Can decrease
          false,  // Increase
          false,  // Decrease
      },
      mToolFilled,  // Tool filled
      ui::LineEditData{
          // Tool value
          true,  // Enabled
          q2s(toSingleLine(mToolValue)),  // Text
          slint::SharedString(),  // Placeholder
          mToolValueSuggestions,  // Suggestions
      },
      ui::LineEditData{
          // Tool attribute value
          mToolAttributeValue.has_value(),  // Enabled
          mToolAttributeValue ? q2s(toSingleLine(*mToolAttributeValue))
                              : slint::SharedString(),  // Text
          q2s(mToolAttributeValuePlaceholder),  // Placeholder
          mToolValueSuggestions,  // Suggestions
      },
      ui::ComboBoxData{
          // Tool attribute unit
          mToolAttributeUnits,  // Items
          static_cast<int>(mToolAttributeUnitsQt.indexOf(
              mToolAttributeUnit)),  // Current index
      },
      q2s(mSceneImagePos),  // Scene image position
      mFrameIndex,  // Frame index
  };
}

void SchematicTab::setDerivedUiData(const ui::SchematicTabData& data) noexcept {
  auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex);

  mSceneImagePos = s2q(data.scene_image_pos);

  mGridStyle = s2l(data.grid_style);
  if (mScene) {
    mScene->setGridStyle(mGridStyle);
  }
  const LengthUnit unit = s2l(data.unit);
  if (sch && (unit != sch->getGridUnit())) {
    sch->setGridUnit(unit);
    mEditor->setManualModificationsMade();
  }
  if (auto l = mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers)) {
    l->setVisible(data.show_pin_numbers);
  }

  if (const Layer* layer = mToolLayersQt.value(data.tool_layer.current_index)) {
    emit layerRequested(*layer);
  }
  emit wireModeRequested(s2l(data.tool_wire_mode));
  emit filledRequested(data.tool_filled);
  mToolLineWidthUnit = s2l(data.tool_line_width.unit);
  if (auto l = s2ulength(data.tool_line_width.value)) {
    emit lineWidthRequested(*l);
  }
  if (data.tool_line_width.increase) {
    emit lineWidthRequested(UnsignedLength(mToolLineWidth * 2));
  } else if (data.tool_line_width.decrease && (*mToolLineWidth % 2 == 0)) {
    emit lineWidthRequested(UnsignedLength(mToolLineWidth / 2));
  }
  mToolSizeUnit = s2l(data.tool_size.unit);
  if (auto h = s2plength(data.tool_size.value)) {
    emit sizeRequested(*h);
  }
  if (data.tool_size.increase) {
    emit sizeRequested(PositiveLength(mToolSize * 2));
  } else if (data.tool_size.decrease && (mToolSize > 1) &&
             (*mToolSize % 2 == 0)) {
    emit sizeRequested(PositiveLength(mToolSize / 2));
  }
  emit valueRequested(toMultiLine(s2q(data.tool_value.text)));
  // Unit must be set before value, because value may override the unit!
  emit attributeUnitRequested(
      mToolAttributeUnitsQt.value(data.tool_attribute_unit.current_index));
  emit attributeValueRequested(
      toMultiLine(s2q(data.tool_attribute_value.text)));

  requestRepaint();
}

void SchematicTab::activate() noexcept {
  if (auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex)) {
    mScene.reset(new SchematicGraphicsScene(
        *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    mScene->setGridInterval(sch->getGridInterval());
    connect(mScene.get(), &GraphicsScene::changed, this,
            &SchematicTab::requestRepaint);
    applyTheme();
    requestRepaint();
  }

  mEditor->registerActiveSchematicTab(this);
}

void SchematicTab::deactivate() noexcept {
  mEditor->unregisterActiveSchematicTab(this);
  mScene.reset();
}

void SchematicTab::triggerAsync(ui::Action a) noexcept {
  if (a == ui::Action::Save) {
    mEditor->saveProject();
    return;
  } else if (a == ui::Action::SectionGridIntervalIncrease) {
    if (auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex)) {
      sch->setGridInterval(PositiveLength(sch->getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(sch->getGridInterval());
        requestRepaint();
      }
    }
    return;
  } else if (a == ui::Action::SectionGridIntervalDecrease) {
    if (auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex)) {
      if ((*sch->getGridInterval() % 2) == 0) {
        sch->setGridInterval(PositiveLength(sch->getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(sch->getGridInterval());
          requestRepaint();
        }
      }
    }
    return;
  } else if (a == ui::Action::ExportPdf) {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
    return;
  } else if (a == ui::Action::Print) {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
    return;
  } else if (a == ui::Action::Cut) {
    mFsm->processCut();
    return;
  } else if (a == ui::Action::Copy) {
    mFsm->processCopy();
    return;
  } else if (a == ui::Action::Paste) {
    mFsm->processPaste();
    return;
  } else if (a == ui::Action::Delete) {
    mFsm->processRemove();
    return;
  } else if (a == ui::Action::RotateCcw) {
    mFsm->processRotate(Angle::deg90());
    return;
  } else if (a == ui::Action::RotateCw) {
    mFsm->processRotate(-Angle::deg90());
    return;
  } else if (a == ui::Action::MirrorHorizontally) {
    mFsm->processMirror(Qt::Horizontal);
    return;
  } else if (a == ui::Action::MirrorVertically) {
    mFsm->processMirror(Qt::Vertical);
    return;
    //} else if (a == ui::Action::MoveAlign) {
    //  mFsm->process();
    //} else if (a == ui::Action::SnapToGrid) {
    //  mFsm->process();
  } else if (a == ui::Action::ResetTexts) {
    mFsm->processResetAllTexts();
    return;
    //} else if (a == ui::Action::LockUnlockPlacement) {
    //  mFsm->process();
  } else if (a == ui::Action::EditProperties) {
    mFsm->processEditProperties();
    return;
  } else if (a == ui::Action::ToolSelect) {
    mFsm->processSelect();
    return;
  } else if (a == ui::Action::ToolWire) {
    mFsm->processDrawWire();
    return;
  } else if (a == ui::Action::ToolNetlabel) {
    mFsm->processAddNetLabel();
    return;
  } else if (a == ui::Action::ToolPolygon) {
    mFsm->processDrawPolygon();
    return;
  } else if (a == ui::Action::ToolText) {
    mFsm->processAddText();
    return;
  } else if (a == ui::Action::ToolComponent) {
    mFsm->processAddComponent();
    return;
  } else if (a == ui::Action::ToolMeasure) {
    mFsm->processMeasure();
    return;
  } else if (a == ui::Action::FocusLost) {
    if (mMouseEvent.buttons.testFlag(Qt::LeftButton)) {
      mMouseEvent.buttons.setFlag(Qt::LeftButton, false);
      mFsm->processGraphicsSceneLeftMouseButtonReleased(mMouseEvent);
    }
    if (mMouseEvent.buttons.testFlag(Qt::RightButton)) {
      mMouseEvent.buttons.setFlag(Qt::RightButton, false);
    }
    return;
  }

  GraphicsSceneTab::triggerAsync(a);
}

bool SchematicTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  if (GraphicsSceneTab::processScenePointerEvent(pos, globalPos, e)) {
    return true;
  }

  using slint::private_api::PointerEventButton;
  using slint::private_api::PointerEventKind;

  bool handled = false;
  if (mMouseEventIsDoubleClick && (e.button == PointerEventButton::Left)) {
    handled =
        mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(mMouseEvent);
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Down)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonPressed(mMouseEvent);
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonReleased(mMouseEvent);
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneRightMouseButtonReleased(mMouseEvent);
  } else if (e.kind == PointerEventKind::Move) {
    handled = mFsm->processGraphicsSceneMouseMoved(mMouseEvent);
  }

  if (handled) {
    requestRepaint();
  }

  return handled;
}

bool SchematicTab::processSceneKeyPressed(
    const slint::private_api::KeyEvent& e) noexcept {
  const QKeySequence seq(s2q(e.text));
  if (seq.count() == 1) {
    mFsm->processKeyPressed(
        GraphicsSceneKeyEvent{seq[0].key(), s2q(e.modifiers)});
  }
  return false;
}

bool SchematicTab::processSceneKeyReleased(
    const slint::private_api::KeyEvent& e) noexcept {
  const QKeySequence seq(s2q(e.text));
  if (seq.count() == 1) {
    mFsm->processKeyReleased(
        GraphicsSceneKeyEvent{seq[0].key(), s2q(e.modifiers)});
  }
  return false;
}

/*******************************************************************************
 *  SchematicEditorFsmAdapter Methods
 ******************************************************************************/

Schematic* SchematicTab::fsmGetActiveSchematic() noexcept {
  return mEditor->getProject().getSchematicByIndex(mObjIndex);
}

SchematicGraphicsScene* SchematicTab::fsmGetGraphicsScene() noexcept {
  return qobject_cast<SchematicGraphicsScene*>(mScene.get());
}

void SchematicTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mToolCursorShape = *shape;
  } else {
    mToolCursorShape = Qt::ArrowCursor;
  }
  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmSetViewGrayOut(bool grayOut) noexcept {
  if (mScene) {
    mScene->setGrayOut(grayOut);
  }
}

void SchematicTab::fsmSetViewInfoBoxText(const QString& text) noexcept {
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

void SchematicTab::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  if (mScene) {
    mScene->setRulerPositions(pos);
  }
}

void SchematicTab::fsmSetSceneCursor(const Point& pos, bool cross,
                                     bool circle) noexcept {
  if (mScene) {
    mScene->setSceneCursor(pos, cross, circle);
  }
}

QPainterPath SchematicTab::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return calcPosWithTolerance(pos, multiplier);
}

Point SchematicTab::fsmMapGlobalPosToScenePos(
    const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void SchematicTab::fsmSetHighlightedNetSignals(
    const QSet<const NetSignal*>& sigs) noexcept {
  mEditor->setHighlightedNetSignals(sigs);
}

void SchematicTab::fsmAbortBlockingToolsInOtherEditors() noexcept {
  /* TODO */
}

void SchematicTab::fsmSetStatusBarMessage(const QString& message,
                                          int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void SchematicTab::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mTool = ui::EditorTool::Select;
  fsmSetFeatures(Features());
  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(SchematicEditorState_Select& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Select;
  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(SchematicEditorState_DrawWire& state) noexcept {
  mTool = ui::EditorTool::Wire;

  // Wire mode
  auto setWireMode = [this](SchematicEditorState_DrawWire::WireMode m) {
    mToolWireMode = m;
    onDerivedUiDataChanged.notify();
  };
  setWireMode(state.getWireMode());
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_DrawWire::wireModeChanged, this,
              setWireMode));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::wireModeRequested, &state,
              &SchematicEditorState_DrawWire::setWireMode));

  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(
    SchematicEditorState_AddNetLabel& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Netlabel;
  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(
    SchematicEditorState_AddComponent& state) noexcept {
  mTool = ui::EditorTool::Component;

  // Value
  auto setValue = [this](const QString& value) {
    mToolValue = value;
    onDerivedUiDataChanged.notify();
  };
  setValue(state.getValue());
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_AddComponent::valueChanged, this,
              setValue));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::valueRequested, &state,
              &SchematicEditorState_AddComponent::setValue));

  // Value suggestions
  auto setValueSuggestions = [this](const QStringList& suggestions) {
    mToolValueSuggestions->clear();
    for (const QString& v : suggestions) {
      mToolValueSuggestions->push_back(q2s(v));
    }
    onDerivedUiDataChanged.notify();
  };
  setValueSuggestions(state.getValueSuggestions());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddComponent::valueSuggestionsChanged, this,
      setValueSuggestions));

  // Attribute key
  auto setAttributeKey = [this](const std::optional<AttributeKey>& key) {
    mToolAttributeValuePlaceholder = key ? **key : QString();
    onDerivedUiDataChanged.notify();
  };
  setAttributeKey(state.getValueAttributeKey());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddComponent::valueAttributeKeyChanged,
      this, setAttributeKey));

  // Attribute value
  auto setAttributeValue = [this](const std::optional<QString>& value) {
    mToolAttributeValue = value;
    onDerivedUiDataChanged.notify();
  };
  setAttributeValue(state.getValueAttributeValue());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddComponent::valueAttributeValueChanged,
      this, setAttributeValue));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::attributeValueRequested, &state,
              [&state](QString value) {
                if (const AttributeType* type = state.getValueAttributeType()) {
                  if (const AttributeUnit* unit =
                          type->tryExtractUnitFromValue(value)) {
                    state.setValueAttributeUnit(unit);
                  }
                  state.setValueAttributeValue(value);
                }
              }));

  // Attribute units
  auto setAttributeUnits = [this](const AttributeType* type) {
    mToolAttributeUnitsQt =
        type ? type->getAvailableUnits() : QList<const AttributeUnit*>{};
    mToolAttributeUnits->clear();
    for (const AttributeUnit* unit : mToolAttributeUnitsQt) {
      mToolAttributeUnits->push_back(q2s(unit->getSymbolTr()));
    }
    onDerivedUiDataChanged.notify();
  };
  setAttributeUnits(state.getValueAttributeType());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddComponent::valueAttributeTypeChanged,
      this, setAttributeUnits));

  // Attribute unit
  auto setAttributeUnit = [this](const AttributeUnit* unit) {
    mToolAttributeUnit = unit;
    onDerivedUiDataChanged.notify();
  };
  setAttributeUnit(state.getValueAttributeUnit());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddComponent::valueAttributeUnitChanged,
      this, setAttributeUnit));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::attributeUnitRequested, &state,
              &SchematicEditorState_AddComponent::setValueAttributeUnit));

  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(
    SchematicEditorState_DrawPolygon& state) noexcept {
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
      &state, &SchematicEditorState_DrawPolygon::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::layerRequested, &state,
              &SchematicEditorState_DrawPolygon::setLayer));

  // Line width
  auto setLineWidth = [this](const UnsignedLength& width) {
    mToolLineWidth = width;
    onDerivedUiDataChanged.notify();
  };
  setLineWidth(state.getLineWidth());
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_DrawPolygon::lineWidthChanged, this,
              setLineWidth));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::lineWidthRequested, &state,
              &SchematicEditorState_DrawPolygon::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_DrawPolygon::filledChanged, this,
              setFilled));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::filledRequested, &state,
              &SchematicEditorState_DrawPolygon::setFilled));

  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(SchematicEditorState_AddText& state) noexcept {
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
      &state, &SchematicEditorState_AddText::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &SchematicTab::layerRequested,
                                      &state,
                                      &SchematicEditorState_AddText::setLayer));

  // Height
  auto setHeight = [this](const PositiveLength& height) {
    mToolSize = height;
    onDerivedUiDataChanged.notify();
  };
  setHeight(state.getHeight());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddText::heightChanged, this, setHeight));
  mFsmStateConnections.append(
      connect(this, &SchematicTab::sizeRequested, &state,
              &SchematicEditorState_AddText::setHeight));

  // Text
  auto setText = [this](const QString& text) {
    mToolValue = text;
    onDerivedUiDataChanged.notify();
  };
  setText(state.getText());
  mFsmStateConnections.append(connect(
      &state, &SchematicEditorState_AddText::textChanged, this, setText));
  mFsmStateConnections.append(connect(this, &SchematicTab::valueRequested,
                                      &state,
                                      &SchematicEditorState_AddText::setText));

  // Text suggestions
  mToolValueSuggestions->clear();
  for (const QString& v : state.getTextSuggestions()) {
    mToolValueSuggestions->push_back(q2s(v));
  }

  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmToolEnter(SchematicEditorState_Measure& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Measure;
  onDerivedUiDataChanged.notify();
}

void SchematicTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    onUiDataChanged.notify();
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const LengthUnit* SchematicTab::getCurrentUnit() const noexcept {
  if (auto sch = mEditor->getProject().getSchematicByIndex(mObjIndex)) {
    return &sch->getGridUnit();
  } else {
    return nullptr;
  }
}

void SchematicTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SchematicTab::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    const QString projectName =
        FilePath::cleanFileName(*mEditor->getProject().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    const QString projectVersion =
        FilePath::cleanFileName(*mEditor->getProject().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    const QString relativePath =
        QString("output/%1/%2_Schematics").arg(projectVersion, projectName);
    const FilePath defaultFilePath =
        mEditor->getProject().getPath().getPathTo(relativePath);

    // Copy all schematic pages to allow processing them in worker threads.
    const int count = mEditor->getProject().getSchematics().count();
    QProgressDialog progress(tr("Preparing schematics..."), tr("Cancel"), 0,
                             count, qApp->activeWindow());
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(100);
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    for (int i = 0; i < count; ++i) {
      pages.append(std::make_shared<SchematicPainter>(
          *mEditor->getProject().getSchematicByIndex(i)));
      progress.setValue(i + 1);
      if (progress.wasCanceled()) {
        return;
      }
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Schematic, output, pages, mObjIndex,
        *mEditor->getProject().getName(), 0, defaultFilePath,
        mApp.getWorkspace().getSettings().defaultLengthUnit.get(),
        mApp.getWorkspace().getSettings().themes.getActive(),
        "schematic_editor/" % settingsKey, qApp->activeWindow());
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mApp.getWorkspace().getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void SchematicTab::applyTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  mGridStyle = theme.getSchematicGridStyle();

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

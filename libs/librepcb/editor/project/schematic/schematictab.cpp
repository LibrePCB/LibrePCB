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

#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/desktopservices.h"
#include "../projecteditor.h"
#include "fsm/schematiceditorfsm.h"
#include "fsm/schematiceditorstate_addcomponent.h"
#include "fsm/schematiceditorstate_addtext.h"
#include "fsm/schematiceditorstate_drawpolygon.h"
#include "graphicsitems/sgi_symbol.h"
#include "schematiceditor.h"
#include "schematicgraphicsscene.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>
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

SchematicTab::SchematicTab(GuiApplication& app, SchematicEditor& editor,
                           QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mProjectEditor(editor.getProjectEditor()),
    mProject(mProjectEditor.getProject()),
    mSchematicEditor(editor),
    mSchematic(mSchematicEditor.getSchematic()),
    mLayers(
        GraphicsLayerList::schematicLayers(&app.getWorkspace().getSettings())),
    mPinNumbersLayer(mLayers->get(Theme::Color::sSchematicPinNumbers)),
    mView(new SlintGraphicsView(this)),
    mMsgInstallLibraries(app.getWorkspace(), "EMPTY_SCHEMATIC_NO_LIBRARIES"),
    mMsgAddDrawingFrame(app.getWorkspace(), "EMPTY_SCHEMATIC_ADD_FRAME"),
    mGridStyle(mApp.getWorkspace()
                   .getSettings()
                   .themes.getActive()
                   .getSchematicGridStyle()),
    mFrameIndex(0),
    mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolWireMode(SchematicEditorState_DrawWire::WireMode::HV),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolLineWidth(app.getWorkspace().getSettings()),
    mToolSize(app.getWorkspace().getSettings()),
    mToolFilled(false),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolAttributeUnits(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolAttributeUnit(nullptr) {
  Q_ASSERT(&mSchematic.getProject() == &mProject);

  // Setup graphics view.
  mView->setEventHandler(this);
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &SchematicTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });

  // Connect schematic editor.
  connect(&mSchematicEditor, &SchematicEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mSchematicEditor, &SchematicEditor::aboutToBeDestroyed, this,
          &SchematicTab::closeEnforced);

  // Connect project editor.
  connect(&mProjectEditor, &ProjectEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mProjectEditor, &ProjectEditor::abortBlockingToolsInOtherEditors,
          this, [this](const void* source) {
            if (source != this) {
              // Not so nice...
              mFsm->processAbortCommand();
              mFsm->processAbortCommand();
              mFsm->processAbortCommand();
            }
          });

  // Connect undo stack.
  connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified, this,
          [this]() { onUiDataChanged.notify(); });
  connect(&mProjectEditor, &ProjectEditor::manualModificationsMade, this,
          [this]() { onUiDataChanged.notify(); });

  // Connect tool values.
  connect(&mToolLineWidth, &LengthEditContext::uiDataChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });

  // Connect search context.
  connect(&mSearchContext, &SearchContext::goToTriggered, this,
          &SchematicTab::goToSymbol);

  // Setup messages.
  connect(&mApp.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &SchematicTab::updateMessages);
  connect(&mSchematic, &Schematic::symbolAdded, this,
          &SchematicTab::updateMessages);
  connect(&mSchematic, &Schematic::symbolRemoved, this,
          &SchematicTab::updateMessages);
  connect(&mMsgInstallLibraries, &DismissableMessageContext::visibilityChanged,
          this, [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mMsgAddDrawingFrame, &DismissableMessageContext::visibilityChanged,
          this, [this]() { onDerivedUiDataChanged.notify(); });
  updateMessages();

  // Build the whole schematic editor finite state machine.
  SchematicEditorFsm::Context fsmContext{
      mApp.getWorkspace(),           mProject, mSchematic,
      mProjectEditor.getUndoStack(), *this,
  };
  mFsm.reset(new SchematicEditorFsm(fsmContext));

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this,
          &SchematicTab::applyTheme);
  applyTheme();

  // Restore client settings.
  QSettings cs;
  if (mPinNumbersLayer) {
    mPinNumbersLayer->setVisible(
        cs.value("schematic_editor/show_pin_numbers", true).toBool());
  }
}

SchematicTab::~SchematicTab() noexcept {
  deactivate();
  mView->setEventHandler(nullptr);

  // Delete FSM as it may trigger some other methods during destruction.
  mFsm.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int SchematicTab::getProjectIndex() const noexcept {
  return mProjectEditor.getUiIndex();
}

int SchematicTab::getProjectObjectIndex() const noexcept {
  return mProject.getSchematicIndex(mSchematic);
}

ui::TabData SchematicTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.save = toFs(mProject.getDirectory().isWritable());
  features.undo = toFs(mProjectEditor.getUndoStack().canUndo());
  features.redo = toFs(mProjectEditor.getUndoStack().canRedo());
  features.export_graphics = toFs(mTool == ui::EditorTool::Select);
  features.select = toFs(mToolFeatures.testFlag(Feature::Select));
  features.cut = toFs(mToolFeatures.testFlag(Feature::Cut));
  features.copy = toFs(mToolFeatures.testFlag(Feature::Copy));
  features.paste = toFs(mToolFeatures.testFlag(Feature::Paste));
  features.remove = toFs(mToolFeatures.testFlag(Feature::Remove));
  features.rotate = toFs(mToolFeatures.testFlag(Feature::Rotate));
  features.mirror = toFs(mToolFeatures.testFlag(Feature::Mirror));
  features.snap_to_grid = toFs(mToolFeatures.testFlag(Feature::SnapToGrid));
  features.reset_texts = toFs(mToolFeatures.testFlag(Feature::ResetTexts));
  features.edit_properties = toFs(mToolFeatures.testFlag(Feature::Properties));
  features.find = toFs(true);

  return ui::TabData{
      ui::TabType::Schematic,  // Type
      q2s(*mSchematic.getName()),  // Title
      features,  // Features
      !mProject.getDirectory().isWritable(),  // Read-only
      mProjectEditor.hasUnsavedChanges(),  // Unsaved changes
      q2s(mProjectEditor.getUndoStack().getUndoCmdText()),  // Undo text
      q2s(mProjectEditor.getUndoStack().getRedoCmdText()),  // Redo text
      q2s(mSearchContext.getTerm()),  // Find term
      mSearchContext.getSuggestions(),  // Find suggestions
      nullptr,  // Layers
  };
}

void SchematicTab::setUiData(const ui::TabData& data) noexcept {
  mSearchContext.setTerm(s2q(data.find_term));
  WindowTab::setUiData(data);
  onUiDataChanged.notify();
}

ui::SchematicTabData SchematicTab::getDerivedUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;

  return ui::SchematicTabData{
      mProjectEditor.getUiIndex(),  // Project index
      mSchematicEditor.getUiIndex(),  // Schematic index
      q2s(bgColor),  // Background color
      q2s(fgColor),  // Foreground color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      l2s(*mSchematic.getGridInterval()),  // Grid interval
      l2s(mSchematic.getGridUnit()),  // Length unit
      mPinNumbersLayer && mPinNumbersLayer->isVisible(),  // Show pin numbers
      mMsgInstallLibraries.getUiData(),  // Message "install libraries"
      mMsgAddDrawingFrame.getUiData(),  // Message "add schematic frame"
      mTool,  // Tool
      q2s(mView->isPanning() ? Qt::ClosedHandCursor
                             : mToolCursorShape),  // Tool cursor
      q2s(mToolOverlayText),  // Tool overlay text
      l2s(mToolWireMode),  // Tool wire mode
      ui::ComboBoxData{
          // Tool layer
          mToolLayers,  // Items
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current index
      },
      mToolLineWidth.getUiData(),  // Tool line width
      mToolSize.getUiData(),  // // Tool size
      mToolFilled,  // Tool filled
      ui::LineEditData{
          // Tool value
          true,  // Enabled
          q2s(EditorToolbox::toSingleLine(mToolValue)),  // Text
          slint::SharedString(),  // Placeholder
          mToolValueSuggestions,  // Suggestions
      },
      ui::LineEditData{
          // Tool attribute value
          mToolAttributeValue.has_value(),  // Enabled
          mToolAttributeValue
              ? q2s(EditorToolbox::toSingleLine(*mToolAttributeValue))
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
  mSceneImagePos = s2q(data.scene_image_pos);

  mGridStyle = s2l(data.grid_style);
  const std::optional<PositiveLength> interval = s2plength(data.grid_interval);
  if (interval && (*interval != mSchematic.getGridInterval())) {
    mSchematic.setGridInterval(*interval);
    mProjectEditor.setManualModificationsMade();
  }
  if (mScene) {
    mScene->setGridStyle(mGridStyle);
    mScene->setGridInterval(mSchematic.getGridInterval());
  }
  const LengthUnit unit = s2l(data.unit);
  if (unit != mSchematic.getGridUnit()) {
    mSchematic.setGridUnit(unit);
    mProjectEditor.setManualModificationsMade();
  }
  if (mPinNumbersLayer &&
      (mPinNumbersLayer->isVisible() != data.show_pin_numbers)) {
    mPinNumbersLayer->setVisible(data.show_pin_numbers);
    QSettings cs;
    cs.setValue("schematic_editor/show_pin_numbers", data.show_pin_numbers);
  }

  // Messages
  mMsgInstallLibraries.setUiData(data.install_libraries_msg);
  mMsgAddDrawingFrame.setUiData(data.add_drawing_frame_msg);

  if (const Layer* layer = mToolLayersQt.value(data.tool_layer.current_index)) {
    emit layerRequested(*layer);
  }
  emit wireModeRequested(s2l(data.tool_wire_mode));
  emit filledRequested(data.tool_filled);
  mToolLineWidth.setUiData(data.tool_line_width);
  mToolSize.setUiData(data.tool_size);
  emit valueRequested(EditorToolbox::toMultiLine(s2q(data.tool_value.text)));
  // Unit must be set before value, because value may override the unit!
  emit attributeUnitRequested(
      mToolAttributeUnitsQt.value(data.tool_attribute_unit.current_index));
  emit attributeValueRequested(
      EditorToolbox::toMultiLine(s2q(data.tool_attribute_value.text)));

  requestRepaint();
}

void SchematicTab::activate() noexcept {
  mScene.reset(new SchematicGraphicsScene(
      mSchematic, *mLayers, mProjectEditor.getHighlightedNetSignals(), this));
  mScene->setGridInterval(mSchematic.getGridInterval());
  connect(&mProjectEditor, &ProjectEditor::highlightedNetSignalsChanged,
          mScene.get(), &SchematicGraphicsScene::updateHighlightedNetSignals);
  connect(mScene.get(), &GraphicsScene::changed, this,
          &SchematicTab::requestRepaint);

  // Initialize search context.
  mSearchContext.init();

  applyTheme();
  mProjectEditor.registerActiveSchematicTab(this);
  requestRepaint();
}

void SchematicTab::deactivate() noexcept {
  mProjectEditor.unregisterActiveSchematicTab(this);
  mSearchContext.deinit();
  mScene.reset();
}

void SchematicTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
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
    case ui::TabAction::SelectAll: {
      mFsm->processSelectAll();
      break;
    }
    case ui::TabAction::Abort: {
      mFsm->processAbortCommand();
      break;
    }
    case ui::TabAction::Undo: {
      mProjectEditor.undo();
      break;
    }
    case ui::TabAction::Redo: {
      mProjectEditor.redo();
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
      if (!mFsm->processMove(Point(-mSchematic.getGridInterval(), 0))) {
        mView->scrollLeft();
      }
      break;
    }
    case ui::TabAction::MoveRight: {
      if (!mFsm->processMove(Point(*mSchematic.getGridInterval(), 0))) {
        mView->scrollRight();
      }
      break;
    }
    case ui::TabAction::MoveUp: {
      if (!mFsm->processMove(Point(0, *mSchematic.getGridInterval()))) {
        mView->scrollUp();
      }
      break;
    }
    case ui::TabAction::MoveDown: {
      if (!mFsm->processMove(Point(0, -mSchematic.getGridInterval()))) {
        mView->scrollDown();
      }
      break;
    }
    case ui::TabAction::SnapToGrid: {
      mFsm->processSnapToGrid();
      break;
    }
    case ui::TabAction::ResetTexts: {
      mFsm->processResetAllTexts();
      break;
    }
    case ui::TabAction::EditProperties: {
      mFsm->processEditProperties();
      break;
    }
    case ui::TabAction::GridIntervalIncrease: {
      mSchematic.setGridInterval(
          PositiveLength(mSchematic.getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(mSchematic.getGridInterval());
        requestRepaint();
      }
      break;
    }
    case ui::TabAction::GridIntervalDecrease: {
      if ((*mSchematic.getGridInterval() % 2) == 0) {
        mSchematic.setGridInterval(
            PositiveLength(mSchematic.getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(mSchematic.getGridInterval());
          requestRepaint();
        }
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
    case ui::TabAction::FindRefreshSuggestions: {
      QStringList names;
      for (const SI_Symbol* sym : mSchematic.getSymbols()) {
        names.append(sym->getName());
      }
      Toolbox::sortNumeric(names);
      mSearchContext.setSuggestions(names);
      break;
    }
    case ui::TabAction::FindNext: {
      mSearchContext.findNext();
      break;
    }
    case ui::TabAction::FindPrevious: {
      mSearchContext.findPrevious();
      break;
    }
    case ui::TabAction::ToolSelect: {
      mFsm->processSelect();
      break;
    }
    case ui::TabAction::ToolWire: {
      mFsm->processDrawWire();
      break;
    }
    case ui::TabAction::ToolNetlabel: {
      mFsm->processAddNetLabel();
      break;
    }
    case ui::TabAction::ToolPolygon: {
      mFsm->processDrawPolygon();
      break;
    }
    case ui::TabAction::ToolText: {
      mFsm->processAddText();
      break;
    }
    case ui::TabAction::ToolComponent: {
      mFsm->processAddComponent();
      break;
    }
    case ui::TabAction::ToolComponentFrame: {
      mFsm->processAddComponent("schematic frame");
      break;
    }
    case ui::TabAction::ToolComponentResistor: {
      mFsm->processAddComponent(
          Uuid::fromString("ef80cd5e-2689-47ee-8888-31d04fc99174"),
          Uuid::fromString(mProjectEditor.getUseIeee315Symbols()
                               ? "d16e1f44-16af-4773-a310-de370f744548"
                               : "a5995314-f535-45d4-8bd8-2d0b8a0dc42a"));
      break;
    }
    case ui::TabAction::ToolComponentInductor: {
      mFsm->processAddComponent(
          Uuid::fromString("506bd124-6062-400e-9078-b38bd7e1aaee"),
          Uuid::fromString(mProjectEditor.getUseIeee315Symbols()
                               ? "4245d515-6f6d-48cb-9958-a4ea23d0187f"
                               : "62a7598c-17fe-41cf-8fa1-4ed274c3adc2"));
      break;
    }
    case ui::TabAction::ToolComponentCapacitorBipolar: {
      mFsm->processAddComponent(
          Uuid::fromString("d167e0e3-6a92-4b76-b013-77b9c230e5f1"),
          Uuid::fromString(mProjectEditor.getUseIeee315Symbols()
                               ? "6e639ff1-4e81-423b-9d0e-b28b35693a61"
                               : "8cd7b37f-e5fa-4af5-a8dd-d78830bba3af"));
      break;
    }
    case ui::TabAction::ToolComponentCapacitorUnipolar: {
      mFsm->processAddComponent(
          Uuid::fromString("c54375c5-7149-4ded-95c5-7462f7301ee7"),
          Uuid::fromString(mProjectEditor.getUseIeee315Symbols()
                               ? "20a01a81-506e-4fee-9dc0-8b50e6537cd4"
                               : "5412add2-af9c-44b8-876d-a0fb7c201897"));
      break;
    }
    case ui::TabAction::ToolComponentGnd: {
      mFsm->processAddComponent(
          Uuid::fromString("8076f6be-bfab-4fc1-9772-5d54465dd7e1"),
          Uuid::fromString("f09ad258-595b-4ee9-a1fc-910804a203ae"));
      break;
    }
    case ui::TabAction::ToolComponentVcc: {
      mFsm->processAddComponent(
          Uuid::fromString("58c3c6cd-11eb-4557-aa3f-d3e05874afde"),
          Uuid::fromString("afb86b45-68ec-47b6-8d96-153d73567228"));
      break;
    }
    case ui::TabAction::ToolMeasure: {
      mFsm->processMeasure();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image SchematicTab::renderScene(float width, float height,
                                       int scene) noexcept {
  Q_UNUSED(scene);
  if (mScene) {
    return mView->render(*mScene, width, height);
  }
  return slint::Image();
}

bool SchematicTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  return mView->pointerEvent(pos, e);
}

bool SchematicTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView->scrollEvent(pos, e);
}

bool SchematicTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  return mView->keyEvent(e);
}

/*******************************************************************************
 *  IF_GraphicsViewEventHandler Methods
 ******************************************************************************/

bool SchematicTab::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyPressed(e);
}

bool SchematicTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool SchematicTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  emit cursorCoordinatesChanged(e.scenePos, mSchematic.getGridUnit());
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool SchematicTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool SchematicTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool SchematicTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  SchematicEditorFsmAdapter Methods
 ******************************************************************************/

SchematicGraphicsScene* SchematicTab::fsmGetGraphicsScene() noexcept {
  return mScene.get();
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
  return mView->calcPosWithTolerance(pos, multiplier);
}

Point SchematicTab::fsmMapGlobalPosToScenePos(
    const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mView->mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void SchematicTab::fsmZoomToSceneRect(const QRectF& r) noexcept {
  mView->zoomToSceneRect(r);
}

void SchematicTab::fsmSetHighlightedNetSignals(
    const QSet<const NetSignal*>& sigs) noexcept {
  mProjectEditor.setHighlightedNetSignals(sigs);
}

void SchematicTab::fsmAbortBlockingToolsInOtherEditors() noexcept {
  mProjectEditor.abortBlockingToolsInOtherEditors(this);
}

void SchematicTab::fsmSetStatusBarMessage(const QString& message,
                                          int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void SchematicTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    onUiDataChanged.notify();
  }
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
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "schematic_editor/draw_polygon/line_width");
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_DrawPolygon::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
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
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "schematic_editor/add_text/size");
  mFsmStateConnections.append(
      connect(&state, &SchematicEditorState_AddText::heightChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SchematicTab::updateMessages() noexcept {
  try {
    const bool empty = mSchematic.getSymbols().isEmpty();
    const bool noLibs = empty
        ? mApp.getWorkspace().getLibraryDb().getAll<Library>().isEmpty()
        : false;
    mMsgInstallLibraries.setActive(empty && noLibs);
    mMsgAddDrawingFrame.setActive(empty && (!noLibs));
  } catch (const Exception& e) {
  }
}

void SchematicTab::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    const QString projectName = FilePath::cleanFileName(
        *mProject.getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    const QString projectVersion = FilePath::cleanFileName(
        *mProject.getVersion(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    const QString relativePath =
        QString("output/%1/%2_Schematics").arg(projectVersion, projectName);
    const FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);

    // Copy all schematic pages to allow processing them in worker threads.
    const int count = mProject.getSchematics().count();
    QProgressDialog progress(tr("Preparing schematics..."), tr("Cancel"), 0,
                             count, qApp->activeWindow());
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(100);
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    for (int i = 0; i < count; ++i) {
      pages.append(
          std::make_shared<SchematicPainter>(*mProject.getSchematicByIndex(i)));
      progress.setValue(i + 1);
      if (progress.wasCanceled()) {
        return;
      }
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Schematic, output, pages,
        mProject.getSchematicIndex(mSchematic), *mProject.getName(), 0,
        defaultFilePath,
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

void SchematicTab::goToSymbol(const QString& name, int index) noexcept {
  QList<SI_Symbol*> symbolCandidates;
  foreach (SI_Symbol* symbol, mSchematic.getSymbols().values()) {
    if (symbol->getName().startsWith(name, Qt::CaseInsensitive)) {
      symbolCandidates.append(symbol);
    }
  }

  // Sort by name for a natural order of results.
  Toolbox::sortNumeric(
      symbolCandidates,
      [](const QCollator& cmp, const SI_Symbol* lhs, const SI_Symbol* rhs) {
        return cmp(lhs->getName(), rhs->getName());
      },
      Qt::CaseInsensitive, false);

  if (symbolCandidates.count()) {
    while (index < 0) {
      index += symbolCandidates.count();
    }
    index %= symbolCandidates.count();
    SI_Symbol* symbol = symbolCandidates[index];
    if (mScene) {
      mScene->clearSelection();
      if (auto item = mScene->getSymbols().value(symbol)) {
        item->setSelected(true);
        QRectF rect = item->mapRectToScene(item->childrenBoundingRect());
        // Zoom to a rectangle relative to the maximum graphics item dimension,
        // occupying 1/4th of the screen, but limiting the margin to 10mm.
        const qreal margin =
            std::min(1.5f * std::max(rect.size().width(), rect.size().height()),
                     Length::fromMm(10).toPx());
        rect.adjust(-margin, -margin, margin, margin);
        mView->zoomToSceneRect(rect);
      }
    }
  }
}

void SchematicTab::applyTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();

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

void SchematicTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

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
#include "schematiceditor.h"

#include "../../dialogs/filedialog.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../editorcommandset.h"
#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/graphicsscene.h"
#include "../../project/cmd/cmdschematicadd.h"
#include "../../project/cmd/cmdschematicedit.h"
#include "../../project/cmd/cmdschematicremove.h"
#include "../../undostack.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/menubuilder.h"
#include "../../utils/standardeditorcommandhandler.h"
#include "../../utils/toolbarproxy.h"
#include "../../utils/undostackactiongroup.h"
#include "../../widgets/attributeunitcombobox.h"
#include "../../widgets/layercombobox.h"
#include "../../widgets/positivelengthedit.h"
#include "../../widgets/rulecheckdock.h"
#include "../../widgets/searchtoolbar.h"
#include "../../widgets/unsignedlengthedit.h"
#include "../../workspace/desktopservices.h"
#include "../bomgeneratordialog.h"
#include "../outputjobsdialog/outputjobsdialog.h"
#include "../projecteditor.h"
#include "../projectsetupdialog.h"
#include "fsm/schematiceditorfsm.h"
#include "fsm/schematiceditorstate_addcomponent.h"
#include "fsm/schematiceditorstate_addtext.h"
#include "fsm/schematiceditorstate_drawpolygon.h"
#include "fsm/schematiceditorstate_drawwire.h"
#include "graphicsitems/sgi_symbol.h"
#include "schematicgraphicsscene.h"
#include "schematicpagesdock.h"
#include "ui_schematiceditor.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QSvgGenerator>
#include <QtCore>
#include <QtPrintSupport>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditor::SchematicEditor(ProjectEditor& projectEditor, Project& project)
  : QMainWindow(0),
    mProjectEditor(projectEditor),
    mProject(project),
    mUi(new Ui::SchematicEditor),
    mCommandToolBarProxy(new ToolBarProxy(this)),
    mStandardCommandHandler(new StandardEditorCommandHandler(
        mProjectEditor.getWorkspace().getSettings(), this)),
    mActiveSchematicIndex(-1),
    mLayers(GraphicsLayerList::schematicLayers(
        &projectEditor.getWorkspace().getSettings())),
    mGraphicsScene(),
    mVisibleSceneRect(),
    mFsm() {
  mUi->setupUi(this);

  // Setup graphics view.
  const Theme& theme =
      mProjectEditor.getWorkspace().getSettings().themes.getActive();
  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->graphicsView->setInfoBoxColors(
      theme.getColor(Theme::Color::sSchematicInfoBox).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicInfoBox).getSecondaryColor());
  mUi->graphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  mUi->graphicsView->setEventHandlerObject(this);
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mUi->statusbar, &StatusBar::setAbsoluteCursorPosition);

  // Setup status bar.
  mUi->statusbar->setFields(StatusBar::AbsolutePosition |
                            StatusBar::ProgressBar);
  mUi->statusbar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mProjectEditor.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanProgressUpdate, mUi->statusbar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);
  mUi->statusbar->setProgressBarPercent(
      mProjectEditor.getWorkspace().getLibraryDb().getScanProgressPercent());
  connect(&mProjectEditor, &ProjectEditor::showTemporaryStatusBarMessage,
          mUi->statusbar, &StatusBar::showMessage);

  // Set window title.
  QString filenameStr = mProject.getFilepath().getFilename();
  if (!mProject.getDirectory().isWritable()) {
    filenameStr.append(QStringLiteral(" [Read-Only]"));
  }
  setWindowTitle(tr("%1 - LibrePCB Schematic Editor").arg(filenameStr));

  // Build the whole schematic editor finite state machine.
  SchematicEditorFsm::Context fsmContext{
      mProjectEditor.getWorkspace(),
      mProject,
      mProjectEditor.getUndoStack(),
      *this,
      *this,
  };
  mFsm.reset(new SchematicEditorFsm(fsmContext));

  // Create all actions, window menus, toolbars and dock widgets.
  createActions();
  createToolBars();
  createDockWidgets();
  createMenus();  // Depends on dock widgets!

  // Disable actions which do not work nicely with *.lppz projects yet.
  if (!mProject.getDirectory().isWritable()) {
    mActionGenerateBom->setEnabled(false);
    mActionOutputJobs->setEnabled(false);
  }

  // Setup "project upgraded" message.
  {
    const QString msg = mProjectEditor.getUpgradeMessageLabelText();
    mUi->msgProjectUpgraded->init(msg, !msg.isEmpty());
    connect(mUi->msgProjectUpgraded, &MessageWidget::linkActivated, this,
            [this]() { mProjectEditor.showUpgradeMessages(this); });
    connect(&mProjectEditor, &ProjectEditor::projectSavedToDisk, this,
            [this]() { mUi->msgProjectUpgraded->setActive(false); });
  }

  // Setup "empty schematic" message.
  mUi->msgEmptySchematic->init(
      mProjectEditor.getWorkspace(), "SCHEMATIC_HAS_NO_SYMBOLS",
      tr("This schematic doesn't contain any components yet. Use the "
         "<a href='%1'>Add Component</a> dialog to populate it. A good idea "
         "is to <a href='%2'>add a schematic frame</a> first.")
          .arg("dialog")
          .arg("frame"),
      false);
  connect(mUi->msgEmptySchematic, &MessageWidget::linkActivated, this,
          [this](const QString& link) {
            if (mFsm) {
              if (link == "frame") {
                mFsm->processAddComponent("schematic frame");
              } else {
                mFsm->processAddComponent();
              }
            }
          });

  // Restore window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("schematic_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("schematic_editor/window_state_v2").toByteArray());
  mActionShowPinNumbers->setChecked(
      clientSettings.value("schematic_editor/show_pin_numbers", true).toBool());

  // Load first schematic page
  if (mProject.getSchematics().count() > 0) setActiveSchematicIndex(0);

  // Set focus to graphics view (avoid having the focus in some arbitrary
  // widget).
  mUi->graphicsView->setFocus();

  // mGraphicsView->zoomAll(); does not work properly here, should be executed
  // later in the event loop (ugly, but seems to work...)
  QTimer::singleShot(200, mUi->graphicsView, &GraphicsView::zoomAll);
}

SchematicEditor::~SchematicEditor() {
  // Save window geometry.
  QSettings clientSettings;
  clientSettings.setValue("schematic_editor/window_geometry", saveGeometry());
  clientSettings.setValue("schematic_editor/window_state_v2", saveState());
  clientSettings.setValue("schematic_editor/show_pin_numbers",
                          mActionShowPinNumbers->isChecked());

  // Delete FSM as it may trigger some other methods during destruction.
  mFsm.reset();

  // Important: Release command toolbar proxy since otherwise the actions will
  // be deleted first.
  mCommandToolBarProxy->setToolBar(nullptr);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Schematic* SchematicEditor::getActiveSchematic() const noexcept {
  return mProject.getSchematicByIndex(mActiveSchematicIndex);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool SchematicEditor::setActiveSchematicIndex(int index) noexcept {
  if (index == mActiveSchematicIndex) return true;

  // "Ask" the FSM if changing the scene is allowed at the moment.
  // If the FSM accepts the event, we can switch to the specified schematic
  // page.
  if (!mFsm->processSwitchToSchematicPage(index)) {
    return false;  // changing the schematic page is not allowed!
  }

  // event accepted --> change the schematic page
  Schematic* schematic = getActiveSchematic();
  if (schematic) {
    // Save current view scene rect.
    mVisibleSceneRect[schematic->getUuid()] =
        mUi->graphicsView->getVisibleSceneRect();
  }
  mUi->graphicsView->setScene(nullptr);
  mGraphicsScene.reset();
  while (!mSchematicConnections.isEmpty()) {
    disconnect(mSchematicConnections.takeLast());
  }

  schematic = mProject.getSchematicByIndex(index);

  if (schematic) {
    // show scene, restore view scene rect, set grid properties
    const Theme& theme =
        mProjectEditor.getWorkspace().getSettings().themes.getActive();
    mGraphicsScene.reset(new SchematicGraphicsScene(
        *schematic, *mLayers, mProjectEditor.getHighlightedNetSignals()));
    mGraphicsScene->setBackgroundColors(
        theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
    mGraphicsScene->setOverlayColors(
        theme.getColor(Theme::Color::sSchematicOverlays).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicOverlays).getSecondaryColor());
    mGraphicsScene->setSelectionRectColors(
        theme.getColor(Theme::Color::sSchematicSelection).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicSelection).getSecondaryColor());
    mGraphicsScene->setGridStyle(theme.getSchematicGridStyle());
    mGraphicsScene->setGridInterval(schematic->getGridInterval());
    connect(&mProjectEditor, &ProjectEditor::highlightedNetSignalsChanged,
            mGraphicsScene.data(),
            &SchematicGraphicsScene::updateHighlightedNetSignals);
    mUi->graphicsView->setScene(mGraphicsScene.data());
    const QRectF sceneRect = mVisibleSceneRect.value(schematic->getUuid());
    if (!sceneRect.isEmpty()) {
      mUi->graphicsView->setVisibleSceneRect(sceneRect);
    }
    mUi->statusbar->setLengthUnit(schematic->getGridUnit());
    mSchematicConnections.append(
        connect(schematic, &Schematic::symbolAdded, this,
                &SchematicEditor::updateEmptySchematicMessage));
    mSchematicConnections.append(
        connect(schematic, &Schematic::symbolRemoved, this,
                &SchematicEditor::updateEmptySchematicMessage));
  } else {
    mUi->graphicsView->setScene(nullptr);
  }

  // update toolbars
  mActionGridProperties->setEnabled(schematic != nullptr);
  mActionGridIncrease->setEnabled(schematic != nullptr);
  mActionGridDecrease->setEnabled(schematic != nullptr);

  // schematic page has changed!
  mActiveSchematicIndex = index;
  mFsm->processSwitchedSchematicPage();
  emit activeSchematicChanged(mActiveSchematicIndex);
  updateEmptySchematicMessage();
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicEditor::abortAllCommands() noexcept {
  // ugly... ;-)
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
}

void SchematicEditor::abortBlockingToolsInOtherEditors() noexcept {
  mProjectEditor.abortBlockingToolsInOtherEditors(this);
}

/*******************************************************************************
 *  SchematicEditorFsmAdapter Methods
 ******************************************************************************/

Schematic* SchematicEditor::fsmGetActiveSchematic() noexcept {
  return getActiveSchematic();
}

SchematicGraphicsScene* SchematicEditor::fsmGetGraphicsScene() noexcept {
  return qobject_cast<SchematicGraphicsScene*>(mUi->graphicsView->getScene());
}

void SchematicEditor::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mUi->graphicsView->setCursor(*shape);
  } else {
    mUi->graphicsView->unsetCursor();
  }
}

void SchematicEditor::fsmSetViewGrayOut(bool grayOut) noexcept {
  if (mGraphicsScene) {
    mGraphicsScene->setGrayOut(grayOut);
  }
}

void SchematicEditor::fsmSetViewInfoBoxText(const QString& text) noexcept {
  mUi->graphicsView->setInfoBoxText(text);
}

void SchematicEditor::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  if (mGraphicsScene) {
    mGraphicsScene->setRulerPositions(pos);
  }
}

void SchematicEditor::fsmSetSceneCursor(const Point& pos, bool cross,
                                        bool circle) noexcept {
  if (mGraphicsScene) {
    mGraphicsScene->setSceneCursor(pos, cross, circle);
  }
}

QPainterPath SchematicEditor::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return mUi->graphicsView->calcPosWithTolerance(pos, multiplier);
}

Point SchematicEditor::fsmMapGlobalPosToScenePos(
    const QPoint& pos) const noexcept {
  return mUi->graphicsView->mapGlobalPosToScenePos(pos);
}

void SchematicEditor::fsmZoomToSceneRect(const QRectF& r) noexcept {
  mUi->graphicsView->zoomToRect(r);
}

void SchematicEditor::fsmSetHighlightedNetSignals(
    const QSet<const NetSignal*>& sigs) noexcept {
  mProjectEditor.setHighlightedNetSignals(sigs);
}

void SchematicEditor::fsmAbortBlockingToolsInOtherEditors() noexcept {
  abortBlockingToolsInOtherEditors();
}

void SchematicEditor::fsmSetStatusBarMessage(const QString& message,
                                             int timeoutMs) noexcept {
  if (timeoutMs < 0) {
    mUi->statusbar->setPermanentMessage(message);
  } else {
    mUi->statusbar->showMessage(message, timeoutMs);
  }
}

void SchematicEditor::fsmToolLeave() noexcept {
  mCommandToolBarProxy->clear();
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::IDLE);
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_Select& state) noexcept {
  Q_UNUSED(state);
  if (mToolsActionGroup) {
    mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::SELECT);
  }
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_DrawWire& state) noexcept {
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::DRAW_WIRE);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add wire mode actions to the "command" toolbar
  std::unique_ptr<QActionGroup> wireModeActionGroup(
      new QActionGroup(mCommandToolBarProxy.get()));
  QAction* aWireModeHV = cmd.wireModeHV.createAction(
      wireModeActionGroup.get(), &state, [&state]() {
        state.setWireMode(SchematicEditorState_DrawWire::WireMode::HV);
      });
  aWireModeHV->setCheckable(true);
  aWireModeHV->setActionGroup(wireModeActionGroup.get());
  QAction* aWireModeVH = cmd.wireModeVH.createAction(
      wireModeActionGroup.get(), &state, [&state]() {
        state.setWireMode(SchematicEditorState_DrawWire::WireMode::VH);
      });
  aWireModeVH->setCheckable(true);
  aWireModeVH->setActionGroup(wireModeActionGroup.get());
  QAction* aWireMode9045 = cmd.wireMode9045.createAction(
      wireModeActionGroup.get(), &state, [&state]() {
        state.setWireMode(SchematicEditorState_DrawWire::WireMode::Deg9045);
      });
  aWireMode9045->setCheckable(true);
  aWireMode9045->setActionGroup(wireModeActionGroup.get());
  QAction* aWireMode4590 = cmd.wireMode4590.createAction(
      wireModeActionGroup.get(), &state, [&state]() {
        state.setWireMode(SchematicEditorState_DrawWire::WireMode::Deg4590);
      });
  aWireMode4590->setCheckable(true);
  aWireMode4590->setActionGroup(wireModeActionGroup.get());
  QAction* aWireModeStraight = cmd.wireModeStraight.createAction(
      wireModeActionGroup.get(), &state, [&state]() {
        state.setWireMode(SchematicEditorState_DrawWire::WireMode::Straight);
      });
  aWireModeStraight->setCheckable(true);
  aWireModeStraight->setActionGroup(wireModeActionGroup.get());
  QHash<SchematicEditorState_DrawWire::WireMode, QPointer<QAction>>
      wireModeActions = {
          {SchematicEditorState_DrawWire::WireMode::HV, aWireModeHV},
          {SchematicEditorState_DrawWire::WireMode::VH, aWireModeVH},
          {SchematicEditorState_DrawWire::WireMode::Deg9045, aWireMode9045},
          {SchematicEditorState_DrawWire::WireMode::Deg4590, aWireMode4590},
          {SchematicEditorState_DrawWire::WireMode::Straight,
           aWireModeStraight},
      };
  auto setWireMode =
      [wireModeActions](SchematicEditorState_DrawWire::WireMode wm) {
        if (auto a = wireModeActions.value(wm)) {
          a->setChecked(true);
        }
      };
  setWireMode(state.getWireMode());
  connect(&state, &SchematicEditorState_DrawWire::wireModeChanged,
          wireModeActionGroup.get(), setWireMode);
  mCommandToolBarProxy->addActionGroup(std::move(wireModeActionGroup));
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_AddNetLabel& state) noexcept {
  Q_UNUSED(state);
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::ADD_NETLABEL);
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_AddComponent& state) noexcept {
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::ADD_COMPONENT);

  auto toSingleLine = [](const QString& s) {
    return QString(s).replace("\n", "\\n");
  };
  auto toMultiLine = [](const QString& s) {
    return s.trimmed().replace("\\n", "\n");
  };

  // Component value.
  mCommandToolBarProxy->addLabel(tr("Value:"), 10);
  std::unique_ptr<QComboBox> cbxValue(new QComboBox());
  QPointer<QComboBox> cbxValuePtr = cbxValue.get();
  cbxValue->setEditable(true);
  cbxValue->setFixedHeight(QLineEdit().sizeHint().height());
  cbxValue->setMinimumWidth(200);
  cbxValue->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto setValue = [cbxValuePtr, toSingleLine](const QString& value) {
    if (cbxValuePtr) {
      cbxValuePtr->setCurrentText(toSingleLine(value));
    }
  };
  setValue(state.getValue());
  connect(&state, &SchematicEditorState_AddComponent::valueChanged,
          cbxValue.get(), setValue);
  connect(cbxValue.get(), &QComboBox::currentTextChanged, &state,
          [&state, toMultiLine](const QString& text) {
            state.setValue(toMultiLine(text));
          });
  auto setValueSuggestions = [cbxValuePtr](const QStringList& suggestions) {
    if (cbxValuePtr) {
      QSignalBlocker block(cbxValuePtr);
      const QString text = cbxValuePtr->currentText();
      cbxValuePtr->clear();
      cbxValuePtr->addItems(suggestions);
      cbxValuePtr->setCurrentText(text);
    }
  };
  setValueSuggestions(state.getValueSuggestions());
  connect(&state, &SchematicEditorState_AddComponent::valueSuggestionsChanged,
          cbxValue.get(), setValueSuggestions);
  // Make sure the start of the value is visible, even if the value is long.
  cbxValue->lineEdit()->setCursorPosition(0);
  mCommandToolBarProxy->addWidget(std::move(cbxValue));

  // Attribute value.
  std::unique_ptr<QLineEdit> edtAttributeValue(new QLineEdit());
  QPointer<QLineEdit> edtAttributeValuePtr = edtAttributeValue.get();
  edtAttributeValue->setClearButtonEnabled(true);
  edtAttributeValue->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  QPointer<QAction> aAttributeValue =
      mCommandToolBarProxy->addWidget(std::move(edtAttributeValue));
  auto setAttributeKey =
      [edtAttributeValuePtr](const std::optional<AttributeKey>& key) {
        if (edtAttributeValuePtr) {
          edtAttributeValuePtr->setPlaceholderText(key ? **key : QString());
        }
      };
  setAttributeKey(state.getValueAttributeKey());
  connect(&state, &SchematicEditorState_AddComponent::valueAttributeKeyChanged,
          aAttributeValue, setAttributeKey);
  auto setAttributeValue = [edtAttributeValuePtr, aAttributeValue,
                            toSingleLine](const std::optional<QString>& value) {
    if (edtAttributeValuePtr && value) {
      edtAttributeValuePtr->setText(toSingleLine(*value));
    }
    if (aAttributeValue) {
      aAttributeValue->setVisible(value.has_value());
    }
  };
  setAttributeValue(state.getValueAttributeValue());
  connect(&state,
          &SchematicEditorState_AddComponent::valueAttributeValueChanged,
          aAttributeValue, setAttributeValue);
  connect(edtAttributeValuePtr, &QLineEdit::textEdited, &state,
          [&state, toMultiLine](const QString& text) {
            if (const AttributeType* type = state.getValueAttributeType()) {
              QString value = toMultiLine(text);
              if (const AttributeUnit* unit =
                      type->tryExtractUnitFromValue(value)) {
                state.setValueAttributeUnit(unit);
              }
              state.setValueAttributeValue(value);
            }
          });

  // Attribute unit.
  std::unique_ptr<AttributeUnitComboBox> cbxAttributeUnit(
      new AttributeUnitComboBox());
  QPointer<AttributeUnitComboBox> cbxAttributeUnitPtr = cbxAttributeUnit.get();
  cbxAttributeUnit->setFixedHeight(QLineEdit().sizeHint().height());
  QPointer<QAction> aAttributeUnit =
      mCommandToolBarProxy->addWidget(std::move(cbxAttributeUnit));
  auto setAttributeType = [cbxAttributeUnitPtr, aAttributeUnit,
                           edtAttributeValuePtr](const AttributeType* type) {
    if (cbxAttributeUnitPtr && type) {
      cbxAttributeUnitPtr->setAttributeType(*type);
    }
    const bool hasUnits = type && (!type->getAvailableUnits().isEmpty());
    if (aAttributeUnit) {
      aAttributeUnit->setVisible(hasUnits);
    }
    if (edtAttributeValuePtr) {
      edtAttributeValuePtr->setMinimumWidth(hasUnits ? 50 : 200);
    }
  };
  setAttributeType(state.getValueAttributeType());
  connect(&state, &SchematicEditorState_AddComponent::valueAttributeTypeChanged,
          aAttributeUnit, setAttributeType);
  auto setAttributeUnit = [cbxAttributeUnitPtr](const AttributeUnit* unit) {
    if (cbxAttributeUnitPtr) {
      cbxAttributeUnitPtr->setCurrentItem(unit);
    }
  };
  setAttributeUnit(state.getValueAttributeUnit());
  connect(&state, &SchematicEditorState_AddComponent::valueAttributeUnitChanged,
          aAttributeUnit, setAttributeUnit);
  connect(cbxAttributeUnitPtr, &AttributeUnitComboBox::currentItemChanged,
          &state, &SchematicEditorState_AddComponent::setValueAttributeUnit);
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_DrawPolygon& state) noexcept {
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::DRAW_POLYGON);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layers combobox to the toolbar
  mCommandToolBarProxy->addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
          &SchematicEditorState_DrawPolygon::setLayer);
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Add the width edit to the toolbar
  mCommandToolBarProxy->addLabel(tr("Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> widthEdit(new UnsignedLengthEdit());
  widthEdit->setValue(state.getLineWidth());
  widthEdit->addAction(cmd.lineWidthIncrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepUp));
  widthEdit->addAction(cmd.lineWidthDecrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepDown));
  connect(widthEdit.get(), &UnsignedLengthEdit::valueChanged, &state,
          &SchematicEditorState_DrawPolygon::setLineWidth);
  mCommandToolBarProxy->addWidget(std::move(widthEdit));

  // Add the filled checkbox to the toolbar
  mCommandToolBarProxy->addLabel(tr("Filled:"), 10);
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox());
  fillCheckBox->setChecked(state.getFilled());
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  connect(fillCheckBox.get(), &QCheckBox::toggled, &state,
          &SchematicEditorState_DrawPolygon::setFilled);
  mCommandToolBarProxy->addWidget(std::move(fillCheckBox));
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_AddText& state) noexcept {
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::ADD_TEXT);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layers combobox to the toolbar
  mCommandToolBarProxy->addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(state.getAvailableLayers());
  layerComboBox->setCurrentLayer(state.getLayer());
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, &state,
          &SchematicEditorState_AddText::setLayer);
  mCommandToolBarProxy->addWidget(std::move(layerComboBox));

  // Add the text combobox to the toolbar
  mCommandToolBarProxy->addLabel(tr("Text:"), 10);
  std::unique_ptr<QComboBox> textComboBox(new QComboBox());
  textComboBox->setEditable(true);
  textComboBox->setMinimumContentsLength(20);
  textComboBox->addItems(state.getTextSuggestions());
  textComboBox->setCurrentIndex(textComboBox->findText(state.getText()));
  textComboBox->setCurrentText(state.getText());
  connect(textComboBox.get(), &QComboBox::currentTextChanged, &state,
          &SchematicEditorState_AddText::setText);
  mCommandToolBarProxy->addWidget(std::move(textComboBox));

  // Add the height spinbox to the toolbar
  mCommandToolBarProxy->addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> heightEdit(new PositiveLengthEdit());
  heightEdit->setValue(state.getHeight());
  heightEdit->addAction(cmd.sizeIncrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepUp));
  heightEdit->addAction(cmd.sizeDecrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepDown));
  connect(heightEdit.get(), &PositiveLengthEdit::valueChanged, &state,
          &SchematicEditorState_AddText::setHeight);
  mCommandToolBarProxy->addWidget(std::move(heightEdit));
}

void SchematicEditor::fsmToolEnter(
    SchematicEditorState_Measure& state) noexcept {
  Q_UNUSED(state);
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::MEASURE);
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

void SchematicEditor::closeEvent(QCloseEvent* event) noexcept {
  if (!mProjectEditor.windowIsAboutToClose(*this))
    event->ignore();
  else
    QMainWindow::closeEvent(event);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SchematicEditor::createActions() noexcept {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  mActionAboutLibrePcb.reset(cmd.aboutLibrePcb.createAction(
      this, &mProjectEditor, &ProjectEditor::aboutLibrePcbRequested));
  mActionAboutQt.reset(
      cmd.aboutQt.createAction(this, qApp, &QApplication::aboutQt));
  mActionOnlineDocumentation.reset(cmd.documentationOnline.createAction(
      this, mStandardCommandHandler.data(),
      &StandardEditorCommandHandler::onlineDocumentation));
  mActionKeyboardShortcutsReference.reset(
      cmd.keyboardShortcutsReference.createAction(
          this, mStandardCommandHandler.data(),
          &StandardEditorCommandHandler::shortcutsReference));
  mActionWebsite.reset(
      cmd.website.createAction(this, mStandardCommandHandler.data(),
                               &StandardEditorCommandHandler::website));
  mActionSaveProject.reset(cmd.projectSave.createAction(
      this, &mProjectEditor, &ProjectEditor::saveProject));
  mActionSaveProject->setEnabled(mProject.getDirectory().isWritable());
  mActionCloseProject.reset(cmd.projectClose.createAction(
      this, this, [this]() { mProjectEditor.closeAndDestroy(true, this); }));
  mActionCloseWindow.reset(
      cmd.windowClose.createAction(this, this, &SchematicEditor::close));
  mActionQuit.reset(cmd.applicationQuit.createAction(
      this, qApp, &QApplication::closeAllWindows,
      EditorCommand::ActionFlag::QueuedConnection));
  mActionFileManager.reset(cmd.fileManager.createAction(this, this, [this]() {
    mStandardCommandHandler->fileManager(mProject.getPath());
  }));
  mActionBoardEditor.reset(cmd.boardEditor.createAction(
      this, &mProjectEditor, &ProjectEditor::showBoardEditor));
  mActionControlPanel.reset(cmd.controlPanel.createAction(
      this, &mProjectEditor, &ProjectEditor::showControlPanelClicked));
  mActionProjectSetup.reset(cmd.projectSetup.createAction(this, this, [this]() {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    ProjectSetupDialog dialog(mProject, mProjectEditor.getUndoStack(), this);
    dialog.exec();
  }));
  mActionUpdateLibrary.reset(
      cmd.projectLibraryUpdate.createAction(this, this, [this]() {
        // Ugly hack until we have a *real* project library updater...
        emit mProjectEditor.openProjectLibraryUpdaterClicked(
            mProject.getFilepath());
      }));
  mActionExportLppz.reset(cmd.exportLppz.createAction(
      this, this, [this]() { mProjectEditor.execLppzExportDialog(this); }));
  mActionExportImage.reset(cmd.exportImage.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                             "image_export");
  }));
  mActionExportPdf.reset(cmd.exportPdf.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
  }));
  mActionPrint.reset(cmd.print.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
  }));
  mActionGenerateBom.reset(cmd.generateBom.createAction(this, this, [this]() {
    const Board* board = mProject.getBoards().count() == 1
        ? mProject.getBoardByIndex(0)
        : nullptr;
    BomGeneratorDialog dialog(mProjectEditor.getWorkspace().getSettings(),
                              mProject, board, this);
    connect(&dialog, &BomGeneratorDialog::projectSettingsModified,
            &mProjectEditor, &ProjectEditor::setManualModificationsMade);
    dialog.exec();
  }));
  mActionOutputJobs.reset(cmd.outputJobs.createAction(this, this, [this]() {
    OutputJobsDialog dialog(mProjectEditor.getWorkspace().getSettings(),
                            mProject, mProjectEditor.getUndoStack(), this);
    connect(&dialog, &OutputJobsDialog::orderPcbDialogTriggered, this,
            [this, &dialog]() { mProjectEditor.execOrderPcbDialog(&dialog); });
    dialog.exec();
  }));
  mActionOrderPcb.reset(cmd.orderPcb.createAction(
      this, this, [this]() { mProjectEditor.execOrderPcbDialog(this); }));
  mActionNewSheet.reset(
      cmd.sheetNew.createAction(this, this, &SchematicEditor::addSchematic));
  mActionRenameSheet.reset(cmd.sheetRename.createAction(
      this, this, [this]() { renameSchematic(mActiveSchematicIndex); }));
  mActionRemoveSheet.reset(cmd.sheetRemove.createAction(
      this, this, [this]() { removeSchematic(mActiveSchematicIndex); }));
  mActionNextPage.reset(cmd.pageNext.createAction(this, this, [this]() {
    const int newIndex = mActiveSchematicIndex + 1;
    if (newIndex < mProject.getSchematics().count()) {
      setActiveSchematicIndex(newIndex);
    }
  }));
  addAction(mActionNextPage.data());
  mActionPreviousPage.reset(cmd.pagePrevious.createAction(this, this, [this]() {
    const int newIndex = mActiveSchematicIndex - 1;
    if (newIndex >= 0) {
      setActiveSchematicIndex(newIndex);
    }
  }));
  addAction(mActionPreviousPage.data());
  mActionFind.reset(cmd.find.createAction(this));
  mActionFindNext.reset(cmd.findNext.createAction(this));
  mActionFindPrevious.reset(cmd.findPrevious.createAction(this));
  mActionSelectAll.reset(cmd.selectAll.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processSelectAll));
  mActionGridProperties.reset(cmd.gridProperties.createAction(
      this, this, &SchematicEditor::execGridPropertiesDialog));
  mActionGridIncrease.reset(cmd.gridIncrease.createAction(this, this, [this]() {
    const Schematic* schematic = getActiveSchematic();
    if (schematic && mGraphicsScene) {
      const Length interval = schematic->getGridInterval() * 2;
      setGridProperties(PositiveLength(interval), schematic->getGridUnit(),
                        mGraphicsScene->getGridStyle(), true);
    }
  }));
  mActionGridDecrease.reset(cmd.gridDecrease.createAction(this, this, [this]() {
    const Schematic* schematic = getActiveSchematic();
    if (schematic && mGraphicsScene) {
      const Length interval = *schematic->getGridInterval();
      if ((interval % 2) == 0) {
        setGridProperties(PositiveLength(interval / 2),
                          schematic->getGridUnit(),
                          mGraphicsScene->getGridStyle(), true);
      }
    }
  }));
  std::shared_ptr<GraphicsLayer> pinNumbersLayer =
      mLayers->get(Theme::Color::sSchematicPinNumbers);
  Q_ASSERT(pinNumbersLayer);
  mActionShowPinNumbers.reset(cmd.showPinNumbers.createAction(
      this, this,
      [pinNumbersLayer](bool checked) {
        if (pinNumbersLayer) pinNumbersLayer->setVisible(checked);
      },
      EditorCommand::ActionFlag::ReactOnToggle));
  mActionShowPinNumbers->setCheckable(true);
  mActionShowPinNumbers->setChecked(pinNumbersLayer &&
                                    pinNumbersLayer->isVisible());
  mActionZoomFit.reset(cmd.zoomFitContent.createAction(this, mUi->graphicsView,
                                                       &GraphicsView::zoomAll));
  mActionZoomIn.reset(
      cmd.zoomIn.createAction(this, mUi->graphicsView, &GraphicsView::zoomIn));
  mActionZoomOut.reset(cmd.zoomOut.createAction(this, mUi->graphicsView,
                                                &GraphicsView::zoomOut));
  mActionUndo.reset(cmd.undo.createAction(this));
  mActionRedo.reset(cmd.redo.createAction(this));
  mActionCut.reset(cmd.clipboardCut.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processCut));
  mActionCopy.reset(cmd.clipboardCopy.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processCopy));
  mActionPaste.reset(cmd.clipboardPaste.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processPaste));
  mActionMoveLeft.reset(cmd.moveLeft.createAction(this, this, [this]() {
    if (const Schematic* schematic = getActiveSchematic()) {
      if (!mFsm->processMove(Point(-schematic->getGridInterval(), 0))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->horizontalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepSub);
      }
    }
  }));
  addAction(mActionMoveLeft.data());
  mActionMoveRight.reset(cmd.moveRight.createAction(this, this, [this]() {
    if (const Schematic* schematic = getActiveSchematic()) {
      if (!mFsm->processMove(Point(*schematic->getGridInterval(), 0))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->horizontalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepAdd);
      }
    }
  }));
  addAction(mActionMoveRight.data());
  mActionMoveUp.reset(cmd.moveUp.createAction(this, this, [this]() {
    if (const Schematic* schematic = getActiveSchematic()) {
      if (!mFsm->processMove(Point(0, *schematic->getGridInterval()))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->verticalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepSub);
      }
    }
  }));
  addAction(mActionMoveUp.data());
  mActionMoveDown.reset(cmd.moveDown.createAction(this, this, [this]() {
    if (const Schematic* schematic = getActiveSchematic()) {
      if (!mFsm->processMove(Point(0, -schematic->getGridInterval()))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->verticalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepAdd);
      }
    }
  }));
  addAction(mActionMoveDown.data());
  mActionRotateCcw.reset(cmd.rotateCcw.createAction(
      this, this, [this]() { mFsm->processRotate(Angle::deg90()); }));
  mActionRotateCw.reset(cmd.rotateCw.createAction(
      this, this, [this]() { mFsm->processRotate(-Angle::deg90()); }));
  mActionMirrorHorizontal.reset(cmd.mirrorHorizontal.createAction(
      this, this, [this]() { mFsm->processMirror(Qt::Horizontal); }));
  mActionMirrorVertical.reset(cmd.mirrorVertical.createAction(
      this, this, [this]() { mFsm->processMirror(Qt::Vertical); }));
  mActionResetAllTexts.reset(cmd.deviceResetTextAll.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processResetAllTexts));
  mActionProperties.reset(cmd.properties.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processEditProperties));
  mActionRemove.reset(cmd.remove.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processRemove));
  mActionAbort.reset(cmd.abort.createAction(
      this, mFsm.data(), &SchematicEditorFsm::processAbortCommand));
  mActionToolSelect.reset(cmd.toolSelect.createAction(this));
  mActionToolWire.reset(cmd.toolWire.createAction(this));
  mActionToolNetLabel.reset(cmd.toolNetLabel.createAction(this));
  mActionToolPolygon.reset(cmd.toolPolygon.createAction(this));
  mActionToolText.reset(cmd.toolText.createAction(this));
  mActionToolComponent.reset(cmd.toolComponent.createAction(this));
  mActionToolMeasure.reset(cmd.toolMeasure.createAction(this));
  mActionComponentResistor.reset(
      cmd.componentResistor.createAction(this, this, [this]() {
        Uuid componentUuid =
            Uuid::fromString("ef80cd5e-2689-47ee-8888-31d04fc99174");
        Uuid symbVarUuid = Uuid::fromString(
            useIeee315Symbols() ? "d16e1f44-16af-4773-a310-de370f744548"
                                : "a5995314-f535-45d4-8bd8-2d0b8a0dc42a");
        mFsm->processAddComponent(componentUuid, symbVarUuid);
      }));
  mActionComponentInductor.reset(
      cmd.componentInductor.createAction(this, this, [this]() {
        Uuid componentUuid =
            Uuid::fromString("506bd124-6062-400e-9078-b38bd7e1aaee");
        Uuid symbVarUuid = Uuid::fromString(
            useIeee315Symbols() ? "4245d515-6f6d-48cb-9958-a4ea23d0187f"
                                : "62a7598c-17fe-41cf-8fa1-4ed274c3adc2");
        mFsm->processAddComponent(componentUuid, symbVarUuid);
      }));
  mActionComponentCapacitorBipolar.reset(
      cmd.componentCapacitorBipolar.createAction(this, this, [this]() {
        Uuid componentUuid =
            Uuid::fromString("d167e0e3-6a92-4b76-b013-77b9c230e5f1");
        Uuid symbVarUuid = Uuid::fromString(
            useIeee315Symbols() ? "6e639ff1-4e81-423b-9d0e-b28b35693a61"
                                : "8cd7b37f-e5fa-4af5-a8dd-d78830bba3af");
        mFsm->processAddComponent(componentUuid, symbVarUuid);
      }));
  mActionComponentCapacitorUnipolar.reset(
      cmd.componentCapacitorUnipolar.createAction(this, this, [this]() {
        Uuid componentUuid =
            Uuid::fromString("c54375c5-7149-4ded-95c5-7462f7301ee7");
        Uuid symbVarUuid = Uuid::fromString(
            useIeee315Symbols() ? "20a01a81-506e-4fee-9dc0-8b50e6537cd4"
                                : "5412add2-af9c-44b8-876d-a0fb7c201897");
        mFsm->processAddComponent(componentUuid, symbVarUuid);
      }));
  mActionComponentGnd.reset(cmd.componentGnd.createAction(this, this, [this]() {
    Uuid componentUuid =
        Uuid::fromString("8076f6be-bfab-4fc1-9772-5d54465dd7e1");
    Uuid symbVarUuid = Uuid::fromString("f09ad258-595b-4ee9-a1fc-910804a203ae");
    mFsm->processAddComponent(componentUuid, symbVarUuid);
  }));
  mActionComponentVcc.reset(cmd.componentVcc.createAction(this, this, [this]() {
    Uuid componentUuid =
        Uuid::fromString("58c3c6cd-11eb-4557-aa3f-d3e05874afde");
    Uuid symbVarUuid = Uuid::fromString("afb86b45-68ec-47b6-8d96-153d73567228");
    mFsm->processAddComponent(componentUuid, symbVarUuid);
  }));
  mActionDockPages.reset(cmd.dockPages.createAction(this, this, [this]() {
    mDockPages->show();
    mDockPages->raise();
    mDockPages->setFocus();
  }));
  mActionDockErc.reset(cmd.dockErc.createAction(this, this, [this]() {
    mDockErc->show();
    mDockErc->raise();
    mDockErc->setFocus();
  }));

  // Widget shortcuts.
  mUi->graphicsView->addAction(cmd.commandToolBarFocus.createAction(
      this, this,
      [this]() {
        mCommandToolBarProxy->startTabFocusCycle(*mUi->graphicsView);
      },
      EditorCommand::ActionFlag::WidgetShortcut));

  // Undo stack action group.
  mUndoStackActionGroup.reset(
      new UndoStackActionGroup(*mActionUndo, *mActionRedo, nullptr,
                               &mProjectEditor.getUndoStack(), this));

  // Tools action group.
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(mActionToolSelect.data(),
                               SchematicEditorFsm::State::SELECT);
  mToolsActionGroup->addAction(mActionToolWire.data(),
                               SchematicEditorFsm::State::DRAW_WIRE);
  mToolsActionGroup->addAction(mActionToolNetLabel.data(),
                               SchematicEditorFsm::State::ADD_NETLABEL);
  mToolsActionGroup->addAction(mActionToolPolygon.data(),
                               SchematicEditorFsm::State::DRAW_POLYGON);
  mToolsActionGroup->addAction(mActionToolText.data(),
                               SchematicEditorFsm::State::ADD_TEXT);
  mToolsActionGroup->addAction(mActionToolComponent.data(),
                               SchematicEditorFsm::State::ADD_COMPONENT);
  mToolsActionGroup->addAction(mActionToolMeasure.data(),
                               SchematicEditorFsm::State::MEASURE);
  mToolsActionGroup->setCurrentAction(SchematicEditorFsm::State::SELECT);
  connect(mToolsActionGroup.data(), &ExclusiveActionGroup::actionTriggered,
          this, &SchematicEditor::toolRequested);
}

void SchematicEditor::createToolBars() noexcept {
  // File.
  mToolBarFile.reset(new QToolBar(tr("File"), this));
  mToolBarFile->setObjectName("toolBarFile");
  mToolBarFile->addAction(mActionCloseProject.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionNewSheet.data());
  mToolBarFile->addAction(mActionSaveProject.data());
  mToolBarFile->addAction(mActionPrint.data());
  mToolBarFile->addAction(mActionExportPdf.data());
  mToolBarFile->addAction(mActionOutputJobs.data());
  mToolBarFile->addAction(mActionOrderPcb.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionControlPanel.data());
  mToolBarFile->addAction(mActionBoardEditor.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionUndo.data());
  mToolBarFile->addAction(mActionRedo.data());
  addToolBar(Qt::TopToolBarArea, mToolBarFile.data());

  // Edit.
  mToolBarEdit.reset(new QToolBar(tr("Edit"), this));
  mToolBarEdit->setObjectName("toolBarEdit");
  mToolBarEdit->addAction(mActionCut.data());
  mToolBarEdit->addAction(mActionCopy.data());
  mToolBarEdit->addAction(mActionPaste.data());
  mToolBarEdit->addAction(mActionRemove.data());
  mToolBarEdit->addAction(mActionRotateCcw.data());
  mToolBarEdit->addAction(mActionRotateCw.data());
  mToolBarEdit->addAction(mActionMirrorHorizontal.data());
  mToolBarEdit->addAction(mActionMirrorVertical.data());
  addToolBar(Qt::TopToolBarArea, mToolBarEdit.data());

  // View.
  mToolBarView.reset(new QToolBar(tr("View"), this));
  mToolBarView->setObjectName("toolBarView");
  mToolBarView->addAction(mActionGridProperties.data());
  mToolBarView->addAction(mActionShowPinNumbers.data());
  mToolBarView->addAction(mActionZoomIn.data());
  mToolBarView->addAction(mActionZoomOut.data());
  mToolBarView->addAction(mActionZoomFit.data());
  addToolBar(Qt::TopToolBarArea, mToolBarView.data());

  // Search.
  mToolBarSearch.reset(new SearchToolBar(this));
  mToolBarSearch->setObjectName("toolBarSearch");
  mToolBarSearch->setPlaceholderText(tr("Find symbol..."));
  mToolBarSearch->setCompleterListFunction(
      std::bind(&SchematicEditor::getSearchToolBarCompleterList, this));
  connect(mActionFind.data(), &QAction::triggered, mToolBarSearch.data(),
          &SearchToolBar::selectAllAndSetFocus);
  connect(mActionFindNext.data(), &QAction::triggered, mToolBarSearch.data(),
          &SearchToolBar::findNext);
  connect(mActionFindPrevious.data(), &QAction::triggered,
          mToolBarSearch.data(), &SearchToolBar::findPrevious);
  addToolBar(Qt::TopToolBarArea, mToolBarSearch.data());
  connect(mToolBarSearch.data(), &SearchToolBar::goToTriggered, this,
          &SchematicEditor::goToSymbol);

  // Command.
  mToolBarCommand.reset(new QToolBar(tr("Command"), this));
  mToolBarCommand->setObjectName("toolBarCommand");
  mToolBarCommand->addAction(mActionAbort.data());
  mToolBarCommand->addSeparator();
  addToolBarBreak(Qt::TopToolBarArea);
  addToolBar(Qt::TopToolBarArea, mToolBarCommand.data());
  mCommandToolBarProxy->setToolBar(mToolBarCommand.data());

  // Tools.
  mToolBarTools.reset(new QToolBar(tr("Tools"), this));
  mToolBarTools->setObjectName("toolBarTools");
  mToolBarTools->addAction(mActionToolSelect.data());
  mToolBarTools->addAction(mActionToolWire.data());
  mToolBarTools->addAction(mActionToolNetLabel.data());
  mToolBarTools->addAction(mActionToolPolygon.data());
  mToolBarTools->addAction(mActionToolText.data());
  mToolBarTools->addAction(mActionToolComponent.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolMeasure.data());
  addToolBar(Qt::LeftToolBarArea, mToolBarTools.data());

  // Components.
  mToolBarComponents.reset(new QToolBar(tr("Components"), this));
  mToolBarComponents->setObjectName("toolBarComponents");
  mToolBarComponents->addAction(mActionComponentResistor.data());
  mToolBarComponents->addAction(mActionComponentInductor.data());
  mToolBarComponents->addAction(mActionComponentCapacitorBipolar.data());
  mToolBarComponents->addAction(mActionComponentCapacitorUnipolar.data());
  mToolBarComponents->addAction(mActionComponentGnd.data());
  mToolBarComponents->addAction(mActionComponentVcc.data());
  addToolBarBreak(Qt::LeftToolBarArea);
  addToolBar(Qt::LeftToolBarArea, mToolBarComponents.data());
  updateComponentToolbarIcons();  // Load icons according workspace settings.
  connect(&mProject, &Project::normOrderChanged, this,
          &SchematicEditor::updateComponentToolbarIcons);
}

void SchematicEditor::createDockWidgets() noexcept {
  // Pages.
  mDockPages.reset(new SchematicPagesDock(
      mProject, mProjectEditor.getUndoStack(),
      mProjectEditor.getWorkspace().getSettings().themes.getActive(), this));
  connect(this, &SchematicEditor::activeSchematicChanged, mDockPages.data(),
          &SchematicPagesDock::setSelectedSchematic);
  connect(mDockPages.data(), &SchematicPagesDock::selectedSchematicChanged,
          this, &SchematicEditor::setActiveSchematicIndex);
  connect(mDockPages.data(), &SchematicPagesDock::addSchematicTriggered, this,
          &SchematicEditor::addSchematic);
  connect(mDockPages.data(), &SchematicPagesDock::removeSchematicTriggered,
          this, &SchematicEditor::removeSchematic);
  connect(mDockPages.data(), &SchematicPagesDock::renameSchematicTriggered,
          this, &SchematicEditor::renameSchematic);
  addDockWidget(Qt::LeftDockWidgetArea, mDockPages.data(), Qt::Vertical);

  // ERC Messages.
  mDockErc.reset(
      new RuleCheckDock(RuleCheckDock::Mode::ElectricalRuleCheck, this));
  mDockErc->setObjectName("dockErc");
  mDockErc->setApprovals(mProject.getErcMessageApprovals());
  connect(&mProject, &Project::ercMessageApprovalsChanged, mDockErc.data(),
          &RuleCheckDock::setApprovals);
  connect(mDockErc.data(), &RuleCheckDock::messageApprovalRequested,
          &mProjectEditor, &ProjectEditor::setErcMessageApproved);
  connect(&mProjectEditor, &ProjectEditor::ercFinished, mDockErc.data(),
          &RuleCheckDock::setMessages);
  addDockWidget(Qt::RightDockWidgetArea, mDockErc.data(), Qt::Vertical);

  // Set reasonable default dock size.
  resizeDocks(
      {
          mDockPages.data(),
          mDockErc.data(),
      },
      {
          120,
          150,
      },
      Qt::Horizontal);
}

void SchematicEditor::createMenus() noexcept {
  MenuBuilder mb(mUi->menuBar);

  // File.
  mb.newMenu(&MenuBuilder::createFileMenu);
  mb.addAction(mActionSaveProject);
  mb.addAction(mActionFileManager);
  mb.addSeparator();
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createExportMenu));
    smb.addAction(mActionExportPdf);
    smb.addAction(mActionExportImage);
    smb.addAction(mActionExportLppz);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createProductionDataMenu));
    smb.addAction(mActionGenerateBom);
  }
  mb.addAction(mActionOutputJobs);
  mb.addSeparator();
  mb.addAction(mActionPrint);
  mb.addAction(mActionOrderPcb);
  mb.addSeparator();
  mb.addAction(mActionCloseWindow);
  mb.addAction(mActionCloseProject);
  mb.addSeparator();
  mb.addAction(mActionQuit);

  // Edit.
  mb.newMenu(&MenuBuilder::createEditMenu);
  mb.addAction(mActionUndo);
  mb.addAction(mActionRedo);
  mb.addSeparator();
  mb.addAction(mActionSelectAll);
  mb.addSeparator();
  mb.addAction(mActionCut);
  mb.addAction(mActionCopy);
  mb.addAction(mActionPaste);
  mb.addAction(mActionRemove);
  mb.addSeparator();
  mb.addAction(mActionRotateCcw);
  mb.addAction(mActionRotateCw);
  mb.addAction(mActionMirrorHorizontal);
  mb.addAction(mActionMirrorVertical);
  mb.addAction(mActionResetAllTexts);
  mb.addSeparator();
  mb.addAction(mActionFind);
  mb.addAction(mActionFindNext);
  mb.addAction(mActionFindPrevious);
  mb.addSeparator();
  mb.addAction(mActionProperties);

  // View.
  mb.newMenu(&MenuBuilder::createViewMenu);
  mb.addAction(mActionGridProperties);
  mb.addAction(mActionGridIncrease.data());
  mb.addAction(mActionGridDecrease.data());
  mb.addSeparator();
  mb.addAction(mActionShowPinNumbers.data());
  mb.addSeparator();
  mb.addAction(mActionZoomIn);
  mb.addAction(mActionZoomOut);
  mb.addAction(mActionZoomFit);
  mb.addSeparator();
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createGoToDockMenu));
    smb.addAction(mActionDockPages);
    smb.addAction(mActionDockErc);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createDocksVisibilityMenu));
    smb.addAction(mDockPages->toggleViewAction());
    smb.addAction(mDockErc->toggleViewAction());
  }

  // Schematic.
  mb.newMenu(&MenuBuilder::createSchematicMenu);
  mb.addAction(mActionNewSheet);
  mb.addAction(mActionRenameSheet);
  mb.addAction(mActionRemoveSheet);

  // Project.
  mb.newMenu(&MenuBuilder::createProjectMenu);
  mb.addAction(mActionProjectSetup);
  mb.addSeparator();
  mb.addAction(mActionUpdateLibrary);

  // Tools.
  mb.newMenu(&MenuBuilder::createToolsMenu);
  mb.addAction(mActionToolSelect);
  mb.addAction(mActionToolWire);
  mb.addAction(mActionToolNetLabel);
  mb.addAction(mActionToolPolygon);
  mb.addAction(mActionToolText);
  mb.addAction(mActionToolComponent);
  mb.addSeparator();
  mb.addAction(mActionToolMeasure);

  // Help.
  mb.newMenu(&MenuBuilder::createHelpMenu);
  mb.addAction(mActionOnlineDocumentation);
  mb.addAction(mActionKeyboardShortcutsReference);
  mb.addAction(mActionWebsite);
  mb.addSeparator();
  mb.addAction(mActionAboutLibrePcb);
  mb.addAction(mActionAboutQt);
}

bool SchematicEditor::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyPressed(e);
}

bool SchematicEditor::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool SchematicEditor::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool SchematicEditor::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditor::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool SchematicEditor::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool SchematicEditor::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

void SchematicEditor::toolRequested(const QVariant& newTool) noexcept {
  // Note: Converting the QVariant to SchematicEditorFsmAdapter::Tool doesn't
  // work with some Qt versions, thus we convert to int instead. Fixed in:
  // https://codereview.qt-project.org/c/qt/qtbase/+/159277/
  switch (newTool.value<int>()) {
    case SchematicEditorFsm::State::SELECT:
      mFsm->processSelect();
      break;
    case SchematicEditorFsm::State::DRAW_WIRE:
      mFsm->processDrawWire();
      break;
    case SchematicEditorFsm::State::ADD_NETLABEL:
      mFsm->processAddNetLabel();
      break;
    case SchematicEditorFsm::State::ADD_COMPONENT:
      mFsm->processAddComponent();
      break;
    case SchematicEditorFsm::State::DRAW_POLYGON:
      mFsm->processDrawPolygon();
      break;
    case SchematicEditorFsm::State::ADD_TEXT:
      mFsm->processAddText();
      break;
    case SchematicEditorFsm::State::MEASURE:
      mFsm->processMeasure();
      break;
    default:
      qCritical() << "Unhandled switch-case in "
                     "SchematicEditor::toolRequested():"
                  << newTool;
      break;
  }
}

void SchematicEditor::addSchematic() noexcept {
  bool ok = false;
  QString name = QInputDialog::getText(this, tr("Add schematic page"),
                                       tr("Choose a name:"), QLineEdit::Normal,
                                       tr("New Page"), &ok);
  if (!ok) return;

  try {
    const QString dirName = FilePath::cleanFileName(
        name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (dirName.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Invalid name: '%1'").arg(name));
    }

    abortBlockingToolsInOtherEditors();  // Release undo stack.
    CmdSchematicAdd* cmd =
        new CmdSchematicAdd(mProject, dirName, ElementName(name));  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd);
    setActiveSchematicIndex(mProject.getSchematics().count() - 1);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::removeSchematic(int index) noexcept {
  Schematic* schematic = mProject.getSchematicByIndex(index);
  if (!schematic) return;

  try {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    CmdSchematicRemove* cmd = new CmdSchematicRemove(mProject, *schematic);
    mProjectEditor.getUndoStack().execCmd(cmd);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::renameSchematic(int index) noexcept {
  Schematic* schematic = mProject.getSchematicByIndex(index);
  if (!schematic) return;

  bool ok = false;
  QString name =
      QInputDialog::getText(this, tr("Rename sheet"), tr("Choose new name:"),
                            QLineEdit::Normal, *schematic->getName(), &ok);
  if (!ok) return;

  try {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    std::unique_ptr<CmdSchematicEdit> cmd(new CmdSchematicEdit(*schematic));
    cmd->setName(ElementName(cleanElementName(name)));  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd.release());
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

QList<SI_Symbol*> SchematicEditor::getSearchCandidates() noexcept {
  QList<SI_Symbol*> candidates;
  foreach (const Schematic* schematic, mProject.getSchematics()) {
    Q_ASSERT(schematic);
    candidates += schematic->getSymbols().values();
  }
  return candidates;
}

QStringList SchematicEditor::getSearchToolBarCompleterList() noexcept {
  QStringList list;
  foreach (SI_Symbol* symbol, getSearchCandidates()) {
    list.append(symbol->getName());
  }
  return list;
}

void SchematicEditor::goToSymbol(const QString& name, int index) noexcept {
  QList<SI_Symbol*> symbolCandidates;
  foreach (SI_Symbol* symbol, getSearchCandidates()) {
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
    Schematic& schematic = symbol->getSchematic();
    if (setActiveSchematicIndex(mProject.getSchematics().indexOf(&schematic)) &&
        mGraphicsScene) {
      mGraphicsScene->clearSelection();
      if (auto item = mGraphicsScene->getSymbols().value(symbol)) {
        item->setSelected(true);
        QRectF rect = item->mapRectToScene(item->childrenBoundingRect());
        // Zoom to a rectangle relative to the maximum graphics item dimension,
        // occupying 1/4th of the screen, but limiting the margin to 10mm.
        const qreal margin =
            std::min(1.5f * std::max(rect.size().width(), rect.size().height()),
                     Length::fromMm(10).toPx());
        rect.adjust(-margin, -margin, margin, margin);
        mUi->graphicsView->zoomToRect(rect);
      }
    }
  }
}

void SchematicEditor::updateEmptySchematicMessage() noexcept {
  Schematic* schematic = getActiveSchematic();
  mUi->msgEmptySchematic->setActive(schematic &&
                                    schematic->getSymbols().isEmpty());
}

void SchematicEditor::updateComponentToolbarIcons() noexcept {
  const QString suffix = useIeee315Symbols() ? "us.png" : "eu.png";
  mActionComponentResistor->setIcon(QIcon(":/img/library/resistor_" % suffix));
  mActionComponentInductor->setIcon(QIcon(":/img/library/inductor_" % suffix));
  mActionComponentCapacitorBipolar->setIcon(
      QIcon(":/img/library/bipolar_capacitor_" % suffix));
  mActionComponentCapacitorUnipolar->setIcon(
      QIcon(":/img/library/unipolar_capacitor_" % suffix));
}

void SchematicEditor::setGridProperties(const PositiveLength& interval,
                                        const LengthUnit& unit,
                                        Theme::GridStyle style,
                                        bool applyToSchematics) noexcept {
  if (mGraphicsScene) {
    mGraphicsScene->setGridInterval(interval);
    mGraphicsScene->setGridStyle(style);
  }

  mUi->statusbar->setLengthUnit(unit);

  if (applyToSchematics) {
    foreach (Schematic* schematic, mProject.getSchematics()) {
      schematic->setGridInterval(interval);
      schematic->setGridUnit(unit);
    }
  }
}

void SchematicEditor::execGridPropertiesDialog() noexcept {
  const Schematic* schematic = getActiveSchematic();
  if (schematic && mGraphicsScene) {
    GridSettingsDialog dialog(schematic->getGridInterval(),
                              schematic->getGridUnit(),
                              mGraphicsScene->getGridStyle(), this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const PositiveLength& interval, const LengthUnit& unit,
                   Theme::GridStyle style) {
              setGridProperties(interval, unit, style, false);
            });
    if (dialog.exec()) {
      setGridProperties(dialog.getInterval(), dialog.getUnit(),
                        dialog.getStyle(), true);
    }
  }
}

void SchematicEditor::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    QString projectName = FilePath::cleanFileName(
        *mProject.getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion = FilePath::cleanFileName(
        *mProject.getVersion(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath =
        QString("output/%1/%2_Schematics").arg(projectVersion, projectName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);

    // Copy all schematic pages to allow processing them in worker threads.
    const int count = mProject.getSchematics().count();
    QProgressDialog progress(tr("Preparing schematics..."), tr("Cancel"), 0,
                             count, this);
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
        getActiveSchematicIndex(), *mProject.getName(), 0, defaultFilePath,
        mProjectEditor.getWorkspace().getSettings().defaultLengthUnit.get(),
        mProjectEditor.getWorkspace().getSettings().themes.getActive(),
        "schematic_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mProjectEditor.getWorkspace().getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

bool SchematicEditor::useIeee315Symbols() const noexcept {
  foreach (const QString& norm, mProject.getNormOrder()) {
    if (norm.toLower() == "ieee 315") {
      return true;
    } else if (norm.toLower() == "iec 60617") {
      return false;
    }
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

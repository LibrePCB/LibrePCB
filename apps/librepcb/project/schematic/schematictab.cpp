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

#include "../../apptoolbox.h"
#include "../../guiapplication.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../uitypes.h"
#include "../projecteditor.h"
#include "../projectsmodel.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/graphics/graphicslayer.h>
#include <librepcb/editor/project/schematiceditor/fsm/schematiceditorfsm.h>
#include <librepcb/editor/project/schematiceditor/fsm/schematiceditorstate_drawpolygon.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>
#include <librepcb/editor/undostack.h>
#include <librepcb/editor/workspace/desktopservices.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

static ui::SchematicTool l2s(SchematicEditorFsmAdapter::Tool v) noexcept {
  if (v == SchematicEditorFsmAdapter::Tool::Wire) {
    return ui::SchematicTool::Wire;
  } else if (v == SchematicEditorFsmAdapter::Tool::NetLabel) {
    return ui::SchematicTool::Netlabel;
  } else if (v == SchematicEditorFsmAdapter::Tool::Polygon) {
    return ui::SchematicTool::Polygon;
  } else if (v == SchematicEditorFsmAdapter::Tool::Text) {
    return ui::SchematicTool::Text;
  } else if (v == SchematicEditorFsmAdapter::Tool::Component) {
    return ui::SchematicTool::Component;
  } else if (v == SchematicEditorFsmAdapter::Tool::Measure) {
    return ui::SchematicTool::Measure;
  } else {
    return ui::SchematicTool::Select;
  }
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
                           std::shared_ptr<ProjectEditor> prj,
                           int schematicIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, prj, schematicIndex, parent),
    mEditor(prj),
    mFsm(),
    mTool(Tool::None),
    mWireMode(SchematicEditorState_DrawWire::WireMode::HV),
    mLineWidth(0),
    mLineWidthUnit(app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mFilled(false),
    mCursorShape(Qt::ArrowCursor) {
  // Apply settings from schematic.
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mGridInterval = sch->getGridInterval();
  }

  // Connect undo stack.
  connect(&prj->getUndoStack(), &UndoStack::stateModified, this,
          &SchematicTab::requestRepaint);
  connect(mEditor.get(), &ProjectEditor::manualModificationsMade, this,
          &SchematicTab::uiDataChanged);

  // Refresh UI when ERC is completed to update execution error.
  connect(mEditor.get(), &ProjectEditor::ercFinished, this,
          &SchematicTab::uiDataChanged);

  // Build the whole schematic editor finite state machine.
  SchematicEditorFsm::Context fsmContext{
      mApp.getWorkspace(),
      mProject->getProject(),
      prj->getUndoStack(),
      qApp->activeWindow(),
      *this,
  };
  mFsm.reset(new SchematicEditorFsm(fsmContext));

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this,
          &SchematicTab::updateTheme);
  updateTheme();
}

SchematicTab::~SchematicTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData SchematicTab::getBaseUiData() const noexcept {
  auto sch = mProject->getProject().getSchematicByIndex(mObjIndex);

  ui::RuleCheckState ercState;
  if (!mProject->getErcMessages()) {
    ercState = ui::RuleCheckState::NotRunYet;
  } else {
    ercState = ui::RuleCheckState::UpToDate;
  }

  return ui::TabData{
      ui::TabType::Schematic,  // Type
      q2s(sch ? *sch->getName() : QString()),  // Title
      q2s(QPixmap(":/image.svg")),  // Icon
      mApp.getProjects().getIndexOf(mEditor),  // Project index
      ercState,  // Rule check state
      mProject->getErcMessages(),  // Rule check messages
      q2s(mEditor->getErcExecutionError()),  // Rule check execution error
      mEditor->canSave(),  // Can save
      true,  // Can export graphics
      mProject->getUndoStack().canUndo(),  // Can undo
      q2s(mProject->getUndoStack().getUndoCmdText()),  // Undo text
      mProject->getUndoStack().canRedo(),  // Can redo
      q2s(mProject->getUndoStack().getRedoCmdText()),  // Redo text
      true,  // Can cut/copy
      true,  // Can paste
      true,  // Can remove
      true,  // Can rotate
      true,  // Can mirror
  };
}

ui::SchematicTabData SchematicTab::getUiData() const noexcept {
  // const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  auto sch = mProject->getProject().getSchematicByIndex(mObjIndex);
  auto pinNumbersLayer =
      mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers);

  QString gridIntervalStr;
  if (sch) {
    const LengthUnit& unit = sch->getGridUnit();
    gridIntervalStr = Toolbox::floatToString(unit.convertToUnit(*mGridInterval),
                                             10, QLocale());
  }

  return ui::SchematicTabData{
      q2s(mBackgroundColor),  // Background color
      q2s(/*theme.getColor(Theme::Color::sSchematicBackground)
              .getSecondaryColor()*/
          QColor(Qt::black)),  // Overlay color (TODO)
      l2s(mGridStyle),  // Grid style
      q2s(gridIntervalStr),  // Grid interval
      sch ? l2s(sch->getGridUnit())
          : ui::LengthUnit::Millimeters,  // Length unit
      pinNumbersLayer && pinNumbersLayer->isVisible(),  // Show pin numbers
      l2s(mTool),  // Active tool
      l2s(mWireMode),  // Wire mode
      l2s(*mLineWidth),  // Line width
      l2s(mLineWidthUnit),  // Line width unit
      mFilled,  // Filled
      q2s(mCursorShape),  // Cursor
  };
}

void SchematicTab::setUiData(const ui::SchematicTabData& data) noexcept {
  auto sch = mProject->getProject().getSchematicByIndex(mObjIndex);

  mGridStyle = s2l(data.grid_style);
  const LengthUnit unit = s2l(data.unit);
  if (sch && (unit != sch->getGridUnit())) {
    sch->setGridUnit(unit);
    mEditor->setManualModificationsMade();
  }
  if (auto l = mLayerProvider->getLayer(Theme::Color::sSchematicPinNumbers)) {
    l->setVisible(data.show_pin_numbers);
  }
  emit wireModeRequested(s2l(data.wire_mode));
  emit filledRequested(data.filled);
  mLineWidthUnit = s2l(data.line_width_unit);
  if (auto l = s2ulength(data.line_width)) {
    emit lineWidthRequested(*l);
  }

  invalidateBackground();
  emit requestRepaint();
}

void SchematicTab::activate() noexcept {
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mScene.reset(new SchematicGraphicsScene(
        *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    emit requestRepaint();
  }
}

void SchematicTab::deactivate() noexcept {
  mScene.reset();
}

bool SchematicTab::actionTriggered(ui::ActionId id) noexcept {
  if (id == ui::ActionId::Save) {
    mEditor->saveProject();
    return true;
  } else if (id == ui::ActionId::SectionGridIntervalIncrease) {
    mGridInterval = PositiveLength(mGridInterval * 2);
    invalidateBackground();
    return true;
  } else if ((id == ui::ActionId::SectionGridIntervalDecrease) &&
             ((*mGridInterval % 2) == 0)) {
    mGridInterval = PositiveLength(mGridInterval / 2);
    invalidateBackground();
    return true;
  } else if (id == ui::ActionId::ExportPdf) {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
    return true;
  } else if (id == ui::ActionId::Print) {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
    return true;
  } else if (id == ui::ActionId::ToolSelect) {
    return mFsm->processSelect();
  } else if (id == ui::ActionId::ToolWire) {
    return mFsm->processDrawWire();
  } else if (id == ui::ActionId::ToolNetlabel) {
    return mFsm->processAddNetLabel();
  } else if (id == ui::ActionId::ToolPolygon) {
    return mFsm->processDrawPolygon();
  } else if (id == ui::ActionId::ToolText) {
    return mFsm->processAddText();
  } else if (id == ui::ActionId::ToolComponent) {
    return mFsm->processAddComponent();
  } else if (id == ui::ActionId::ToolMeasure) {
    return mFsm->processMeasure();
  }

  return GraphicsSceneTab::actionTriggered(id);
}

bool SchematicTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  static qint64 lastClickTime = 0;

  QTransform tf;
  tf.translate(mProjection.offset.x(), mProjection.offset.y());
  tf.scale(1 / mProjection.scale, 1 / mProjection.scale);
  const QPointF scenePosPx = tf.map(pos);

  QGraphicsSceneMouseEvent qe;
  qe.setButton(s2q(e.button));
  qe.setScenePos(scenePosPx);
  qe.setScreenPos(globalPos.toPoint());

  if (e.kind == slint::private_api::PointerEventKind::Down) {
    mMouseButtonDownScenePos[qe.button()] = scenePosPx;
    mMouseButtonDownScreenPos[qe.button()] = globalPos.toPoint();
  }

  qe.setButtonDownScenePos(qe.button(), mMouseButtonDownScenePos[qe.button()]);
  qe.setButtonDownScreenPos(qe.button(),
                            mMouseButtonDownScreenPos[qe.button()]);

  if (GraphicsSceneTab::processScenePointerEvent(pos, globalPos, e)) {
    return true;
  }

  bool isDoubleClick = false;
  if (e.kind == slint::private_api::PointerEventKind::Down) {
    const qint64 t = QDateTime::currentMSecsSinceEpoch();
    if (t - lastClickTime < 300) {
      isDoubleClick = true;
    }
    lastClickTime = t;
  }

  bool handled = false;
  if (isDoubleClick &&
      (e.button == slint::private_api::PointerEventButton::Left)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Left) &&
             (e.kind == slint::private_api::PointerEventKind::Down)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonPressed(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Left) &&
             (e.kind == slint::private_api::PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonReleased(qe);
  } else if ((e.button == slint::private_api::PointerEventButton::Right) &&
             (e.kind == slint::private_api::PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneRightMouseButtonReleased(qe);
  } else if (e.kind == slint::private_api::PointerEventKind::Move) {
    handled = mFsm->processGraphicsSceneMouseMoved(qe);
  }

  if (handled) {
    emit requestRepaint();
  }

  return handled;
}

/*******************************************************************************
 *  SchematicEditorFsmAdapter Methods
 ******************************************************************************/

Schematic* SchematicTab::fsmGetActiveSchematic() noexcept {
  return mProject->getProject().getSchematicByIndex(mObjIndex);
}

SchematicGraphicsScene* SchematicTab::fsmGetGraphicsScene() noexcept {
  return qobject_cast<SchematicGraphicsScene*>(mScene.get());
}

void SchematicTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mCursorShape = *shape;
  } else {
    mCursorShape = Qt::ArrowCursor;
  }
  emit uiDataChanged();
}

void SchematicTab::fsmSetViewGrayOut(bool grayOut) noexcept {
  /* TODO */
}

void SchematicTab::fsmSetViewInfoBoxText(const QString& text) noexcept {
  /* TODO */
}

void SchematicTab::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  /* TODO */
}

void SchematicTab::fsmSetSceneCursor(const Point& pos, bool cross,
                                     bool circle) noexcept {
  /* TODO */
}

QPainterPath SchematicTab::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return calcPosWithTolerance(pos, multiplier);
}

Point SchematicTab::fsmMapGlobalPosToScenePos(const QPoint& pos,
                                              bool boundToView,
                                              bool mapToGrid) const noexcept {
  return mapGlobalPosToScenePos(pos, boundToView, mapToGrid);
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

void SchematicTab::fsmSetTool(Tool tool, SchematicEditorState* state) noexcept {
  mTool = tool;

  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }

  if (tool == Tool::Wire) {
    SchematicEditorState_DrawWire* s =
        static_cast<SchematicEditorState_DrawWire*>(state);
    mWireMode = s->getWireMode();
    mFsmStateConnections.append(
        connect(s, &SchematicEditorState_DrawWire::wireModeChanged, this,
                [this](SchematicEditorState_DrawWire::WireMode m) {
                  mWireMode = m;
                  emit uiDataChanged();
                }));
    mFsmStateConnections.append(
        connect(this, &SchematicTab::wireModeRequested, s,
                &SchematicEditorState_DrawWire::setWireMode));
  } else if (tool == Tool::Polygon) {
    SchematicEditorState_DrawPolygon* s =
        static_cast<SchematicEditorState_DrawPolygon*>(state);

    mLineWidth = s->getLineWidth();
    mFsmStateConnections.append(
        connect(s, &SchematicEditorState_DrawPolygon::lineWidthChanged, this,
                [this](const UnsignedLength& w) {
                  mLineWidth = w;
                  emit uiDataChanged();
                }));
    mFsmStateConnections.append(
        connect(this, &SchematicTab::lineWidthRequested, s,
                &SchematicEditorState_DrawPolygon::setLineWidth));

    mFilled = s->getFilled();
    mFsmStateConnections.append(
        connect(s, &SchematicEditorState_DrawPolygon::filledChanged, this,
                [this](bool f) {
                  mFilled = f;
                  emit uiDataChanged();
                }));
    mFsmStateConnections.append(
        connect(this, &SchematicTab::filledRequested, s,
                &SchematicEditorState_DrawPolygon::setFilled));
  }

  emit uiDataChanged();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const LengthUnit* SchematicTab::getCurrentUnit() const noexcept {
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    return &sch->getGridUnit();
  } else {
    return nullptr;
  }
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

void SchematicTab::updateTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();

  mBackgroundColor =
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor();
  mGridColor =
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor();
  mGridStyle = theme.getSchematicGridStyle();

  invalidateBackground();
  emit uiDataChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

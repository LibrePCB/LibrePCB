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

#ifndef LIBREPCB_EDITOR_SCHEMATICTAB_H
#define LIBREPCB_EDITOR_SCHEMATICTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscenetab.h"

#include <librepcb/core/types/lengthunit.h>
#include <librepcb/editor/dialogs/graphicsexportdialog.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorfsmadapter.h>
#include <librepcb/editor/project/schematic/fsm/schematiceditorstate_drawwire.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeUnit;

namespace editor {

class GraphicsScene;
class GuiApplication;
class IF_GraphicsLayerProvider;
class SchematicEditorFsm;

/*******************************************************************************
 *  Class SchematicTab
 ******************************************************************************/

/**
 * @brief The SchematicTab class
 */
class SchematicTab final : public GraphicsSceneTab,
                           public SchematicEditorFsmAdapter {
  Q_OBJECT

public:
  // Signals
  Signal<SchematicTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  SchematicTab() = delete;
  SchematicTab(const SchematicTab& other) = delete;
  explicit SchematicTab(GuiApplication& app,
                        std::shared_ptr<ProjectEditor2> prj, int schematicIndex,
                        QObject* parent = nullptr) noexcept;
  virtual ~SchematicTab() noexcept;

  // General Methods
  ui::TabData getUiData() const noexcept override;
  ui::SchematicTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::SchematicTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  void triggerAsync(ui::Action a) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;
  bool processSceneKeyPressed(
      const slint::private_api::KeyEvent& e) noexcept override;
  bool processSceneKeyReleased(
      const slint::private_api::KeyEvent& e) noexcept override;

  // SchematicEditorFsmAdapter
  Schematic* fsmGetActiveSchematic() noexcept override;
  SchematicGraphicsScene* fsmGetGraphicsScene() noexcept override;
  void fsmSetViewCursor(
      const std::optional<Qt::CursorShape>& shape) noexcept override;
  void fsmSetViewGrayOut(bool grayOut) noexcept override;
  void fsmSetViewInfoBoxText(const QString& text) noexcept override;
  void fsmSetViewRuler(
      const std::optional<std::pair<Point, Point>>& pos) noexcept override;
  void fsmSetSceneCursor(const Point& pos, bool cross,
                         bool circle) noexcept override;
  QPainterPath fsmCalcPosWithTolerance(
      const Point& pos, qreal multiplier) const noexcept override;
  Point fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept override;
  void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept override;
  void fsmAbortBlockingToolsInOtherEditors() noexcept override;
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(SchematicEditorState_Select& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_DrawWire& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddNetLabel& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddComponent& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddText& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_Measure& state) noexcept override;
  void fsmSetFeatures(Features features) noexcept override;

  // Operator Overloadings
  SchematicTab& operator=(const SchematicTab& rhs) = delete;

signals:
  void wireModeRequested(SchematicEditorState_DrawWire::WireMode mode);
  void layerRequested(const Layer& layer);
  void lineWidthRequested(const UnsignedLength& width);
  void sizeRequested(const PositiveLength& size);
  void filledRequested(bool filled);
  void valueRequested(const QString& value);
  void attributeValueRequested(const QString& value);
  void attributeUnitRequested(const AttributeUnit* unit);

protected:
  const LengthUnit* getCurrentUnit() const noexcept override;
  void requestRepaint() noexcept override;

private:
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void applyTheme() noexcept;

private:
  std::shared_ptr<ProjectEditor2> mEditor;
  const int mObjIndex;
  Theme::GridStyle mGridStyle;
  QPointF mSceneImagePos;
  int mFrameIndex;

  // Current tool
  Features mToolFeatures;
  ui::EditorTool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  SchematicEditorState_DrawWire::WireMode mToolWireMode;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  UnsignedLength mToolLineWidth;
  LengthUnit mToolLineWidthUnit;
  PositiveLength mToolSize;
  LengthUnit mToolSizeUnit;
  bool mToolFilled;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  std::optional<QString> mToolAttributeValue;
  QString mToolAttributeValuePlaceholder;
  QList<const AttributeUnit*> mToolAttributeUnitsQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolAttributeUnits;
  const AttributeUnit* mToolAttributeUnit;

  QVector<QMetaObject::Connection> mFsmStateConnections;
  QScopedPointer<SchematicEditorFsm> mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

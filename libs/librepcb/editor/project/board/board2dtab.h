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

#ifndef LIBREPCB_EDITOR_BOARD2DTAB_H
#define LIBREPCB_EDITOR_BOARD2DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../backgroundimagesettings.h"
#include "../../dialogs/graphicsexportdialog.h"
#include "../../utils/dismissablemessagecontext.h"
#include "../../utils/lengtheditcontext.h"
#include "../../utils/searchcontext.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "fsm/boardeditorfsmadapter.h"
#include "fsm/boardeditorstate_drawtrace.h"
#include "windowtab.h"

#include <librepcb/core/geometry/zone.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Package;
class RuleCheckMessage;

namespace editor {

class BoardEditor;
class BoardEditorFsm;
class FootprintGraphicsItem;
class GraphicsLayerList;
class GraphicsLayersModel;
class ProjectEditor;
class SlintGraphicsView;

/*******************************************************************************
 *  Class Board2dTab
 ******************************************************************************/

/**
 * @brief The Board2dTab class
 */
class Board2dTab final : public WindowTab,
                         public BoardEditorFsmAdapter,
                         public IF_GraphicsViewEventHandler {
  Q_OBJECT

  struct DeviceMetadata {
    /// Device library element UUID
    Uuid deviceUuid;

    /// Device library element name
    QString deviceName;

    /// Package library element UUID
    Uuid packageUuid;

    /// Package library element name
    QString packageName;

    /// Whether this device has been added as a part in the component instance
    /// or not
    bool isListedInComponentInstance;
  };

  enum class PlaceComponentsMode {
    Single,
    Similar,
    All,
  };

public:
  // Signals
  Signal<Board2dTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  Board2dTab() = delete;
  Board2dTab(const Board2dTab& other) = delete;
  explicit Board2dTab(GuiApplication& app, BoardEditor& editor,
                      QObject* parent = nullptr) noexcept;
  ~Board2dTab() noexcept;

  // General Methods
  int getProjectIndex() const noexcept;
  int getProjectObjectIndex() const noexcept;
  ui::TabData getUiData() const noexcept override;
  void setUiData(const ui::TabData& data) noexcept override;
  ui::Board2dTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::Board2dTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  void trigger(ui::TabAction a) noexcept override;
  slint::Image renderScene(float width, float height,
                           int scene) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, slint::private_api::PointerEvent e) noexcept override;
  bool processSceneScrolled(
      const QPointF& pos,
      slint::private_api::PointerScrollEvent e) noexcept override;
  bool processSceneKeyEvent(
      const slint::private_api::KeyEvent& e) noexcept override;
  QSet<const Layer*> getVisibleCopperLayers() const noexcept;

  // IF_GraphicsViewEventHandler
  bool graphicsSceneKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // BoardEditorFsmAdapter
  BoardGraphicsScene* fsmGetGraphicsScene() noexcept override;
  bool fsmGetIgnoreLocks() const noexcept override;
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
  void fsmSetFeatures(Features features) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(BoardEditorState_Select& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddVia& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddStrokeText& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPlane& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawZone& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddHole& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddDevice& state) noexcept override;
  void fsmToolEnter(BoardEditorState_Measure& state) noexcept override;

  // Operator Overloadings
  Board2dTab& operator=(const Board2dTab& rhs) = delete;

signals:
  void wireModeRequested(BoardEditorState_DrawTrace::WireMode mode);
  void netRequested(bool autoNet, const std::optional<Uuid>& net);
  void layerRequested(const Layer& layer);
  void filledRequested(bool filled);
  void mirroredRequested(bool mirrored);
  void valueRequested(const QString& value);
  void zoneRuleRequested(Zone::Rule rule, bool enable);

private:
  void updateEnabledCopperLayers() noexcept;
  void loadLayersVisibility() noexcept;
  void storeLayersVisibility() noexcept;
  void updateMessages() noexcept;
  void highlightDrcMessage(const std::shared_ptr<const RuleCheckMessage>& msg,
                           bool zoomTo) noexcept;
  void clearDrcMarker() noexcept;
  void scheduleUnplacedComponentsUpdate() noexcept;
  void updateUnplacedComponents() noexcept;
  void restartIdleTimer() noexcept;
  void setSelectedUnplacedComponent(int index) noexcept;
  void setSelectedUnplacedComponentDevice(int index) noexcept;
  void setSelectedUnplacedComponentDeviceAndPackage(
      const std::optional<Uuid>& deviceUuid, Package* package,
      bool packageOwned) noexcept;
  void setSelectedUnplacedComponentFootprint(int index) noexcept;
  /**
   * @brief Get all available devices for a specific component instance
   *
   * @param cmp   The desired component instance.
   *
   * @return  Metadata of all available devices, and the list index of the
   *          best match / most relevant device.
   */
  std::pair<QList<DeviceMetadata>, int> getAvailableDevices(
      ComponentInstance& cmp) const noexcept;
  std::optional<Uuid> getSuggestedFootprint(
      const Uuid& libPkgUuid) const noexcept;
  void addUnplacedComponentsToBoard(PlaceComponentsMode mode) noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void execPickPlaceExportDialog() noexcept;
  void execD356NetlistExportDialog() noexcept;
  void execSpecctraExportDialog() noexcept;
  void execSpecctraImportDialog() noexcept;
  void goToDevice(const QString& name, int index) noexcept;
  bool toggleBackgroundImage() noexcept;
  void applyBackgroundImageSettings() noexcept;
  FilePath getBackgroundImageCacheDir() const noexcept;
  void applyTheme() noexcept;
  void requestRepaint() noexcept;

private:
  // References
  ProjectEditor& mProjectEditor;
  Project& mProject;
  BoardEditor& mBoardEditor;
  Board& mBoard;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;
  QVector<QMetaObject::Connection> mActiveConnections;

  // Message handles
  DismissableMessageContext mMsgEmptySchematics;
  DismissableMessageContext mMsgPlaceDevices;

  // State
  SearchContext mSearchContext;
  Theme::GridStyle mGridStyle;
  QPointF mSceneImagePos;
  bool mIgnorePlacementLocks;
  int mFrameIndex;

  // Current tool
  Features mToolFeatures;
  ui::EditorTool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  BoardEditorState_DrawTrace::WireMode mToolWireMode;
  QVector<std::pair<bool, std::optional<Uuid>>> mToolNetsQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolNets;
  std::pair<bool, std::optional<Uuid>> mToolNet;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  LengthEditContext mToolLineWidth;
  LengthEditContext mToolSize;
  LengthEditContext mToolDrill;
  bool mToolFilled;  // Also used for auto-width
  bool mToolMirrored;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  Zone::Rules mToolZoneRules;

  // Unplaced components
  QList<Uuid> mUnplacedComponents;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mUnplacedComponentsModel;
  int mUnplacedComponentIndex;
  QPointer<ComponentInstance> mUnplacedComponent;
  QList<DeviceMetadata> mUnplacedComponentDevices;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mUnplacedComponentDevicesModel;
  int mUnplacedComponentDeviceIndex;
  QPointer<Package> mUnplacedComponentPackage;
  bool mUnplacedComponentPackageOwned;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mUnplacedComponentFootprintsModel;
  int mUnplacedComponentFootprintIndex;
  std::unique_ptr<GraphicsScene> mUnplacedComponentGraphicsScene;
  std::unique_ptr<FootprintGraphicsItem> mUnplacedComponentGraphicsItem;
  QHash<Uuid, Uuid> mLastDeviceOfComponent;
  QHash<Uuid, Uuid> mLastFootprintOfPackage;
  std::unique_ptr<QTimer> mUnplacedComponentsUpdateTimer;

  // FSM
  QVector<QMetaObject::Connection> mFsmStateConnections;
  QScopedPointer<BoardEditorFsm> mFsm;

  // Objects in active state
  std::shared_ptr<GraphicsLayersModel> mLayersModel;
  std::unique_ptr<BoardGraphicsScene> mScene;
  std::unique_ptr<QTimer> mInputIdleTimer;
  std::unique_ptr<QGraphicsPathItem> mDrcLocationGraphicsItem;

  // Background image
  BackgroundImageSettings mBackgroundImageSettings;
  std::shared_ptr<QGraphicsPixmapItem> mBackgroundImageGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

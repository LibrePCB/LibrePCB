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

#ifndef LIBREPCB_EDITOR_PACKAGETAB_H
#define LIBREPCB_EDITOR_PACKAGETAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../3d/openglobject.h"
#include "../../dialogs/graphicsexportdialog.h"
#include "../../utils/lengtheditcontext.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../libraryeditortab.h"
#include "fsm/packageeditorfsmadapter.h"

#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/types/version.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Footprint;
class Layer;

namespace editor {

class CategoryTreeModel;
class FootprintListModel;
class GraphicsLayerList;
class GraphicsScene;
class LibraryEditor;
class LibraryElementCategoriesModel;
class OpenGlSceneBuilder;
class PackageEditorFsm;
class PackageModelListModel;
class PackagePadListModel;
class SlintGraphicsView;
class SlintOpenGlView;
struct OpenGlProjection;

/*******************************************************************************
 *  Class BackgroundImageSettings
 ******************************************************************************/

struct BackgroundImageSettings {
  bool enabled = true;  ///< Whether the background is enabled or not
  QImage image;  ///< The original loaded image
  Angle rotation;  ///< Rotation in scene
  QList<std::pair<QPointF, Point>> references;  ///< References in #image

  bool tryLoadFromDir(const FilePath& dir) noexcept;
  void saveToDir(const FilePath& dir) noexcept;
  QPixmap buildPixmap(const QColor& bgColor) const noexcept;
};

/*******************************************************************************
 *  Class PackageTab
 ******************************************************************************/

/**
 * @brief The PackageTab class
 */
class PackageTab final : public LibraryEditorTab,
                         public PackageEditorFsmAdapter,
                         public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<PackageTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  PackageTab() = delete;
  PackageTab(const PackageTab& other) = delete;
  explicit PackageTab(LibraryEditor& editor, std::unique_ptr<Package> pkg,
                      Mode mode, QObject* parent = nullptr) noexcept;
  ~PackageTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::PackageTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::PackageTabData& data) noexcept;
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
  bool requestClose() noexcept override;

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

  // PackageEditorFsmAdapter
  GraphicsScene* fsmGetGraphicsScene() noexcept override;
  PositiveLength fsmGetGridInterval() const noexcept override;
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
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmSetFeatures(Features features) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(PackageEditorState_Select& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawLine& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawRect& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawCircle& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawArc& state) noexcept override;
  void fsmToolEnter(PackageEditorState_AddNames& state) noexcept override;
  void fsmToolEnter(PackageEditorState_AddValues& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawText& state) noexcept override;
  void fsmToolEnter(PackageEditorState_AddPads& state) noexcept override;
  void fsmToolEnter(PackageEditorState_DrawZone& state) noexcept override;
  void fsmToolEnter(PackageEditorState_AddHoles& state) noexcept override;
  void fsmToolEnter(PackageEditorState_ReNumberPads& state) noexcept override;
  void fsmToolEnter(PackageEditorState_Measure& state) noexcept override;

  // Operator Overloadings
  PackageTab& operator=(const PackageTab& rhs) = delete;

signals:
  void layerRequested(const Layer& layer);
  void angleRequested(const Angle& angle);
  void ratioRequested(const UnsignedLimitedRatio& ratio);
  void filledRequested(bool filled);
  void grabAreaRequested(bool grabArea);
  void valueRequested(const QString& value);
  void hAlignRequested(const HAlign& align);
  void vAlignRequested(const VAlign& align);
  void packagePadRequested(const std::optional<Uuid>& pad);
  void componentSideRequested(FootprintPad::ComponentSide side);
  void shapeRequested(const ui::PadShape shape);
  void pressFitRequested(bool pressFit);
  void zoneLayerRequested(Zone::Layer layer, bool enable);
  void zoneRuleRequested(Zone::Rule rule, bool enable);

protected:
  std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() override;
  bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                   bool checkOnly) override;
  template <typename MessageType>
  bool autoFixHelper(const std::shared_ptr<const RuleCheckMessage>& msg,
                     bool checkOnly);
  template <typename MessageType>
  void autoFix(const MessageType& msg);
  template <typename MessageType>
  void fixPadFunction(const MessageType& msg);
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  void setCurrentFootprintIndex(int index) noexcept;
  void setCurrentModelIndex(int index) noexcept;
  void autoSelectCurrentModelIndex() noexcept;
  bool isWritable() const noexcept;
  void refreshUiData() noexcept;
  void commitUiData() noexcept;
  bool save() noexcept;
  void setGridInterval(const PositiveLength& interval) noexcept;
  bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void scheduleOpenGlSceneUpdate() noexcept;
  void updateOpenGlScene() noexcept;
  bool toggleBackgroundImage() noexcept;
  void applyBackgroundImageSettings() noexcept;
  FilePath getBackgroundImageCacheDir() const noexcept;
  void requestRepaint() noexcept;
  void applyTheme() noexcept;

private:
  // References
  std::unique_ptr<Package> mPackage;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;
  bool mOpenGlSceneRebuildScheduled;
  const bool mIsNewElement;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  bool mView3d;
  Theme::GridStyle mGridStyle;
  PositiveLength mGridInterval;
  LengthUnit mUnit;
  bool mChooseCategory;
  std::shared_ptr<PackageModel> mCurrentModel;
  std::unique_ptr<OpenGlProjection> mOpenGlProjection;
  QHash<OpenGlObject::Type, float> mAlpha;
  QStringList mOpenGlSceneBuilderErrors;
  QPointF mSceneImagePos;
  int mFrameIndex;

  // Library metadata to be applied
  slint::SharedString mName;
  slint::SharedString mNameError;
  ElementName mNameParsed;
  slint::SharedString mDescription;
  slint::SharedString mKeywords;
  slint::SharedString mAuthor;
  slint::SharedString mVersion;
  slint::SharedString mVersionError;
  Version mVersionParsed;
  bool mDeprecated;
  std::shared_ptr<LibraryElementCategoriesModel> mCategories;
  std::shared_ptr<CategoryTreeModel> mCategoriesTree;
  Package::AssemblyType mAssemblyType;
  std::shared_ptr<PackagePadListModel> mPads;
  std::shared_ptr<slint::SortModel<ui::PackagePadData>> mPadsSorted;
  slint::SharedString mNewPadName;
  slint::SharedString mNewPadNameError;
  std::shared_ptr<FootprintListModel> mFootprints;
  std::shared_ptr<PackageModelListModel> mModels;

  // Current tool
  Features mToolFeatures;
  ui::EditorTool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  LengthEditContext mToolLineWidth;
  LengthEditContext mToolSize;
  LengthEditContext mToolDrill;
  Angle mToolAngle;
  UnsignedLimitedRatio mToolRatio;
  bool mToolFilled;
  bool mToolGrabArea;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  Alignment mToolAlign;
  QVector<std::optional<Uuid>> mToolPackagePadsQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolPackagePads;
  std::optional<Uuid> mToolPackagePad;
  FootprintPad::ComponentSide mToolComponentSide;
  ui::PadShape mToolShape;
  bool mToolFiducial;
  bool mToolPressFit;
  Zone::Layers mToolZoneLayers;
  Zone::Rules mToolZoneRules;

  /// Editor state machine
  QVector<QMetaObject::Connection> mFsmStateConnections;
  std::unique_ptr<PackageEditorFsm> mFsm;

  // Objects in active state
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<SlintOpenGlView> mOpenGlView;
  std::unique_ptr<OpenGlSceneBuilder> mOpenGlSceneBuilder;
  std::unique_ptr<QTimer> mOpenGlSceneRebuildTimer;

  // Background image
  BackgroundImageSettings mBackgroundImageSettings;
  std::shared_ptr<QGraphicsPixmapItem> mBackgroundImageGraphicsItem;

  /// Broken interface detection
  bool mIsInterfaceBroken;
  QSet<Uuid> mOriginalPackagePadUuids;
  FootprintList mOriginalFootprints;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_EDITOR_SYMBOLTAB_H
#define LIBREPCB_EDITOR_SYMBOLTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../dialogs/graphicsexportdialog.h"
#include "../../utils/dismissablemessagecontext.h"
#include "../../utils/lengtheditcontext.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../libraryeditortab.h"
#include "fsm/symboleditorfsmadapter.h"

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

class Layer;
class Symbol;

namespace editor {

class CategoryTreeModel;
class GraphicsLayerList;
class GraphicsScene;
class LibraryElementCategoriesModel;
class SlintGraphicsView;
class SymbolEditorFsm;
class SymbolGraphicsItem;

/*******************************************************************************
 *  Class SymbolTab
 ******************************************************************************/

/**
 * @brief The SymbolTab class
 */
class SymbolTab final : public LibraryEditorTab,
                        public SymbolEditorFsmAdapter,
                        public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<SymbolTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  SymbolTab() = delete;
  SymbolTab(const SymbolTab& other) = delete;
  explicit SymbolTab(LibraryEditor& editor, std::unique_ptr<Symbol> sym,
                     Mode mode, QObject* parent = nullptr) noexcept;
  ~SymbolTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::SymbolTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::SymbolTabData& data) noexcept;
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

  // SymbolEditorFsmAdapter
  GraphicsScene* fsmGetGraphicsScene() noexcept override;
  SymbolGraphicsItem* fsmGetGraphicsItem() noexcept override;
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
  void fsmToolEnter(SymbolEditorState_Select& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawLine& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawRect& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawCircle& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawArc& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_AddNames& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_AddValues& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_DrawText& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_AddPins& state) noexcept override;
  void fsmToolEnter(SymbolEditorState_Measure& state) noexcept override;

  // Operator Overloadings
  SymbolTab& operator=(const SymbolTab& rhs) = delete;

signals:
  void layerRequested(const Layer& layer);
  void angleRequested(const Angle& angle);
  void filledRequested(bool filled);
  void grabAreaRequested(bool grabArea);
  void valueRequested(const QString& value);
  void hAlignRequested(const HAlign& align);
  void vAlignRequested(const VAlign& align);

protected:
  void watchedFilesModifiedChanged() noexcept override;
  void reloadFromDisk() override;
  std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() override;
  bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                   bool checkOnly) override;
  template <typename MessageType>
  bool autoFixHelper(const std::shared_ptr<const RuleCheckMessage>& msg,
                     bool checkOnly);
  template <typename MessageType>
  bool autoFix(const MessageType& msg);
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  bool isWritable() const noexcept;
  void refreshUiData() noexcept;
  void commitUiData() noexcept;
  bool save() noexcept;
  void memorizeInterface() noexcept;
  void updateWatchedFiles() noexcept;
  void setGridInterval(const PositiveLength& interval) noexcept;
  bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void requestRepaint() noexcept;
  void applyTheme() noexcept;

private:
  // References
  std::unique_ptr<Symbol> mSymbol;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;
  const bool mIsNewElement;

  // Message handles
  DismissableMessageContext mMsgImportPins;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  Theme::GridStyle mGridStyle;
  PositiveLength mGridInterval;
  LengthUnit mUnit;
  bool mChooseCategory;
  bool mCompactLayout;
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
  Angle mToolAngle;
  bool mToolFilled;
  bool mToolGrabArea;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  Alignment mToolAlign;

  /// Editor state machine
  QVector<QMetaObject::Connection> mFsmStateConnections;
  std::unique_ptr<SymbolEditorFsm> mFsm;

  // Objects in active state
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<SymbolGraphicsItem> mGraphicsItem;

  /// Broken interface detection
  bool mIsInterfaceBroken;
  QSet<Uuid> mOriginalSymbolPinUuids;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

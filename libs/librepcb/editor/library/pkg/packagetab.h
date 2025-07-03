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
#include "../../dialogs/graphicsexportdialog.h"
#include "../../utils/dismissablemessagecontext.h"
#include "../../utils/lengtheditcontext.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../libraryeditortab.h"

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
class Package;

namespace editor {

class CategoryTreeModel2;
class FootprintGraphicsItem;
class GraphicsLayerList;
class GraphicsLayerList;
class GraphicsScene;
class GraphicsScene;
class LibraryEditor2;
class LibraryElementCategoriesModel;
class PackagePadListModel2;
class SlintGraphicsView;
class SlintGraphicsView;

/*******************************************************************************
 *  Class PackageTab
 ******************************************************************************/

/**
 * @brief The PackageTab class
 */
class PackageTab final : public LibraryEditorTab,
                         public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<PackageTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  PackageTab() = delete;
  PackageTab(const PackageTab& other) = delete;
  explicit PackageTab(LibraryEditor2& editor, std::unique_ptr<Package> pkg,
                      bool wizardMode, QObject* parent = nullptr) noexcept;
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

  // Operator Overloadings
  PackageTab& operator=(const PackageTab& rhs) = delete;

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
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  bool isWritable() const noexcept;
  bool isInterfaceBroken() const noexcept;
  void refreshMetadata() noexcept;
  void commitMetadata() noexcept;
  bool save() noexcept;
  void setGridInterval(const PositiveLength& interval) noexcept;
  // bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
  //                               const QString& settingsKey) noexcept;
  void requestRepaint() noexcept;
  void applyTheme() noexcept;

private:
  // References
  std::unique_ptr<Package> mPackage;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;
  const bool mIsNewElement;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  Theme::GridStyle mGridStyle;
  PositiveLength mGridInterval;
  LengthUnit mUnit;
  bool mAddCategoryRequested;
  // bool mCompactLayout;
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

  // UI data
  std::shared_ptr<LibraryElementCategoriesModel> mCategories;
  std::shared_ptr<CategoryTreeModel2> mCategoriesTree;
  std::shared_ptr<PackagePadListModel2> mPads;

  // Current tool
  // Features mToolFeatures;
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
  // std::unique_ptr<SymbolEditorFsm> mFsm;

  // Objects in active state
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<FootprintGraphicsItem> mGraphicsItem;

  /// Broken interface detection
  QSet<Uuid> mOriginalPackagePadUuids;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

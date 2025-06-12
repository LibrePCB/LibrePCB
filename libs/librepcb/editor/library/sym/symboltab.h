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
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../libraryeditortab.h"

#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/version.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

class GraphicsLayerList;
class GraphicsScene;
class SlintGraphicsView;
class SymbolGraphicsItem;

/*******************************************************************************
 *  Class SymbolTab
 ******************************************************************************/

/**
 * @brief The SymbolTab class
 */
class SymbolTab final : public LibraryEditorTab,
                        public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<SymbolTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  SymbolTab() = delete;
  SymbolTab(const SymbolTab& other) = delete;
  explicit SymbolTab(LibraryEditor2& editor, std::unique_ptr<Symbol> sym,
                     bool wizardMode, QObject* parent = nullptr) noexcept;
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

  // Operator Overloadings
  SymbolTab& operator=(const SymbolTab& rhs) = delete;

protected:
  std::pair<RuleCheckMessageList, QSet<SExpression>> runChecksImpl() override;
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
  void refreshMetadata() noexcept;
  void commitMetadata() noexcept;
  bool save() noexcept;
  void requestRepaint() noexcept;
  void applyTheme() noexcept;

private:
  // References
  std::unique_ptr<Symbol> mSymbol;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;

  // State
  bool mWizardMode;
  Theme::GridStyle mGridStyle;
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

  // Objects in active state
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<SymbolGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

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
#include "../../windowtab.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/theme.h>
#include "../../widgets/if_graphicsvieweventhandler.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

class LibraryEditor2;
class SymbolGraphicsItem;
class SlintGraphicsView;
class GraphicsLayerList;
class GraphicsScene;

/*******************************************************************************
 *  Class SymbolTab
 ******************************************************************************/

/**
 * @brief The SymbolTab class
 */
class SymbolTab final : public WindowTab,
    public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<SymbolTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  SymbolTab() = delete;
  SymbolTab(const SymbolTab& other) = delete;
  explicit SymbolTab(GuiApplication& app, LibraryEditor2& editor,
                     std::unique_ptr<Symbol> sym, bool wizardMode,
                     QObject* parent = nullptr) noexcept;
  ~SymbolTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept;
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

private:
  void applyTheme() noexcept;
  void requestRepaint() noexcept;

private:
  // References
  LibraryEditor2& mEditor;
  std::unique_ptr<Symbol> mSymbol;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::unique_ptr<SlintGraphicsView> mView;

  // State
  bool mWizardMode;
  Theme::GridStyle mGridStyle;
  QPointF mSceneImagePos;
  int mFrameIndex;

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

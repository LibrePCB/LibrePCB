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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDVIA_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDVIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/project/board/items/bi_via.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Via;

namespace editor {

class BoardGraphicsScene;
class CmdBoardViaEdit;

/*******************************************************************************
 *  Class BoardEditorState_AddVia
 ******************************************************************************/

/**
 * @brief The "add via" state/tool of the board editor
 */
class BoardEditorState_AddVia final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_AddVia() = delete;
  BoardEditorState_AddVia(const BoardEditorState_AddVia& other) = delete;
  explicit BoardEditorState_AddVia(const Context& context) noexcept;
  virtual ~BoardEditorState_AddVia() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  bool getAutoDrillDiameter() const noexcept {
    return !mCurrentProperties.getDrillDiameter().has_value();
  }
  PositiveLength getDrillDiameter() const noexcept;
  void setDrillDiameter(const std::optional<PositiveLength>& diameter) noexcept;
  void saveDrillDiameterInBoard() noexcept;
  void saveDrillDiameterInNetClass() noexcept;
  bool getUseAutoSize() const noexcept {
    return !mCurrentProperties.getSize().has_value();
  }
  PositiveLength getSize() const noexcept;
  void setSize(const std::optional<PositiveLength>& size) noexcept;
  QVector<std::pair<Uuid, QString>> getAvailableNets() const noexcept;
  bool getUseAutoNet() const noexcept { return mUseAutoNetSignal; }
  std::optional<Uuid> getNet() const noexcept { return mCurrentNetSignal; }
  void setNet(bool autoNet, const std::optional<Uuid>& net) noexcept;

  // Operator Overloadings
  BoardEditorState_AddVia& operator=(const BoardEditorState_AddVia& rhs) =
      delete;

signals:
  void drillDiameterChanged(bool autoDrill, const PositiveLength& diameter);
  void sizeChanged(bool autoSize, const PositiveLength& size);
  void netChanged(bool autoNet, const std::optional<Uuid>& net);

private:  // Methods
  bool addVia(const Point& pos) noexcept;
  bool updatePosition(BoardGraphicsScene& scene, const Point& pos) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void applySelectedNetSignal() noexcept;
  void updateClosestNetSignal(BoardGraphicsScene& scene,
                              const Point& pos) noexcept;
  NetSignal* getCurrentNetSignal() const noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;

  // Current tool settings
  Via mCurrentProperties;

  /// Whether the net signal is determined automatically or not
  bool mUseAutoNetSignal;

  /// The current net signal of the via
  std::optional<Uuid> mCurrentNetSignal;

  /// Whether #mCurrentNetSignal contains an up-to-date closest net signal
  bool mClosestNetSignalIsUpToDate;

  // Information about the current via to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Via* mCurrentViaToPlace;
  std::unique_ptr<CmdBoardViaEdit> mCurrentViaEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

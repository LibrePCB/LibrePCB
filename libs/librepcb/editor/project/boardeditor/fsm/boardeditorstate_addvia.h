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

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Via;

namespace editor {

class CmdBoardViaEdit;
class PositiveLengthEdit;

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
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  BoardEditorState_AddVia& operator=(const BoardEditorState_AddVia& rhs) =
      delete;

private:  // Methods
  bool addVia(Board& board, const Point& pos) noexcept;
  bool updatePosition(Board& board, const Point& pos) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  bool fixPosition(Board& board, const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void updateShapeActionsCheckedState() noexcept;
  void sizeEditValueChanged(const PositiveLength& value) noexcept;
  void drillDiameterEditValueChanged(const PositiveLength& value) noexcept;
  void applySelectedNetSignal() noexcept;
  void updateClosestNetSignal(Board& board, const Point& pos) noexcept;
  NetSignal* getCurrentNetSignal() const noexcept;
  BI_Via* findVia(Board& board, const Point pos,
                  const QSet<const NetSignal*>& netsignals = {},
                  const QSet<BI_Via*>& except = {}) const noexcept;
  BI_FootprintPad* findPad(Board& board, const Point pos,
                           const QSet<const NetSignal*>& netsignals = {},
                           const QSet<BI_FootprintPad*>& except = {}) const
      noexcept;
  BI_NetLine* findNetLine(Board& board, const Point pos,
                          const QSet<const NetSignal*>& netsignals = {}) const
      noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  Via mLastViaProperties;

  /// Whether the net signal is determined automatically or not
  bool mUseAutoNetSignal;

  /// The current net signal of the via
  tl::optional<Uuid> mCurrentNetSignal;

  /// Whether #mCurrentNetSignal contains an up-to-date closest net signal
  bool mClosestNetSignalIsUpToDate;

  // Information about the current via to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Via* mCurrentViaToPlace;
  QScopedPointer<CmdBoardViaEdit> mCurrentViaEditCmd;

  // Widgets for the command toolbar
  QHash<int, QAction*> mShapeActions;
  QList<QAction*> mActionSeparators;
  QScopedPointer<QLabel> mSizeLabel;
  QScopedPointer<PositiveLengthEdit> mSizeEdit;
  QScopedPointer<QLabel> mDrillLabel;
  QScopedPointer<PositiveLengthEdit> mDrillEdit;
  QScopedPointer<QLabel> mNetSignalLabel;
  QScopedPointer<QComboBox> mNetSignalComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

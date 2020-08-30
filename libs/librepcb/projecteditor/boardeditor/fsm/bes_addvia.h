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

#ifndef LIBREPCB_PROJECT_BES_ADDVIA_H
#define LIBREPCB_PROJECT_BES_ADDVIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bes_base.h"

#include <librepcb/project/boards/items/bi_via.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PositiveLengthEdit;

namespace project {

class Board;
class BI_Via;
class CmdBoardViaEdit;

namespace editor {

/*******************************************************************************
 *  Class BES_AddVia
 ******************************************************************************/

/**
 * @brief The BES_AddVia class
 */
class BES_AddVia final : public BES_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BES_AddVia(BoardEditor& editor, Ui::BoardEditor& editorUi,
                      GraphicsView& editorGraphicsView, UndoStack& undoStack);
  ~BES_AddVia();

  // General Methods
  ProcRetVal process(BEE_Base* event) noexcept override;
  bool       entry(BEE_Base* event) noexcept override;
  bool       exit(BEE_Base* event) noexcept override;

private:
  // Private Types

  /// Internal FSM States (substates)
  enum SubState {
    SubState_Idle,           ///< idle state [initial state]
    SubState_PositioningVia  ///< in this state, an undo command is active!
  };

  // Private Methods
  ProcRetVal processSceneEvent(BEE_Base* event) noexcept;
  bool       addVia(Board& board) noexcept;
  bool       updateVia(Board& board, const Point& pos) noexcept;
  bool       fixVia(Board& board, const Point& pos) noexcept;
  void       updateShapeActionsCheckedState() noexcept;
  void       sizeEditValueChanged(const PositiveLength& value) noexcept;
  void drillDiameterEditValueChanged(const PositiveLength& value) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  NetSignal*       getClosestNetSignal(Board& board, const Point& pos) noexcept;
  QSet<NetSignal*> getNetSignalsAtScenePos(Board& board, const Point& pos,
                                           QSet<BI_Base*> except = {}) const
      noexcept;
  BI_Via* findVia(Board& board, const Point pos, NetSignal* netsignal = nullptr,
                  const QSet<BI_Via*>& except = {}) const noexcept;
  BI_FootprintPad* findPad(Board& board, const Point pos,
                           NetSignal*                    netsignal = nullptr,
                           const QSet<BI_FootprintPad*>& except    = {}) const
      noexcept;
  BI_NetPoint* findNetPoint(Board& board, const Point pos,
                            NetSignal*                netsignal = nullptr,
                            const QSet<BI_NetPoint*>& except    = {}) const
      noexcept;
  BI_NetLine* findNetLine(Board& board, const Point pos,
                          NetSignal* netsignal = nullptr) const noexcept;
  void        abortPlacement(const bool showErrorMessage = false) noexcept;

  // General Attributes
  SubState                        mSubState;
  QString                         mAutoText;
  BI_Via*                         mCurrentVia;
  BI_Via::Shape                   mCurrentViaShape;
  PositiveLength                  mCurrentViaSize;
  PositiveLength                  mCurrentViaDrillDiameter;
  NetSignal*                      mCurrentViaNetSignal;
  bool                            mFindClosestNetSignal;
  NetSignal*                      mLastClosestNetSignal;
  QScopedPointer<CmdBoardViaEdit> mViaEditCmd;

  // Widgets for the command toolbar
  QHash<int, QAction*> mShapeActions;
  QList<QAction*>      mActionSeparators;
  QLabel*              mSizeLabel;
  PositiveLengthEdit*  mSizeEdit;
  QLabel*              mDrillLabel;
  PositiveLengthEdit*  mDrillEdit;
  QLabel*              mNetSignalLabel;
  QComboBox*           mNetSignalComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BES_ADDVIA_H

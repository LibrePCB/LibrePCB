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

#ifndef LIBREPCB_PROJECT_BES_DRAWTRACE_H
#define LIBREPCB_PROJECT_BES_DRAWTRACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bes_base.h"

#include <librepcb/project/boards/items/bi_via.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class PositiveLengthEdit;

namespace project {

class NetSignal;
class BI_FootprintPad;
class BI_NetPoint;
class BI_NetLine;
class BI_NetLineAnchor;

namespace editor {

/*******************************************************************************
 *  Class BES_DrawTrace
 ******************************************************************************/

/**
 * @brief The BES_DrawTrace class
 */
class BES_DrawTrace final : public BES_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BES_DrawTrace(BoardEditor& editor, Ui::BoardEditor& editorUi,
                         GraphicsView& editorGraphicsView,
                         UndoStack&    undoStack);
  ~BES_DrawTrace();

  // General Methods
  ProcRetVal process(BEE_Base* event) noexcept override;
  bool       entry(BEE_Base* event) noexcept override;
  bool       exit(BEE_Base* event) noexcept override;

private:
  // Private Types

  /// Internal FSM States (substates)
  enum SubState {
    SubState_Idle,                ///< idle state [initial state]
    SubState_PositioningNetPoint  ///< in this state, an undo command is active!
  };

  /**
   * @brief The WireMode enum contains all available wire modes
   *
   * @note The first item must have the value 0!
   */
  enum WireMode {
    WireMode_HV = 0,    ///< horizontal - vertical [default]
    WireMode_VH,        ///< vertical - horizontal
    WireMode_9045,      ///< 90° - 45°
    WireMode_4590,      ///< 45° - 90°
    WireMode_Straight,  ///< straight
    WireMode_COUNT      ///< count of wire modes
  };

  // Private Methods
  ProcRetVal       processSubStateIdle(BEE_Base* event) noexcept;
  ProcRetVal       processSubStatePositioning(BEE_Base* event) noexcept;
  ProcRetVal       processIdleSceneEvent(BEE_Base* event) noexcept;
  ProcRetVal       processPositioningSceneEvent(BEE_Base* event) noexcept;
  bool             startPositioning(Board& board, const Point& pos,
                                    BI_NetPoint* fixedPoint = nullptr) noexcept;
  bool             addNextNetPoint(Board& board) noexcept;
  bool             abortPositioning(bool showErrMsgBox) noexcept;
  BI_Via*          findVia(Board& board, const Point& pos,
                           NetSignal* netsignal = nullptr) const noexcept;
  BI_FootprintPad* findPad(Board& board, const Point& pos,
                           GraphicsLayer* layer     = nullptr,
                           NetSignal*     netsignal = nullptr) const noexcept;
  BI_NetPoint*     findNetPoint(Board& board, const Point& pos,
                                GraphicsLayer*            layer     = nullptr,
                                NetSignal*                netsignal = nullptr,
                                const QSet<BI_NetPoint*>& except    = {}) const
      noexcept;
  BI_NetLine* findNetLine(Board& board, const Point& pos,
                          GraphicsLayer*           layer     = nullptr,
                          NetSignal*               netsignal = nullptr,
                          const QSet<BI_NetLine*>& except = {}) const noexcept;
  void        updateNetpointPositions(const Point& cursorPos) noexcept;
  void        layerComboBoxIndexChanged(int index) noexcept;
  void        updateShapeActionsCheckedState() noexcept;
  void        sizeEditValueChanged(const PositiveLength& value) noexcept;
  void        drillDiameterEditValueChanged(const PositiveLength& value) noexcept;
  void        wireWidthEditValueChanged(const PositiveLength& value) noexcept;
  void        snapCheckBoxCheckedChanged(bool checked) noexcept;
  void        updateWireModeActionsCheckedState() noexcept;
  Point calcMiddlePointPos(const Point& p1, const Point p2, WireMode mode) const
      noexcept;

  // General Attributes
  SubState          mSubState;          ///< the current substate
  WireMode          mCurrentWireMode;   ///< the current wire mode
  QString           mCurrentLayerName;  ///< the current board layer name
  bool              mAddVia;            ///< whether a via add is requested
  BI_Via::Shape     mCurrentViaShape;
  PositiveLength    mCurrentViaSize;
  PositiveLength    mCurrentViaDrillDiameter;
  QString           mViaLayerName;      ///< the name of the layer where the via
                                        ///< was started
  PositiveLength    mCurrentWidth;      ///< the current wire width
  bool              mCurrentSnapActive; ///< the current active snap to target
  BI_NetLineAnchor* mFixedStartAnchor;  ///< the fixed netline anchor (start
                                        ///< point of the line)
  BI_NetLine*  mPositioningNetLine1;    ///< line between fixed point and p1
  BI_NetPoint* mPositioningNetPoint1;   ///< the first netpoint to place
  BI_NetLine*  mPositioningNetLine2;    ///< line between p1 and p2
  BI_NetPoint* mPositioningNetPoint2;   ///< the second netpoint to place

  // Widgets for the command toolbar
  QHash<WireMode, QAction*> mWireModeActions;
  QList<QAction*>           mActionSeparators;
  QLabel*                   mLayerLabel;
  QComboBox*                mLayerComboBox;
  QHash<int, QAction*>      mShapeActions;
  QLabel*                   mSizeLabel;
  PositiveLengthEdit*       mSizeEdit;
  QLabel*                   mDrillLabel;
  PositiveLengthEdit*       mDrillEdit;
  QLabel*                   mWidthLabel;
  PositiveLengthEdit*       mWidthEdit;
  QLabel*                   mSnapLabel;
  QCheckBox*                mSnapCheckBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BES_DRAWTRACE_H

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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_DRAWTRACE_H
#define LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_DRAWTRACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/common/geometry/via.h>

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
class BI_Via;
class BI_NetSegment;

namespace editor {

/*******************************************************************************
 *  Class BoardEditorState_DrawTrace
 ******************************************************************************/

/**
 * @brief The "draw trace" state/tool of the board editor
 */
class BoardEditorState_DrawTrace final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_DrawTrace() = delete;
  BoardEditorState_DrawTrace(const BoardEditorState_DrawTrace& other) = delete;
  explicit BoardEditorState_DrawTrace(const Context& context) noexcept;
  virtual ~BoardEditorState_DrawTrace() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processKeyPressed(const QKeyEvent& e) noexcept override;
  virtual bool processKeyReleased(const QKeyEvent& e) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Operator Overloadings
  BoardEditorState_DrawTrace& operator=(const BoardEditorState_DrawTrace& rhs) =
      delete;

private:
  /// Internal FSM States (substates)
  enum SubState {
    SubState_Idle,  ///< idle state [initial state]
    SubState_Initializing,  ///< beginning to start
    SubState_PositioningNetPoint  ///< in this state, an undo command is active!
  };

  /**
   * @brief All available wire modes
   *
   * @note The first item must have the value 0!
   */
  enum WireMode {
    WireMode_HV = 0,  ///< horizontal - vertical [default]
    WireMode_VH,  ///< vertical - horizontal
    WireMode_9045,  ///< 90° - 45°
    WireMode_4590,  ///< 45° - 90°
    WireMode_Straight,  ///< straight
    WireMode_COUNT  ///< count of wire modes
  };

  /**
   * @brief Begin drawing the next BI_NetLine
   *
   * @param board On which board the new traces are drawn.
   * @param pos The position, where the tracing should begin. If necessary a new
   * BI_NetPoint is created.
   * @param fixedPoint the BI_NetPoint used as the start anchor, when beginning
   * a new trace
   * @return True, when the tracing is successfully started.
   */
  bool startPositioning(Board& board, const Point& pos,
                        BI_NetPoint* fixedPoint = nullptr) noexcept;

  /**
   * @brief Finalize the BI_NetLines and connect them to other
   * existing traces if necessary.
   * @param board On which board the drawing is finalized.
   * @return True, when the trace is successfully drawn. When the trace is
   * continued, return the result of startPositioning(). False when canceled or
   * an error occurred
   */
  bool addNextNetPoint(Board& board) noexcept;

  /**
   * @brief Abort or cancel the current drawing of the trace.
   * @param showErrMsgBox When true, show an error message in a pop-up box.
   */
  bool abortPositioning(bool showErrMsgBox) noexcept;

  /**
   * @brief Find a BI_Via at the given position on the board.
   * @param board The board on which to look.
   * @param pos The position at which to look.
   * @param netsignals  When not empty, only look for vias which are part of
   *                    one of these signals.
   * @param except A QSet of BI_Via that should be excluded when looking at the
   * target position.
   * @return A single BI_Via, if any. at the target position.
   */
  BI_Via* findVia(Board& board, const Point& pos,
                  const QSet<const NetSignal*>& netsignals = {},
                  const QSet<BI_Via*>& except = {}) const noexcept;

  /**
   * @brief Find a BI_FootprintPad at the given position on the board.
   * @param board The board on which to look.
   * @param pos The position at which to look.
   * @param layer When specified only look for BI_FootprintPad which are on that
   * layer
   * @param netsignals  When not empty, only look for pads which are part of
   *                    one of these signals.
   * @return A single BI_FootprintPad, if any. at the target position.
   */
  BI_FootprintPad* findPad(Board& board, const Point& pos,
                           GraphicsLayer* layer = nullptr,
                           const QSet<const NetSignal*>& netsignals = {}) const
      noexcept;

  /**
   * @brief Find a BI_NetPoint at the given position on the board.
   * @param board The board on which to look.
   * @param pos The position at which to look.
   * @param layer When specified only look for BI_NetPoint which are on that
   * layer
   * @param netsignals  When not empty, only look for netpoints which are part
   *                    of one of these signals.
   * @param except A QSet of BI_NetPoint that should be excluded when looking at
   * the target position.
   * @return A single BI_NetPoint, if any. at the target position.
   */
  BI_NetPoint* findNetPoint(Board& board, const Point& pos,
                            GraphicsLayer* layer = nullptr,
                            const QSet<const NetSignal*>& netsignals = {},
                            const QSet<BI_NetPoint*>& except = {}) const
      noexcept;

  /**
   * @brief Find a BI_NetLine at the given position on the board.
   * @param board The board on which to look.
   * @param pos The position at which to look.
   * @param layer When specified only look for BI_NetLine which are on that
   * layer
   * @param netsignals  When not empty, only look for netlines which are part
   *                    of one of these signals.
   * @param except A QSet of BI_NetLine that should be excluded when looking at
   * the target position.
   * @return A single BI_NetLine, if any. at the target position.
   */
  BI_NetLine* findNetLine(Board& board, const Point& pos,
                          GraphicsLayer* layer = nullptr,
                          const QSet<const NetSignal*>& netsignals = {},
                          const QSet<BI_NetLine*>& except = {}) const noexcept;

  BI_NetLineAnchor* findAnchorNextTo(
      Board& board, const Point& pos, const UnsignedLength& maxDistance,
      GraphicsLayer* layer = nullptr,
      const QSet<const NetSignal*>& netsignals = {}) const noexcept;

  /**
   * @brief Update the currently active traces according
   * to the set parameters.
   *
   * Uses the current mCursorPos to where the currently active trace is snapped
   * to and how its BI_NetLine are palced. Also determines whether a BI_Via
   * should be added or if the target anchor can provide the desired layer
   * change.
   */
  void updateNetpointPositions() noexcept;

  /**
   * @brief Sets the BI_Via of the currently active trace.
   *
   * When true, adds a BI_Via instead of the current last BI_NetPoint to the
   * currently active trace. Otherwise removes it if necessary and replaces it
   * again with a BI_NetPoint.
   * It also updates the BI_Via according to the currently selected parameters.
   *
   * @warning mPositioningNetPoint2 and mTempVia are exclusive. If one is set,
   * the other is nullptr and vice versa.
   *
   * @param isVisible Whether the BI_Via is shown or not
   */
  void showVia(bool isVisible) noexcept;

  BI_NetLineAnchor* combineAnchors(BI_NetLineAnchor& a, BI_NetLineAnchor& b);

  // Callback Functions for the Gui elements
  void layerComboBoxIndexChanged(int index) noexcept;
  void updateShapeActionsCheckedState() noexcept;
  void sizeEditValueChanged(const PositiveLength& value) noexcept;
  void drillDiameterEditValueChanged(const PositiveLength& value) noexcept;
  void wireWidthEditValueChanged(const PositiveLength& value) noexcept;
  void wireAutoWidthEditToggled(const bool checked) noexcept;
  void updateWireModeActionsCheckedState() noexcept;

  /**
   * @brief Calculate the 'middle point' of two point,
   * according to the chosen WireMode.
   * @param p1 Start point.
   * @param p2 End point.
   * @param mode The selected WireMode.
   * @return Middle Point.
   */
  Point calcMiddlePointPos(const Point& p1, const Point p2, WireMode mode) const
      noexcept;

  // State
  SubState mSubState;  ///< the current substate
  WireMode mCurrentWireMode;  ///< the current wire mode
  QString mCurrentLayerName;  ///< the current board layer name
  bool mAddVia;  ///< whether a via add is requested
  BI_Via* mTempVia;
  Via mCurrentViaProperties;  ///< The current Via properties
                              ///< diameter
  QString mViaLayerName;  ///< the name of the layer where the via
                          ///< was started
  Point mTargetPos;  ///< the current target position of the
                     ///< active trace

  Point mCursorPos;  ///< the current cursor position
  PositiveLength mCurrentWidth;  ///< the current wire width
  bool mCurrentAutoWidth;  ///< automatically adjust wire width
  bool mCurrentSnapActive;  ///< the current active snap to target
  BI_NetLineAnchor* mFixedStartAnchor;  ///< the fixed netline anchor (start
                                        ///< point of the line)
  BI_NetSegment* mCurrentNetSegment;  ///< the net segment that is currently
                                      ///< edited
  BI_NetLine* mPositioningNetLine1;  ///< line between fixed point and p1
  BI_NetPoint* mPositioningNetPoint1;  ///< the first netpoint to place
  BI_NetLine* mPositioningNetLine2;  ///< line between p1 and p2
  BI_NetPoint* mPositioningNetPoint2;  ///< the second netpoint to place

  // Widgets for the command toolbar
  QHash<WireMode, QAction*> mWireModeActions;
  QList<QAction*> mActionSeparators;
  QScopedPointer<QLabel> mLayerLabel;
  QScopedPointer<QComboBox> mLayerComboBox;
  QHash<int, QAction*> mShapeActions;
  QScopedPointer<QLabel> mSizeLabel;
  QScopedPointer<PositiveLengthEdit> mSizeEdit;
  QScopedPointer<QLabel> mDrillLabel;
  QScopedPointer<PositiveLengthEdit> mDrillEdit;
  QScopedPointer<QLabel> mWidthLabel;
  QScopedPointer<PositiveLengthEdit> mWidthEdit;
  QScopedPointer<QCheckBox> mAutoWidthEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif

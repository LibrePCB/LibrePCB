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

#ifndef LIBREPCB_EDITOR_BOARDEDITOR_H
#define LIBREPCB_EDITOR_BOARDEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class BoardPlaneFragmentsBuilder;
class OrderPcbApiRequest;
class Project;

namespace editor {

class Board2dTab;
class Board3dTab;
class Notification;
class ProjectEditor;
class RuleCheckMessagesModel;

/*******************************************************************************
 *  Class BoardEditor
 ******************************************************************************/

/**
 * @brief The BoardEditor class
 */
class BoardEditor final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<BoardEditor> onUiDataChanged;

  // Constructors / Destructor
  BoardEditor() = delete;
  BoardEditor(const BoardEditor& other) = delete;
  explicit BoardEditor(ProjectEditor& prjEditor, Board& board, int uiIndex,
                       QObject* parent = nullptr) noexcept;
  ~BoardEditor() noexcept;

  // General Methods
  ProjectEditor& getProjectEditor() noexcept { return mProjectEditor; }
  Board& getBoard() noexcept { return mBoard; }
  int getUiIndex() const noexcept { return mUiIndex; }
  void setUiIndex(int index) noexcept;
  ui::BoardData getUiData() const noexcept;
  void setUiData(const ui::BoardData& data) noexcept;
  bool isRebuildingPlanes() const noexcept;
  void schedulePlanesRebuild();
  void startPlanesRebuild(bool force = false) noexcept;
  void startDrc(bool quick) noexcept;
  void registerActiveTab(Board2dTab* tab) noexcept;
  void unregisterActiveTab(Board2dTab* tab) noexcept;
  void registerActiveTab(Board3dTab* tab) noexcept;
  void unregisterActiveTab(Board3dTab* tab) noexcept;
  void execBoardSetupDialog(bool switchToDrcSettings = false) noexcept;
  void execStepExportDialog() noexcept;

  /**
   * @brief Prepare the "Order PCB" feature (request server status)
   */
  void prepareOrderPcb() noexcept;

  /**
   * @brief Start the "Order PCB" upload
   *
   * @param openBrowser   Whether to open the web browser after the upload
   *                      or not.
   */
  void startOrderPcbUpload(bool openBrowser) noexcept;

  // Operator Overloadings
  BoardEditor& operator=(const BoardEditor& rhs) = delete;

signals:
  void uiIndexChanged();
  void planesRebuildStatusChanged();
  void planesUpdated();
  void drcMessageHighlightRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                    bool zoomTo);
  void aboutToBeDestroyed();

private:
  void setDrcResult(const BoardDesignRuleCheck::Result& result) noexcept;
  void registeredTabsModified() noexcept;
  void planesRebuildTimerTimeout() noexcept;

private:
  ProjectEditor& mProjectEditor;
  Project& mProject;
  Board& mBoard;
  int mUiIndex;

  // Plane fragments builder
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlanesBuilder;
  std::unique_ptr<QTimer> mPlanesRebuildTimer;
  qint64 mTimestampOfLastPlaneRebuild;

  // DRC
  std::unique_ptr<BoardDesignRuleCheck> mDrc;
  std::shared_ptr<Notification> mDrcNotification;
  uint mDrcUndoStackState;
  std::shared_ptr<RuleCheckMessagesModel> mDrcMessages;

  // Order PCB
  std::unique_ptr<OrderPcbApiRequest> mOrderRequest;  ///< May be `nullptr`
  QString mOrderStatus;  ///< Either error or status
  int mOrderUploadProgressPercent;  ///< -1 means "not in progress"
  bool mOrderOpenBrowser;
  QString mDrcExecutionError;

  // Registered active tabs
  QVector<QPointer<Board2dTab>> mActive2dTabs;
  QVector<QPointer<Board3dTab>> mActive3dTabs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

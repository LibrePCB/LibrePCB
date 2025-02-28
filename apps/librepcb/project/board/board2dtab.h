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

#ifndef LIBREPCB_PROJECT_BOARD2DTAB_H
#define LIBREPCB_PROJECT_BOARD2DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscenetab.h"

#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace app {

class GuiApplication;
class Notification;
class ProjectEditor;
class RuleCheckMessagesModel;

/*******************************************************************************
 *  Class Board2dTab
 ******************************************************************************/

/**
 * @brief The Board2dTab class
 */
class Board2dTab final : public GraphicsSceneTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  Board2dTab() = delete;
  Board2dTab(const Board2dTab& other) = delete;
  explicit Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                      int boardIndex, QObject* parent = nullptr) noexcept;
  virtual ~Board2dTab() noexcept;

  // General Methods
  ui::TabData getBaseUiData() const noexcept override;
  ui::Board2dTabData getUiData() const noexcept;
  void setUiData(const ui::Board2dTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  bool actionTriggered(ui::ActionId id) noexcept override;

  // Operator Overloadings
  Board2dTab& operator=(const Board2dTab& rhs) = delete;

protected:
  const LengthUnit* getCurrentUnit() const noexcept override;
  void requestRepaint() noexcept override;

private:
  void startDrc(bool quick) noexcept;
  void setDrcResult(const BoardDesignRuleCheck::Result& result) noexcept;
  void applyTheme() noexcept;

private:
  std::shared_ptr<ProjectEditor> mEditor;

  Theme::GridStyle mGridStyle;

  std::unique_ptr<BoardDesignRuleCheck> mDrc;
  std::shared_ptr<Notification> mDrcNotification;
  uint mDrcUndoStackState;
  std::shared_ptr<RuleCheckMessagesModel> mDrcMessages;
  QString mDrcExecutionError;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;

  int mFrameIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif

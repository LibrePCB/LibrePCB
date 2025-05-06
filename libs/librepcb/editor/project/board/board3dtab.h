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

#ifndef LIBREPCB_EDITOR_BOARD3DTAB_H
#define LIBREPCB_EDITOR_BOARD3DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Project;

namespace editor {

class BoardEditor;
class GuiApplication;
class OpenGlSceneBuilder;
class ProjectEditor;
class SlintOpenGlView;
struct OpenGlProjection;

/*******************************************************************************
 *  Class Board3dTab
 ******************************************************************************/

/**
 * @brief The Board3dTab class
 */
class Board3dTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<Board3dTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  Board3dTab() = delete;
  Board3dTab(const Board3dTab& other) = delete;
  explicit Board3dTab(GuiApplication& app, BoardEditor& editor,
                      QObject* parent = nullptr) noexcept;
  ~Board3dTab() noexcept;

  // General Methods
  int getProjectIndex() const noexcept;
  int getProjectObjectIndex() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::Board3dTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::Board3dTabData& data) noexcept;
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

  // Operator Overloadings
  Board3dTab& operator=(const Board3dTab& rhs) = delete;

private:
  void scheduleSceneRebuild() noexcept;
  void sceneRebuildTimerTimeout() noexcept;
  void requestRepaint() noexcept;

  // References
  ProjectEditor& mProjectEditor;
  Project& mProject;
  BoardEditor& mBoardEditor;
  Board& mBoard;

  // State
  std::unique_ptr<OpenGlProjection> mProjection;
  qint64 mTimestampOfLastSceneRebuild;
  QStringList mSceneBuilderErrors;
  int mFrameIndex;

  std::shared_ptr<SlintOpenGlView> mView;
  std::shared_ptr<OpenGlSceneBuilder> mSceneBuilder;
  std::unique_ptr<QTimer> mSceneRebuildTimer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

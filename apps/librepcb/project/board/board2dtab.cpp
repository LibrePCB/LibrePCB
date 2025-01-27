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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "board2dtab.h"

#include "../../apptoolbox.h"
#include "../../guiapplication.h"
#include "../../uitypes.h"
#include "../projecteditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/boardeditor/boardgraphicsscene.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

static QString getTitle(std::shared_ptr<ProjectEditor> prj, int boardIndex) {
  if (auto b = prj->getProject().getBoardByIndex(boardIndex)) {
    return *b->getName();
  }
  return QString();
}

Board2dTab::Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                       int boardIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, ui::TabType::Board2d, QPixmap(":/projects.png"), prj,
                     boardIndex, getTitle(prj, boardIndex), parent),
    mUiData{
        q2s(mBackgroundColor),  // Background color
        q2s(Qt::white),  // Overlay color
        ui::GridStyle::None,  // Grid style
      slint::SharedString(), // Grid interval
        ui::LengthUnit::Millimeters,  // Length unit
    } {
  // Apply theme.
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  mBackgroundColor =
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor();
  mGridColor =
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor();
  mGridStyle = theme.getBoardGridStyle();
  mUiData.grid_style = l2s(mGridStyle);
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    mGridInterval = brd->getGridInterval();
  }

  // Update UI data.
  mUiData.grid_interval = q2s(mGridInterval->toMmString());
}

Board2dTab::~Board2dTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Board2dTab::setUiData(const ui::Board2dTabData& data) noexcept {
  mUiData = data;

  mGridStyle = s2l(mUiData.grid_style);

  invalidateBackground();
}

void Board2dTab::activate() noexcept {
  if (auto brd = mProject->getProject().getBoardByIndex(mObjIndex)) {
    mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
    connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
            [this](BoardPlaneFragmentsBuilder::Result result) {
              if (result.applyToBoard()) {
                emit requestRepaint();
              }
            });
    mPlaneBuilder->start(*brd);
    mScene.reset(new BoardGraphicsScene(
        *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    mUiData.overlay_color = q2s(Qt::white);
    emit requestRepaint();
  }
}

void Board2dTab::deactivate() noexcept {
  mPlaneBuilder.reset();
  mScene.reset();
}

bool Board2dTab::actionTriggered(ui::ActionId id) noexcept {
  if (id == ui::ActionId::SectionGridIntervalIncrease) {
    mGridInterval = PositiveLength(mGridInterval * 2);
    mUiData.grid_interval = q2s(mGridInterval->toMmString());
    invalidateBackground();
    emit uiDataChanged();
    return true;
  } else   if ((id == ui::ActionId::SectionGridIntervalDecrease) && ((*mGridInterval % 2) == 0)) {
    mGridInterval = PositiveLength(mGridInterval / 2);
    mUiData.grid_interval = q2s(mGridInterval->toMmString());
    invalidateBackground();
    emit uiDataChanged();
    return true;
  }

  return GraphicsSceneTab::actionTriggered(id);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

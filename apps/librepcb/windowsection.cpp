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
#include "windowsection.h"

#include "apptoolbox.h"
#include "guiapplication.h"
#include "library/createlibrarytab.h"
#include "library/downloadlibrarytab.h"
#include "project/board/board2dtab.h"
#include "project/board/board3dtab.h"
#include "project/schematic/schematictab.h"
#include "windowtab.h"
#include "windowtabsmodel.h"
#include "windowtabsmodeladapter.h"

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

WindowSection::WindowSection(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent),
    mTabsModel(new WindowTabsModel(app, this)),
    mUiData{
        mTabsModel,
        std::make_shared<
            WindowTabsModelAdapter<CreateLibraryTab, ui::CreateLibraryTabData>>(
            mTabsModel, this),
        std::make_shared<WindowTabsModelAdapter<DownloadLibraryTab,
                                                ui::DownloadLibraryTabData>>(
            mTabsModel, this),
        std::make_shared<
            WindowTabsModelAdapter<SchematicTab, ui::SchematicTabData>>(
            mTabsModel, this),
        std::make_shared<
            WindowTabsModelAdapter<Board2dTab, ui::Board2dTabData>>(mTabsModel,
                                                                    this),
        std::make_shared<
            WindowTabsModelAdapter<Board3dTab, ui::Board3dTabData>>(mTabsModel,
                                                                    this),
        0,
        0} {
  connect(mTabsModel.get(), &WindowTabsModel::cursorCoordinatesChanged, this,
          &WindowSection::cursorCoordinatesChanged);
  connect(mTabsModel.get(), &WindowTabsModel::requestRepaint, this, [this]() {
    mUiData.frame++;
    uiDataChanged();
  });
  connect(mTabsModel.get(), &WindowTabsModel::uiDataChanged, this,
          &WindowSection::uiDataChanged);
}

WindowSection::~WindowSection() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::size_t WindowSection::getTabCount() const noexcept {
  return mTabsModel->row_count();
}

std::shared_ptr<WindowTab> WindowSection::getTab(int index) noexcept {
  return mTabsModel->getTab(index);
}

std::shared_ptr<WindowTab> WindowSection::getCurrentTab() noexcept {
  return mTabsModel->getTab(mUiData.current_tab_index);
}

std::shared_ptr<ProjectEditor> WindowSection::getCurrentProject() noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->getProject();
  }
  return nullptr;
}

void WindowSection::addTab(std::shared_ptr<WindowTab> tab) noexcept {
  mTabsModel->addTab(tab);
  setCurrentTab(mTabsModel->row_count() - 1);
}

void WindowSection::closeTab(int index) noexcept {
  mTabsModel->closeTab(index);

  int currentIndex = mUiData.current_tab_index;
  if (index < currentIndex) {
    --currentIndex;
  }
  setCurrentTab(
      std::min(currentIndex, static_cast<int>(mTabsModel->row_count()) - 1));
}

void WindowSection::setCurrentTab(int index) noexcept {
  mTabsModel->setCurrentTab(index);
  mUiData.current_tab_index = index;
  emit uiDataChanged();
  emit currentProjectChanged(getCurrentProject());
}

bool WindowSection::actionTriggered(ui::ActionId id) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    if (t->actionTriggered(id)) {
      return true;
    }
  }

  return false;
}

slint::Image WindowSection::renderScene(float width, float height) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    return t->renderScene(width, height);
  }
  return slint::Image();
}

bool WindowSection::processSceneDoubleClicked(float x, float y, float width,
                                              float height) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->processSceneDoubleClicked(x, y, width, height);
  }
  return false;
}

bool WindowSection::processScenePointerEvent(
    float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->processScenePointerEvent(x, y, width, height, e);
  }
  return false;
}

bool WindowSection::processSceneScrolled(
    float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->processSceneScrolled(x, y, width, height, e);
  }
  return false;
}

void WindowSection::zoomFit(float width, float height) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->zoomFit(width, height);
  }
}

void WindowSection::zoomIn(float width, float height) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->zoomIn(width, height);
  }
}

void WindowSection::zoomOut(float width, float height) noexcept {
  if (std::shared_ptr<WindowTab> t = getTab(mUiData.current_tab_index)) {
    return t->zoomOut(width, height);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

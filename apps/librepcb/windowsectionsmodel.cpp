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
#include "windowsectionsmodel.h"

#include "apptoolbox.h"
#include "guiapplication.h"
#include "project/projecteditor.h"
#include "windowsection.h"
#include "windowtab.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

WindowSectionsModel::WindowSectionsModel(GuiApplication& app,
                                         QObject* parent) noexcept
  : QObject(parent), mApp(app), mItems(), mCurrentSectionIndex(0) {
  splitSection(0);
}

WindowSectionsModel::~WindowSectionsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowSectionsModel::openCreateLibraryTab() noexcept {
  addTab(ui::TabType::CreateLibrary, nullptr, -1);
}

bool WindowSectionsModel::createLibrary(int sectionIndex) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    return s->createLibrary();
  }
  return false;
}

void WindowSectionsModel::openSchematic(std::shared_ptr<ProjectEditor> prj,
                                        int index) noexcept {
  addTab(ui::TabType::Schematic, prj, index);
}

void WindowSectionsModel::openBoard(std::shared_ptr<ProjectEditor> prj,
                                    int index) noexcept {
  addTab(ui::TabType::Board2d, prj, index);
}

void WindowSectionsModel::openBoard3dViewer(int sectionIndex) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    if (std::shared_ptr<WindowTab> t = s->getCurrentTab()) {
      if (auto prj = t->getProject()) {
        addTab(ui::TabType::Board3d, prj, t->getObjIndex());
      }
    }
  }
}

void WindowSectionsModel::splitSection(int sectionIndex) noexcept {
  const int newIndex = qBound(0, sectionIndex + 1, mItems.count());
  std::shared_ptr<WindowSection> s =
      std::make_shared<WindowSection>(mApp, this);
  connect(s.get(), &WindowSection::uiDataChanged, this,
          [this, newIndex]() { row_changed(newIndex); });
  connect(s.get(), &WindowSection::currentProjectChanged, this,
          &WindowSectionsModel::currentProjectChanged);
  connect(s.get(), &WindowSection::cursorCoordinatesChanged, this,
          &WindowSectionsModel::cursorCoordinatesChanged);
  mItems.insert(newIndex, s);
  row_added(newIndex, 1);
}

void WindowSectionsModel::closeSection(int sectionIndex) noexcept {
  if (mItems.count() <= 1) {
    return;  // Do not allow to close the last section.
  }
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    for (std::size_t i = 0; i < s->getTabCount(); ++i) {
      s->closeTab(i);
    }
    mItems.remove(sectionIndex);
    mCurrentSectionIndex = qBound(-1, mCurrentSectionIndex, mItems.count() - 1);
    row_removed(sectionIndex, 1);
    emit currentSectionIndexChanged(mCurrentSectionIndex);
    if (auto sNew = mItems.value(mCurrentSectionIndex)) {
      emit currentProjectChanged(sNew->getCurrentProject());
    }
  }
}

void WindowSectionsModel::setCurrentTab(int sectionIndex,
                                        int tabIndex) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    if (sectionIndex != mCurrentSectionIndex) {
      mCurrentSectionIndex = sectionIndex;
      emit currentSectionIndexChanged(mCurrentSectionIndex);
      emit currentProjectChanged(s->getCurrentProject());
    }

    s->setCurrentTab(tabIndex);
    row_changed(sectionIndex);  // TODO: signal
  }
}

void WindowSectionsModel::closeTab(int sectionIndex, int tabIndex) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->closeTab(tabIndex);
  }
}

slint::Image WindowSectionsModel::renderScene(int sectionIndex, float width,
                                              float height,
                                              int frame) noexcept {
  Q_UNUSED(frame);
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    return s->renderScene(width, height);
  }
  return slint::Image();
}

slint::private_api::EventResult WindowSectionsModel::processScenePointerEvent(
    int sectionIndex, float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    if ((e.kind == slint::private_api::PointerEventKind::Down) &&
        (sectionIndex != mCurrentSectionIndex)) {
      mCurrentSectionIndex = sectionIndex;
      emit currentSectionIndexChanged(mCurrentSectionIndex);
      emit currentProjectChanged(s->getCurrentProject());
    }

    if (s->processScenePointerEvent(x, y, width, height, e)) {
      row_changed(sectionIndex);
    }
  }
  return slint::private_api::EventResult::Accept;
}

slint::private_api::EventResult WindowSectionsModel::processSceneScrolled(
    int sectionIndex, float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    if (s->processSceneScrolled(x, y, width, height, e)) {
      row_changed(sectionIndex);
    }
  }
  return slint::private_api::EventResult::Accept;
}

void WindowSectionsModel::zoomFit(int sectionIndex, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->zoomFit(width, height);
  }
}

void WindowSectionsModel::zoomIn(int sectionIndex, float width,
                                 float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->zoomIn(width, height);
  }
}

void WindowSectionsModel::zoomOut(int sectionIndex, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->zoomOut(width, height);
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t WindowSectionsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::WindowSection> WindowSectionsModel::row_data(
    std::size_t i) const {
  if (std::shared_ptr<WindowSection> s = mItems.value(i)) {
    return s->getUiData();
  } else {
    return std::nullopt;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WindowSectionsModel::addTab(ui::TabType type,
                                 std::shared_ptr<ProjectEditor> prj,
                                 int objIndex) noexcept {
  const int sectionIndex = qBound(0, mCurrentSectionIndex, mItems.count() - 1);
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->addTab(type, prj, objIndex);
    setCurrentTab(sectionIndex, s->getTabCount() - 1);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

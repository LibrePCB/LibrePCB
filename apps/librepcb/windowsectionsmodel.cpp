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
  : QObject(parent), mApp(app), mItems(), mCurrentSectionId(1) {
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

void WindowSectionsModel::openSchematic(std::shared_ptr<ProjectEditor> prj,
                                        int index) noexcept {
  addTab(ui::TabType::Schematic, prj, index);
}

void WindowSectionsModel::openBoard(std::shared_ptr<ProjectEditor> prj,
                                    int index) noexcept {
  addTab(ui::TabType::Board2d, prj, index);
}

void WindowSectionsModel::openBoard3dViewer(int sectionId, int tab) noexcept {
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
    if (std::shared_ptr<WindowTab> t = s->getTab(tab)) {
      if (auto prj = t->getProject()) {
        addTab(ui::TabType::Board3d, prj, t->getObjIndex());
      }
    }
  }
}

void WindowSectionsModel::splitSection(int sectionId) noexcept {
  const int newIndex = qBound(0, mIndex.value(sectionId) + 1, mItems.count());
  const int newId = ++mNextId;
  std::shared_ptr<WindowSection> s =
      std::make_shared<WindowSection>(mApp, newId, this);
  connect(s.get(), &WindowSection::uiDataChanged, this,
          [this, newId]() { row_changed(mIndex.value(newId, -1)); });
  connect(s.get(), &WindowSection::currentProjectChanged, this,
          &WindowSectionsModel::currentProjectChanged);
  connect(s.get(), &WindowSection::cursorCoordinatesChanged, this,
          &WindowSectionsModel::cursorCoordinatesChanged);
  mItems.insert(newIndex, s);
  updateIndex();
  row_added(newIndex, 1);
}

void WindowSectionsModel::closeSection(int sectionId) noexcept {
  if (mItems.count() <= 1) {
    return;  // Do not allow to close the last section.
  }
  int sectionIndex;
  if (std::shared_ptr<WindowSection> s = getSection(sectionId, &sectionIndex)) {
    for (std::size_t i = 0; i < s->getTabCount(); ++i) {
      s->closeTab(i);
    }
    mItems.remove(sectionIndex);
    updateIndex();
    const int currentIndex = mIndex.value(mCurrentSectionId);
    if (auto s = mItems.value(qBound(-1, currentIndex, mItems.count() - 1))) {
      mCurrentSectionId = s->getId();
    } else {
      mCurrentSectionId = -1;
    }
    row_removed(sectionIndex, 1);
    emit currentSectionIdChanged(mCurrentSectionId);
    if (auto sNew = getSection(mCurrentSectionId)) {
      emit currentProjectChanged(sNew->getCurrentProject());
    }
  }
}

void WindowSectionsModel::setCurrentTab(int sectionId, int tab) noexcept {
  int sectionIndex;
  if (std::shared_ptr<WindowSection> s = getSection(sectionId, &sectionIndex)) {
    if (sectionIndex != mCurrentSectionId) {
      mCurrentSectionId = sectionId;
      emit currentSectionIdChanged(mCurrentSectionId);
      emit currentProjectChanged(s->getCurrentProject());
    }

    s->setCurrentTab(tab);
    row_changed(sectionIndex);  // TODO: signal
  }
}

void WindowSectionsModel::closeTab(int sectionId, int tab) noexcept {
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
    s->closeTab(tab);
  }
}

slint::Image WindowSectionsModel::renderScene(int sectionId, int tab,
                                              float width, float height,
                                              int frame) noexcept {
  Q_UNUSED(frame);
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
    return s->renderScene(tab, width, height);
  }
  return slint::Image();
}

slint::private_api::EventResult WindowSectionsModel::processScenePointerEvent(
    int sectionId, float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  int sectionIndex;
  if (std::shared_ptr<WindowSection> s = getSection(sectionId, &sectionIndex)) {
    if ((e.kind == slint::private_api::PointerEventKind::Down) &&
        (sectionId != mCurrentSectionId)) {
      mCurrentSectionId = sectionId;
      emit currentSectionIdChanged(mCurrentSectionId);
      emit currentProjectChanged(s->getCurrentProject());
    }

    if (s->processScenePointerEvent(x, y, width, height, e)) {
      row_changed(sectionIndex);
    }
  }
  return slint::private_api::EventResult::Accept;
}

slint::private_api::EventResult WindowSectionsModel::processSceneScrolled(
    int sectionId, float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  int sectionIndex;
  if (std::shared_ptr<WindowSection> s = getSection(sectionId, &sectionIndex)) {
    if (s->processSceneScrolled(x, y, width, height, e)) {
      row_changed(sectionIndex);
    }
  }
  return slint::private_api::EventResult::Accept;
}

void WindowSectionsModel::zoomFit(int sectionId, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
    s->zoomFit(width, height);
  }
}

void WindowSectionsModel::zoomIn(int sectionId, float width,
                                 float height) noexcept {
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
    s->zoomIn(width, height);
  }
}

void WindowSectionsModel::zoomOut(int sectionId, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = getSection(sectionId)) {
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
  const int sectionIndex =
      qBound(0, mIndex.value(mCurrentSectionId, -1), mItems.count() - 1);
  if (std::shared_ptr<WindowSection> s = mItems.value(sectionIndex)) {
    s->addTab(type, prj, objIndex);
    setCurrentTab(s->getId(), s->getTabCount() - 1);
  }
}

std::shared_ptr<WindowSection> WindowSectionsModel::getSection(
    int id, int* index) noexcept {
  const int i = mIndex.value(id, -1);
  if (index) *index = i;
  return mItems.value(i);
}

void WindowSectionsModel::updateIndex() noexcept {
  mIndex.clear();
  for (int i = 0; i < mItems.count(); ++i) {
    mIndex.insert(mItems.at(i)->getId(), i);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

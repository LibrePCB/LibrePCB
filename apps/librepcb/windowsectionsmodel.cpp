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
  : QObject(parent), mApp(app), mItems(), mCurrentSection(-1) {
}

WindowSectionsModel::~WindowSectionsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowSectionsModel::openSchematic(std::shared_ptr<ProjectEditor> prj,
                                        int index) noexcept {
  addTab(prj, ui::TabType::Schematic, index);
}

void WindowSectionsModel::openBoard(std::shared_ptr<ProjectEditor> prj,
                                    int index) noexcept {
  addTab(prj, ui::TabType::Board2d, index);
}

void WindowSectionsModel::openBoard3dViewer(int section, int tab) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    if (std::shared_ptr<WindowTab> t = s->getTab(tab)) {
      if (auto prj = t->getProject()) {
        addTab(prj, ui::TabType::Board3d, t->getObjIndex());
      }
    }
  }
}

void WindowSectionsModel::setCurrentTab(int section, int tab) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    if (section != mCurrentSection) {
      mCurrentSection = section;
      emit currentSectionChanged(mCurrentSection);
      emit currentProjectChanged(s->getCurrentProject());
    }

    s->setCurrentTab(tab);
    row_changed(section);  // TODO: signal
  }
}

void WindowSectionsModel::closeTab(int section, int tab) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    s->closeTab(tab);
    if (s->getTabCount() == 0) {
      if (section < mCurrentSection) {
        --mCurrentSection;
      }
      mItems.remove(section);
      row_removed(section, 1);
      for (int i = section; i < mItems.count(); ++i) {
        mItems[i]->setIndex(mItems[i]->getIndex() - 1);
        row_changed(i);
      }
      emit currentSectionChanged(mCurrentSection);
      auto sNew = mItems.value(mCurrentSection);
      emit currentProjectChanged(sNew ? sNew->getCurrentProject() : nullptr);
    }
  }
}

slint::Image WindowSectionsModel::renderScene(int section, int tab, float width,
                                              float height,
                                              int frame) noexcept {
  Q_UNUSED(frame);
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    return s->renderScene(tab, width, height);
  }
  return slint::Image();
}

slint::private_api::EventResult WindowSectionsModel::processScenePointerEvent(
    int section, float x, float y, float width, float height,
    slint::private_api::PointerEvent e) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    if ((e.kind == slint::private_api::PointerEventKind::Down) &&
        (section != mCurrentSection)) {
      mCurrentSection = section;
      emit currentSectionChanged(mCurrentSection);
      emit currentProjectChanged(s->getCurrentProject());
    }

    if (s->processScenePointerEvent(x, y, width, height, e)) {
      row_changed(section);
    }
  }
  return slint::private_api::EventResult::Accept;
}

slint::private_api::EventResult WindowSectionsModel::processSceneScrolled(
    int section, float x, float y, float width, float height,
    slint::private_api::PointerScrollEvent e) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    if (s->processSceneScrolled(x, y, width, height, e)) {
      row_changed(section);
    }
  }
  return slint::private_api::EventResult::Accept;
}

void WindowSectionsModel::zoomFit(int section, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    s->zoomFit(width, height);
  }
}

void WindowSectionsModel::zoomIn(int section, float width,
                                 float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    s->zoomIn(width, height);
  }
}

void WindowSectionsModel::zoomOut(int section, float width,
                                  float height) noexcept {
  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
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

void WindowSectionsModel::addTab(std::shared_ptr<ProjectEditor> prj,
                                 ui::TabType type, int objIndex) noexcept {
  // Determine section.
  int section = 0;
  if (mItems.count() < 2) {
    section = mItems.count();
    std::shared_ptr<WindowSection> s =
        std::make_shared<WindowSection>(mApp, section, this);
    connect(s.get(), &WindowSection::uiDataChanged, this,
            [this](int index) { row_changed(index); });
    connect(s.get(), &WindowSection::currentProjectChanged, this,
            &WindowSectionsModel::currentProjectChanged);
    connect(s.get(), &WindowSection::cursorCoordinatesChanged, this,
            &WindowSectionsModel::cursorCoordinatesChanged);
    mItems.append(s);
    row_added(section, 1);
  } else {
    for (int i = 0; i < mItems.count(); ++i) {
      section += mItems[i]->getTabCount();
    }
    section %= 2;
  }

  if (std::shared_ptr<WindowSection> s = mItems.value(section)) {
    s->addTab(prj, type, objIndex);
    setCurrentTab(section, s->getTabCount() - 1);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

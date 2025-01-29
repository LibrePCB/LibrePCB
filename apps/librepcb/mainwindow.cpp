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
#include "mainwindow.h"

#include "apptoolbox.h"
#include "guiapplication.h"
#include "notificationsmodel.h"
#include "project/projecteditor.h"
#include "project/projectsmodel.h"
#include "windowsectionsmodel.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/newprojectwizard/newprojectwizard.h>
#include <librepcb/editor/project/outputjobsdialog/outputjobsdialog.h>
#include <librepcb/editor/workspace/desktopservices.h>
#include <librepcb/editor/workspace/initializeworkspacewizard/initializeworkspacewizard.h>

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

MainWindow::MainWindow(GuiApplication& app,
                       slint::ComponentHandle<ui::AppWindow> win, int index,
                       QObject* parent) noexcept
  : QObject(parent),
    mIndex(index),
    mSettingsPrefix(QString("window_%1").arg(index + 1)),
    mApp(app),
    mSections(new WindowSectionsModel(app, win->global<ui::Data>(),
                                      mSettingsPrefix, this)),
    mWindow(win),
    mWidget(static_cast<QWidget*>(slint::cbindgen_private::slint_qt_get_widget(
        &mWindow->window().window_handle()))) {
  Q_ASSERT(mWidget);

  // Register Slint callbacks.
  mWindow->window().on_close_requested(
      std::bind(&MainWindow::closeRequested, this));

  // Set global data.
  const ui::Data& d = mWindow->global<ui::Data>();
  d.set_current_page(ui::MainPage::Home);
  d.set_sections(mSections);
  d.set_current_section_index(0);
  d.set_cursor_coordinates(slint::SharedString());
  d.set_ignore_placement_locks(false);
  d.set_unread_notifications_count(
      mApp.getNotifications().getUnreadNotificationsCount());
  d.set_current_progress_notification_index(
      mApp.getNotifications().getCurrentProgressIndex());

  // Bind global data to signals.
  connect(&mApp.getNotifications(),
          &NotificationsModel::unreadNotificationsCountChanged, this,
          [this](int count) {
            mWindow->global<ui::Data>().set_unread_notifications_count(count);
          });
  connect(&mApp.getNotifications(),
          &NotificationsModel::currentProgressIndexChanged, this,
          [this](int index) {
            mWindow->global<ui::Data>().set_current_progress_notification_index(
                index);
          });
  connect(mSections.get(), &WindowSectionsModel::currentProjectChanged, this,
          &MainWindow::setCurrentProject);
  connect(mSections.get(), &WindowSectionsModel::cursorCoordinatesChanged, this,
          [&d](const Point& pos, const LengthUnit& unit) {
            d.set_cursor_coordinates(
                q2s(QString("X: %1 Y: %2")
                        .arg(unit.convertToUnit(pos.getX()), 10, 'f',
                             unit.getReasonableNumberOfDecimals())
                        .arg(unit.convertToUnit(pos.getY()), 10, 'f',
                             unit.getReasonableNumberOfDecimals())));
          });

  // Register global callbacks.
  const ui::Backend& b = mWindow->global<ui::Backend>();
  b.on_action_triggered(std::bind(&MainWindow::actionTriggered, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
  b.on_key_pressed(
      std::bind(&MainWindow::keyPressed, this, std::placeholders::_1));
  b.on_project_item_doubleclicked(std::bind(
      &MainWindow::projectItemDoubleClicked, this, std::placeholders::_1));
  b.on_schematic_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      mSections->openSchematic(prj, index);
    }
  });
  b.on_board_clicked([this](int projectIndex, int index) {
    if (auto prj = mApp.getProjects().getProject(projectIndex)) {
      mSections->openBoard(prj, index);
    }
  });
  b.on_tab_clicked(std::bind(&WindowSectionsModel::setCurrentTab,
                             mSections.get(), std::placeholders::_1,
                             std::placeholders::_2));
  b.on_tab_close_clicked(std::bind(&WindowSectionsModel::closeTab,
                                   mSections.get(), std::placeholders::_1,
                                   std::placeholders::_2));
  b.on_render_scene(std::bind(
      &WindowSectionsModel::renderScene, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  b.on_scene_pointer_event([this](int sectionIndex, float x, float y,
                                  const slint::Point<float>& scenePos,
                                  slint::private_api::PointerEvent e) {
    // const auto winPos = mWindow->window().position();
    QPointF globalPos(scenePos.x + x, scenePos.y + y);
    if (QWidget* win = qApp->activeWindow()) {
      globalPos = win->mapToGlobal(globalPos);
    }
    return mSections->processScenePointerEvent(sectionIndex, QPointF(x, y),
                                               globalPos, e);
  });
  b.on_scene_scrolled(std::bind(&WindowSectionsModel::processSceneScrolled,
                                mSections.get(), std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3,
                                std::placeholders::_4));
  b.on_scene_zoom_fit_clicked(std::bind(
      &WindowSectionsModel::zoomFit, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_in_clicked(std::bind(
      &WindowSectionsModel::zoomIn, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_scene_zoom_out_clicked(std::bind(
      &WindowSectionsModel::zoomOut, mSections.get(), std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));
  b.on_open_url([this](const slint::SharedString& url) {
    DesktopServices ds(mApp.getWorkspace().getSettings());
    ds.openUrl(QUrl(s2q(url)));
  });

  // Show window.
  mWindow->show();

  // Load window state.
  QSettings cs;
  mWidget->restoreGeometry(
      cs.value(mSettingsPrefix % "/geometry").toByteArray());
}

MainWindow::~MainWindow() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool MainWindow::isCurrentWindow() const noexcept {
  return mWidget->isActiveWindow();
}

void MainWindow::setCurrentPage(ui::MainPage page) noexcept {
  mWindow->global<ui::Data>().set_current_page(page);
}

void MainWindow::popUpNotifications() noexcept {
  mWindow->fn_open_notifications_popup();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

slint::CloseRequestResponse MainWindow::closeRequested() noexcept {
  // Save window state.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/geometry", mWidget->saveGeometry());

  return slint::CloseRequestResponse::HideWindow;
}

bool MainWindow::actionTriggered(ui::ActionId id, int sectionIndex) noexcept {
  if (mSections->actionTriggered(id, sectionIndex)) {
    return true;
  } else if (id == ui::ActionId::ProjectNew) {
    newProject();
    return true;
  } else if (id == ui::ActionId::ProjectOpen) {
    setCurrentProject(mApp.getProjects().openProject());
    return true;
  } else if (id == ui::ActionId::ProjectImportExamples) {
    const QString msg =
        tr("This downloads some example projects from the internet and copies "
           "them into the workspace to help you evaluating LibrePCB with real "
           "projects.") %
        "\n\n" %
        tr("Once you don't need them anymore, just delete the examples "
           "directory to get rid of them.");
    const int ret = QMessageBox::information(
        qApp->activeWindow(), tr("Add Example Projects"), msg,
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok) {
      InitializeWorkspaceWizardContext ctx(this);
      ctx.setWorkspacePath(mApp.getWorkspace().getPath());
      ctx.installExampleProjects();
    }
    return true;
  } else if (id == ui::ActionId::ProjectImportEagle) {
    newProject(true);
    return true;
  } else if (id == ui::ActionId::ProjectOpenOutputJobs) {
    if (std::shared_ptr<ProjectEditor> editor =
            mApp.getProjects().getProject(sectionIndex)) {
      OutputJobsDialog dlg(mApp.getWorkspace().getSettings(),
                           editor->getProject(), editor->getUndoStack(),
                           mSettingsPrefix % "/output_jobs_dialog", mWidget);
      dlg.exec();
      return true;
    }
  } else if (id == ui::ActionId::WindowNew) {
    mApp.createNewWindow(
        mWindow->global<ui::Data>().get_current_project_index());
    return true;
  } else if (id == ui::ActionId::WindowClose) {
    mWindow->hide();  // TODO: Remove from GuiApplication
    return true;
  } else if (id == ui::ActionId::CopyApplicationDetailsIntoClipboard) {
    QApplication::clipboard()->setText(
        s2q(mWindow->global<ui::Data>().get_about_librepcb_details()));
    return true;
  } else if (mApp.actionTriggered(id, sectionIndex)) {
    return true;
  }

  qWarning() << "Unhandled UI action:" << static_cast<int>(id);
  return false;
}

slint::private_api::EventResult MainWindow::keyPressed(
    const slint::private_api::KeyEvent& e) noexcept {
  if ((std::string_view(e.text) == "f") && (e.modifiers.control)) {
    mWindow->invoke_focus_search();
    return slint::private_api::EventResult::Accept;
  }

  qDebug() << "Unhandled UI key event:" << e.text.data();
  return slint::private_api::EventResult::Reject;
}

void MainWindow::projectItemDoubleClicked(
    const slint::SharedString& path) noexcept {
  const FilePath fp(s2q(path));
  if (!fp.isValid()) {
    qWarning() << "Invalid file path:" << path.data();
    return;
  }
  if ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz")) {
    setCurrentProject(mApp.getProjects().openProject(fp));
    mWindow->global<ui::Data>().set_current_page(ui::MainPage::Project);
  } else {
    DesktopServices ds(mApp.getWorkspace().getSettings());
    ds.openLocalPath(fp);
  }
}

void MainWindow::setCurrentProject(
    std::shared_ptr<ProjectEditor> prj) noexcept {
  if (prj) {
    mWindow->global<ui::Data>().set_current_project_index(
        mApp.getProjects().getIndexOf(prj));
  }
}

std::shared_ptr<ProjectEditor> MainWindow::getCurrentProject() noexcept {
  return mApp.getProjects().getProject(
      mWindow->global<ui::Data>().get_current_project_index());
}

void MainWindow::newProject(bool eagleImport,
                            const FilePath& parentDir) noexcept {
  const NewProjectWizard::Mode mode = eagleImport
      ? NewProjectWizard::Mode::EagleImport
      : NewProjectWizard::Mode::NewProject;
  NewProjectWizard wizard(mApp.getWorkspace(), mode, qApp->activeWindow());
  if (parentDir.isValid()) {
    wizard.setLocationOverride(parentDir);
  }
  if (wizard.exec() == QWizard::Accepted) {
    try {
      std::unique_ptr<Project> project = wizard.createProject();  // can throw
      const FilePath fp = project->getFilepath();
      project.reset();  // Release lock.
      setCurrentProject(mApp.getProjects().openProject(fp));
    } catch (const Exception& e) {
      QMessageBox::critical(qApp->activeWindow(),
                            tr("Could not create project"), e.getMsg());
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

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

#include <librepcb/core/application.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindow::MainWindow(Workspace& ws, QObject* parent) noexcept
  : QObject(parent), mWorkspace(ws), mWindow(ui::AppWindow::create()) {
  mWindow->set_window_title(
      QString("LibrePCB %1").arg(Application::getVersion()).toUtf8().data());
  mWindow->set_workspace_path(ws.getPath().toNative().toUtf8().data());

  QObject::connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanProgressUpdate,
                   this, [this](int progress) {
                     mWindow->set_status_progress(progress / qreal(100));
                   });
  ws.getLibraryDb().startLibraryRescan();

  mWindow->global<ui::LengthEditGlobals>().on_parse_length_input(
      [](slint::SharedString text, slint::SharedString unit) {
        ui::LengthEditParseResult res{false, text, unit};
        try {
          QString value = text.begin();
          foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
            foreach (const QString& suffix, unit.getUserInputSuffixes()) {
              if (value.endsWith(suffix)) {
                value.chop(suffix.length());
                res.evaluatedUnit = unit.toShortStringTr().toStdString();
              }
            }
          }
          Length l = Length::fromMm(value);
          value = l.toMmString();
          if (value.endsWith(".0")) {
            value.chop(2);
          }
          res.evaluatedValue = value.toStdString();
          res.valid = true;
        } catch (const Exception& e) {
        }
        return res;
      });

  mWindow->on_close([&] { slint::quit_event_loop(); });

  mWindow->show();
}

MainWindow::~MainWindow() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

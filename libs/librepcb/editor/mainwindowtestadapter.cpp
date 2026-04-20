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
#include "mainwindowtestadapter.h"

#include "guiapplication.h"
#include "mainwindow.h"
#include "project/projecteditor.h"
#include "windowsection.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MainWindowTestAdapter::MainWindowTestAdapter(GuiApplication& app,
                                             MainWindow& win,
                                             QWidget* parent) noexcept
  : QWidget(parent), mApp(app), mWindow(win) {
  setObjectName("testAdapter");
  resize(1, 1);  // Do not occupy parts of the UI.

  connect(&mApp.getWorkspace().getLibraryDb(), &WorkspaceLibraryDb::scanStarted,
          this, [this]() { mLibraryScanFinished = false; });
  connect(&mApp.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanFinished, this,
          [this]() { mLibraryScanFinished = true; });

  startMcpServer();
}

MainWindowTestAdapter::~MainWindowTestAdapter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVariant MainWindowTestAdapter::trigger(QVariant action) noexcept {
  if (action == "workspace-switch") {
    emit actionTriggered(ui::Action::WorkspaceSwitch);
  } else if (action == "workspace-settings") {
    emit actionTriggered(ui::Action::WorkspaceSettings);
  } else if (action == "project-new") {
    emit actionTriggered(ui::Action::ProjectNew);
  } else if (action == "project-open") {
    emit actionTriggered(ui::Action::ProjectOpen);
  } else if (action == "schematic-add-component-dialog") {
    QMetaObject::invokeMethod(
        this,
        [this]() {
          mWindow.triggerSchematic(0, 0, ui::SchematicAction::Open);
          mWindow.triggerTab(0, 1, ui::TabAction::ToolComponent);
        },
        Qt::QueuedConnection);
  } else if (action == "schematic-export-image-dialog") {
    QMetaObject::invokeMethod(
        this,
        [this]() {
          mWindow.triggerSchematic(0, 0, ui::SchematicAction::Open);
          mWindow.triggerTab(0, 1, ui::TabAction::ExportImage);
        },
        Qt::QueuedConnection);
  } else if (action == "schematic-export-pdf-dialog") {
    QMetaObject::invokeMethod(
        this,
        [this]() {
          mWindow.triggerSchematic(0, 0, ui::SchematicAction::Open);
          mWindow.triggerTab(0, 1, ui::TabAction::ExportPdf);
        },
        Qt::QueuedConnection);
  } else if (action == "board-export-image-dialog") {
    QMetaObject::invokeMethod(
        this,
        [this]() {
          mWindow.triggerBoard(0, 0, ui::BoardAction::Open2d);
          mWindow.triggerTab(0, 1, ui::TabAction::ExportImage);
        },
        Qt::QueuedConnection);
  } else if (action == "board-export-pdf-dialog") {
    QMetaObject::invokeMethod(
        this,
        [this]() {
          mWindow.triggerBoard(0, 0, ui::BoardAction::Open2d);
          mWindow.triggerTab(0, 1, ui::TabAction::ExportPdf);
        },
        Qt::QueuedConnection);
  } else {
    qCritical() << "Unknown action triggered:" << action;
  }

  return QVariant();
}

QVariant MainWindowTestAdapter::getOpenProjects(QVariant) const noexcept {
  QVariantList root;
  for (auto prjEditor : mApp.getProjects()) {
    QVariantMap prjObj;
    prjObj["name"] = *prjEditor->getProject().getName();
    prjObj["path"] = prjEditor->getProject().getFilepath().toStr();
    root.append(prjObj);
  }
  return root;
}

void MainWindowTestAdapter::startMcpServer() noexcept {
  const int port = 19999;
  mMcpServer = new QTcpServer(this);
  connect(mMcpServer, &QTcpServer::newConnection, this,
          &MainWindowTestAdapter::handleMcpConnection);
  if (!mMcpServer->listen(QHostAddress::Any, port)) {
    qWarning() << "MCP server failed to listen on port" << port << ":"
               << mMcpServer->errorString();
  } else {
    qInfo() << "MCP server listening on port" << port;
  }
}

void MainWindowTestAdapter::handleMcpConnection() noexcept {
  while (mMcpServer->hasPendingConnections()) {
    QTcpSocket* socket = mMcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this,
            [this, socket]() { handleMcpData(socket); });
    connect(socket, &QTcpSocket::disconnected, socket,
            &QTcpSocket::deleteLater);
  }
}

void MainWindowTestAdapter::handleMcpData(QTcpSocket* socket) noexcept {
  while (socket->canReadLine()) {
    const QByteArray line = socket->readLine().trimmed();
    if (line.isEmpty()) continue;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (doc.isNull()) {
      socket->write(
          QJsonDocument(QJsonObject{{"ok", false}, {"error", err.errorString()}})
              .toJson(QJsonDocument::Compact) +
          "\n");
      continue;
    }

    const QJsonObject req = doc.object();
    const QString cmd = req["command"].toString();
    QString error;

    if (cmd == "show-about-panel") {
      QMetaObject::invokeMethod(
          this, [this]() { mWindow.showPanelPage(ui::PanelPage::About); },
          Qt::QueuedConnection);
    } else if (cmd == "switch-tab") {
      const int section = req.value("section").toInt(0);
      const int index = req.value("index").toInt(0);
      QMetaObject::invokeMethod(
          this,
          [this, section, index]() {
            if (auto s = mWindow.mSections->value(section)) {
              auto data = s->getUiData();
              data.current_tab_index = index;
              s->setUiData(data);
            }
          },
          Qt::QueuedConnection);
    } else {
      error = "Unknown command: " + cmd;
    }

    QJsonObject resp;
    resp["ok"] = error.isEmpty();
    if (!error.isEmpty()) resp["error"] = error;
    socket->write(QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n");
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

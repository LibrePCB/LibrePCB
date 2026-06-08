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
#include "boardeditorstate_autorouter.h"

#include "../../../notification.h"
#include "../../../undostack.h"
#include "../../cmd/cmdboardspecctraimport.h"

#include <librepcb/core/autorouter/onlineautorouter.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardspecctraexport.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_Autorouter::BoardEditorState_Autorouter(
    const Context& context) noexcept
  : BoardEditorState(context) {
}

BoardEditorState_Autorouter::~BoardEditorState_Autorouter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_Autorouter::entry() noexcept {
  mRouters.clear();
  for (const WorkspaceSettings::ApiEndpoint& cfg :
       mContext.workspace.getSettings().apiEndpoints.get()) {
    if (auto ep = ApiEndpoint::get(cfg.url)) {
      auto future = ep->requestAutorouteInfo();
      future
          .then(this,
                [this, url = cfg.url](
                    const ApiEndpoint::AutorouteInfoResult& result) {
                  for (const auto& router : result.routers) {
                    mRouters.append(Router{
                        url,
                        router.id,
                        url.toString() % "|" % router.id,
                        router.name,
                    });
                  }
                  if (!result.routers.isEmpty()) {
                    emit routersChanged(mRouters);
                  }
                  if (mRouterId.isEmpty() && (!mRouters.isEmpty())) {
                    setRouterId(mRouters.first().uniqueId);
                  }
                })
          .onFailed(this, [this](const ApiEndpoint::Exception& e) {
            // TODO
          });
      mStatusRequests.append(future);
    }
  }

  mAdapter.fsmToolEnter(*this);
  return true;
}

bool BoardEditorState_Autorouter::exit() noexcept {
  while (!mStatusRequests.isEmpty()) {
    mStatusRequests.takeLast().cancel();
  }
  mAutorouter.reset();
  if (mNotification) {
    mNotification->dismiss();
    mNotification.reset();
  }
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_Autorouter::processAbortCommand() noexcept {
  return false;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void BoardEditorState_Autorouter::setRouterId(const QString& id) noexcept {
  if (id == mRouterId) {
    return;
  }

  mRouterId = id;
  emit routerChanged(mRouterId);
}

void BoardEditorState_Autorouter::start() noexcept {
  qDebug() << "start";
  mAutorouter.reset();
  if (mNotification) {
    mNotification->dismiss();
    mNotification.reset();
  }

  const Router* router = nullptr;
  for (auto& obj : mRouters) {
    if (obj.uniqueId == mRouterId) {
      router = &obj;
      break;
    }
  }
  if (!router) {
    return;
  }

  std::shared_ptr<ApiEndpoint> ep = ApiEndpoint::get(router->endpoint);
  if (!ep) {
    return;
  }

  try {
    BoardSpecctraExport dsnExport(mContext.board);
    const QByteArray dsn = dsnExport.generate();  // can throw

    mNotification = std::make_shared<Notification>(
        ui::NotificationType::Progress,
        tr("Routing '%1'...").arg(*mContext.project.getName()),
        tr("Autorouting board '%1' with %2.")
            .arg(*mContext.board.getName(), router->name),
        QString(), QString(), true);
    mAdapter.fsmPushNotification(mNotification);

    mAutorouter = std::make_unique<OnlineAutorouter>(
        ep, router->id, std::make_shared<MessageLogger>());
    connect(mAutorouter.get(), &Autorouter::statusNotification, this,
            &BoardEditorState_Autorouter::statusReceived);
    mAutorouter->start(dsn);
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardEditorState_Autorouter::statusReceived(
    const Autorouter::Status& status) noexcept {
  if (mNotification) {
    mNotification->setProgress(status.progressPercent);
  }

  if (status.state == Autorouter::State::Succeeded) {
    try {
      std::unique_ptr<SExpression> root = SExpression::parse(
          status.ses, FilePath(), SExpression::Mode::Permissive);  // can throw
      mContext.undoStack.execCmd(new CmdBoardSpecctraImport(
          mContext.board, *root,
          std::make_shared<MessageLogger>()));  // can throw
    } catch (const Exception& e) {
      // TODO
    }
  }

  if (status.state != Autorouter::State::Running) {
    if (mNotification) {
      mNotification->dismiss();
      mNotification.reset();
    }
    mAutorouter.reset();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

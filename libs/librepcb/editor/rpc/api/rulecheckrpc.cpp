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
#include "rulecheckrpc.h"

#include "../../project/board/boardeditor.h"
#include "../../project/projecteditor.h"
#include "../rpcnotification.h"
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

/// The object handle of an ERC message: A hash of the approval, i.e. a
/// deterministic identifier which survives re-runs of the ERC as long as
/// the message (resp. its approval) stays the same.
static QByteArray getObj(const RuleCheckMessage& msg) noexcept {
  return msg.getApproval().toByteArray();
}

static const char* severityToString(RuleCheckMessage::Severity severity) {
  switch (severity) {
    case RuleCheckMessage::Severity::Hint:
      return "hint";
    case RuleCheckMessage::Severity::Warning:
      return "warning";
    case RuleCheckMessage::Severity::Error:
      return "error";
  }
}

static QVariantMap summary(const RuleCheckMessage& msg,
                           const QSet<SExpression>& approved) {
  return {
      {"obj", getObj(msg)},
      {"severity", severityToString(msg.getSeverity())},
      {"message", msg.getMessage()},
      {"description", msg.getDescription()},
      {"approved", approved.contains(msg.getApproval())},
  };
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void RuleCheckRpc::run(GuiApplication& app, RpcObjectRegistry& registry,
                       RpcRequest& request) {
  Q_UNUSED(app);
  const bool quick = getOptionalArg<bool>(request, "quick").value_or(false);

  auto sendResponse = [&request](const RuleCheckMessageList& messages,
                                 const QSet<SExpression>& approved) {
    request.send({
        {"messages",
         toVariant(messages,
                   [&](const std::shared_ptr<const RuleCheckMessage>& msg) {
                     return summary(*msg, approved);
                   })},
    });
  };

  if (auto editor = registry.tryGetAs<ProjectEditor>(request.getArg("obj"))) {
    // ERC: Synchronous.
    const auto messages = editor->runErc();
    if (!messages) throw RpcInternalError("ERC failed.");
    sendResponse(*messages,
                 editor->getProject().getErcMessageApprovals());
  } else if (auto editor =
                 registry.tryGetAs<BoardEditor>(request.getArg("obj"))) {
    // DRC: Asynchronous.
    BoardDesignRuleCheck& drc = editor->startDrc(quick);
    if (!drc.isRunning()) throw RpcInternalError();
    request.makeAsync(drc);
    request.send(RpcNotification().progress(5));
    QObject::connect(&request, &RpcRequest::cancelled, &drc,
                     &BoardDesignRuleCheck::cancel);
    QObject::connect(&drc, &BoardDesignRuleCheck::progressPercent, &request,
                     [&request](int percent) {
                       request.send(RpcNotification().progress(percent));
                     });
    QObject::connect(
        &drc, &BoardDesignRuleCheck::finished, &request,
        [editor, &request,
         sendResponse](const BoardDesignRuleCheck::Result& result) {
          if (result.errors.isEmpty()) {
            sendResponse(result.messages,
                         editor->getBoard().getDrcMessageApprovals());
          } else {
            request.send(RpcCommandFailed(result.errors.join("; ")));
          }
        });
  } else {
    throw RpcInvalidHandle();
  }
}

void RuleCheckRpc::set(GuiApplication& app, RpcObjectRegistry& registry,
                       RpcRequest& request) {
  Q_UNUSED(app);
  const auto handles = getArg<QList<QByteArray>>(request, "messages");
  const bool approved = getArg<bool>(request, "approved");

  if (auto editor = registry.tryGetAs<ProjectEditor>(request.getArg("obj"))) {
    // ERC
    bool modified = false;
    for (const auto& handle : handles) {
      const auto approval = SExpression::parse(handle, FilePath());
      if (editor->getProject().setErcMessageApproved(*approval, approved)) {
        modified = true;
      }
    }
    if (modified) {
      editor->setManualModificationsMade();
      editor->scheduleErcRun(); // Not sure if needed.
    }
    request.send();
  } else if (auto editor =
             registry.tryGetAs<BoardEditor>(request.getArg("obj"))) {
    // DRC
    for (const auto& handle : handles) {
      const auto approval = SExpression::parse(handle, FilePath());
      editor->getBoard().setDrcMessageApproved(*approval, approved);
    }
    request.send();
  } else {
    throw RpcInvalidHandle();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

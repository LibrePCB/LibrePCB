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
#include "rpcrequest.h"

#include "rpcserver.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

/// Serialize an error into a map of the protocol's value domain.
static QVariantMap errorToVariant(rs::RpcErrorCode code, const QString& message,
                                  const QVariantMap& properties) noexcept {
  QVariantMap error;
  error.insert("code", QVariant::fromValue(static_cast<qlonglong>(code)));
  error.insert("message", QVariant(message));
  for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
    error.insert(it.key(), it.value());
  }
  return error;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RpcRequest::RpcRequest(GuiApplication& app, RpcServer& server, uint32_t connId,
                       uint32_t msgId, const QString& method,
                       const QVariantMap& args) noexcept
  : onFinished(*this),
    onNotification(*this),
    mApp(app),
    mServer(server),
    mConnId(connId),
    mMsgid(msgId),
    mMethod(method),
    mArgs(args),
    mReplied(false),
    mCancelled(false) {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RpcRequest::cancel() noexcept {
  if (!mReplied) {
    mCancelled = true;
    emit cancelled();
  }
}

void RpcRequest::sendNotification(const QVariant& payload) noexcept {
  if (!mReplied) {
    onNotification.notify(payload);
  }
}

void RpcRequest::sendSuccess(const QVariant& result) noexcept {
  sendReply(true, result);
}

void RpcRequest::sendError(rs::RpcErrorCode code, const QString& message,
                           const QVariantMap& properties) noexcept {
  sendReply(false, errorToVariant(code, message, properties));
}

void RpcRequest::sendReply(bool ok, const QVariant& value) noexcept {
  if (!mReplied) {
    mReplied = true;
    onFinished.notify(ok, value);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

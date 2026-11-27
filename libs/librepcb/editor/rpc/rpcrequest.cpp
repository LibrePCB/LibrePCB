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

#include "api/rpcerrors.h"
#include "rpcnotification.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

/// Serialize an error into a map of the protocol's value domain. The
/// error code is *not* part of the map — it is sent as the reply status
/// instead (see the FFI call in `RpcServer::requestEvent()`).
static QVariant errorToVariant(const QString& message,
                               const QVariantMap& properties) noexcept {
  QVariantMap error;
  error.insert("message", QVariant(message));
  for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
    error.insert(it.key(), it.value());
  }
  return error;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RpcRequest::RpcRequest(uint32_t connId, uint32_t msgId, const QString& method,
                       const QVariantMap& args) noexcept
  : onEvent(*this),
    mConnId(connId),
    mMsgid(msgId),
    mMethod(method),
    mArgs(args),
    mAsync(false),
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

void RpcRequest::makeAsync(QObject& supervisor) noexcept {
  mAsync = true;
  connect(
      &supervisor, &QObject::destroyed, this,
      [this]() {
        send(RpcInternalError("Supervisor got deleted before responding."));
      },
      Qt::QueuedConnection);  // Not sure if queued makes sense or not.
}

void RpcRequest::send(const RpcNotification& notification) noexcept {
  if (!mReplied) {
    onEvent.notify(Event::Notification, 0, notification.toVariant());
  }
}

void RpcRequest::send() noexcept {
  reply(0, QVariant());
}

void RpcRequest::send(const QVariantMap& result) noexcept {
  reply(0, QVariant(result));
}

void RpcRequest::send(const RpcError& error) noexcept {
  reply(static_cast<uint32_t>(error.getCode()),
        errorToVariant(error.getDetail(), error.getProperties()));
}

void RpcRequest::reply(uint32_t status, const QVariant& payload) noexcept {
  if (!mReplied) {
    mReplied = true;
    onEvent.notify(Event::Reply, status, payload);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

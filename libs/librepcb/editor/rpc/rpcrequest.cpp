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

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

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
        sendError(
            rpc::RpcInternalError("Supervisor got deleted before responding."));
      },
      Qt::QueuedConnection);  // Not sure if queued makes sense or not.
}

void RpcRequest::sendProgress(int percent) noexcept {
  QVariantMap value;
  value["progress"] = std::clamp(percent, 0, 100);
  sendNotification(value);
}

void RpcRequest::sendNotification(const QVariant& payload) noexcept {
  if (!mReplied) {
    onEvent.notify(Event::Notification, mConnId, mMsgid, 0, payload);
  }
}

void RpcRequest::sendSuccess(const QVariant& result) noexcept {
  if (!mReplied) {
    mReplied = true;
    onEvent.notify(Event::Reply, mConnId, mMsgid, 0, result);
  }
}

void RpcRequest::sendError(const RpcError& error) noexcept {
  if (!mReplied) {
    mReplied = true;
    onEvent.notify(Event::Reply, mConnId, mMsgid,
                   static_cast<uint32_t>(error.getCode()),
                   errorToVariant(error.getDetail(), error.getProperties()));
  }
}

void RpcRequest::throwInvalidArgs(const QString& value, const QString& argName,
                                  const char* enumName,
                                  const QStringList& values) {
  throw rpc::RpcInvalidArgs(QString("Unknown value '%1' for argument '%2' of "
                                    "enum %3. Supported values are [%4].")
                                .arg(value, argName,
                                     QString::fromLatin1(enumName),
                                     values.join(", ")));
}

void RpcRequest::throwMissingArgs(const QString& argName,
                                  const char* enumName) {
  throw rpc::RpcInvalidArgs(
      QString("Missing value for argument '%1' of enum %2.")
          .arg(argName, QString::fromLatin1(enumName)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

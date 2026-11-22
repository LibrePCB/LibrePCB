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

/// Serialize an error into a map of the protocol's value domain.
static QVariantMap errorToVariant(rpc::RpcErrorCode code,
                                  const QString& message,
                                  const QVariantMap& properties) noexcept {
  QVariantMap error;
  error.insert("code", static_cast<int>(code));
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
  : onFinished(*this),
    onNotification(*this),
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

void RpcRequest::sendProgress(float percent) noexcept {
  QVariantMap value;
  value["progress"] = std::clamp(percent / 100, float(0), float(1));
  sendNotification(value);
}

void RpcRequest::sendNotification(const QVariant& payload) noexcept {
  if (!mReplied) {
    onNotification.notify(payload);
  }
}

void RpcRequest::sendSuccess(const QVariant& result) noexcept {
  sendReply(true, result);
}

void RpcRequest::sendError(const RpcError& error) noexcept {
  sendReply(false,
            errorToVariant(error.getCode(), error.getDetail(),
                           error.getProperties()));
}

void RpcRequest::throwInvalidArgs(const QString& value, const QString& argName,
                                  const char* enumName) {
  throw rpc::RpcInvalidArgs(
      QString("Unknown value '%1' for argument '%2' of enum %3.")
          .arg(value, argName, QString::fromLatin1(enumName)));
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

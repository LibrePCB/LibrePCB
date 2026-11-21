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

#ifndef LIBREPCB_EDITOR_RPCREQUEST_H
#define LIBREPCB_EDITOR_RPCREQUEST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/utils/signalslot.h>
#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class RpcServer;

/*******************************************************************************
 *  Class RpcRequest
 ******************************************************************************/

/**
 * @brief A command request, as handed to the command handlers
 *
 * A request object is created by the #RpcServer (on the GUI thread) when a
 * command is dispatched. The handler must ensure that it is replied exactly
 * once, either synchronously (by calling #sendSuccess() or #sendError()) or
 * asynchronously (by keeping the object alive and calling one of these
 * methods later). If a handler forgets to reply, no response is sent at all
 * — the request stays pending and the client will run into a timeout.
 *
 * @note The class is fully decoupled from the #RpcServer: Responses and
 *       request-scoped messages are sent through its public API, and the
 *       server is notified about the reply through the #onFinished signal
 *       (it removes the request from its pending-request registry then).
 */
class RpcRequest final {
public:
  // Signals

  /**
   * @brief Emitted when the request was replied
   *
   * Emitted at the very end of #sendSuccess() or #sendError(), after the
   * response was sent. The #RpcServer uses this to remove the request from
   * its pending-request registry, which may drop the last reference to
   * this object, destroying it.
   */
  Signal<RpcRequest> onFinished;
  typedef Slot<RpcRequest> OnFinishedSlot;

  // Constructors / Destructor
  RpcRequest() = delete;
  RpcRequest(const RpcRequest& other) = delete;
  RpcRequest& operator=(const RpcRequest& rhs) = delete;
  RpcRequest(GuiApplication& app, RpcServer& server, uint32_t connId,
             uint32_t msgId, const QString& method,
             const QVariantMap& args) noexcept;

  // Getters
  GuiApplication& getApp() noexcept { return mApp; }
  uint32_t getConnId() const noexcept { return mConnId; }
  uint32_t getMsgId() const noexcept { return mMsgid; }
  const QString& getMethod() const noexcept { return mMethod; }
  const QVariantMap& getArgs() const noexcept { return mArgs; }

  /**
   * @brief Check whether the client cancelled this request
   *
   * @note Cancellation is cooperative: The handler may poll this flag
   *       during long-running work and stop early (replying with the
   *       "cancelled" error). If it doesn't, the normal result is sent
   *       as usual — the client must handle both cases.
   */
  bool isCancelled() const noexcept { return mCancelled; }

  // General Methods

  /**
   * @brief Mark the request as cancelled
   *
   * Called by the #RpcServer when the client cancelled this request. See
   * #isCancelled() for the cooperative cancellation semantics.
   */
  void setCancelled() noexcept { mCancelled = true; }

  /**
   * @brief Send a request-scoped message to the client (e.g. progress)
   *
   * May be called any number of times while the request is pending, i.e.
   * before it is replied. Never blocks, thus safe to call from the GUI
   * thread.
   *
   * @param payload  The message payload (e.g. a progress map).
   */
  void sendNotification(const QVariant& payload) noexcept;

  /**
   * @brief Reply with a result (finishes the request)
   *
   * @param result  The result value (any type of the protocol's value
   *                domain, usually a map).
   */
  void sendSuccess(const QVariant& result = QVariant()) noexcept;

  /**
   * @brief Reply with an error (finishes the request)
   *
   * @param code        An error code (see #ErrorCode).
   * @param message     A human-readable error message.
   * @param properties  Additional error properties, sent next to
   *                    "code" and "message" (see the error types in
   *                    the API specification).
   */
  void sendError(rs::RpcErrorCode code, const QString& message,
                 const QVariantMap& properties = QVariantMap()) noexcept;

private:  // Methods
  /// Send the response and emit #onFinished (finishes the request).
  void sendReply(bool ok, const QVariant& value) noexcept;

private:  // Data
  GuiApplication& mApp;
  RpcServer& mServer;
  const uint32_t mConnId;
  const uint32_t mMsgid;
  const QString mMethod;
  const QVariantMap mArgs;
  bool mReplied;
  bool mCancelled;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

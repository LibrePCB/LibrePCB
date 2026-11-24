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
#include "rpcerror.h"

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class RpcNotification;

/*******************************************************************************
 *  Class RpcRequest
 ******************************************************************************/

/**
 * @brief A command request, as handed to the command handlers
 *
 * A request object is created by the #RpcServer (on the GUI thread) when a
 * command is dispatched. The handler must ensure that it is replied exactly
 * once, either synchronously (by calling #send()) or asynchronously (by
 * keeping the object alive and calling one of the #send() methods later).
 * If a handler forgets to reply, no response is sent at all
 * — the request stays pending and the client will run into a timeout.
 *
 * @note The class is fully decoupled from the #RpcServer: Responses and
 *       request-scoped messages are sent through its public API, and the
 *       server is notified about the reply through the #onFinished signal
 *       (it removes the request from its pending-request registry then).
 *
 * @note The QObject base class does not serve the RPC internals, but
 *       allows connecting asynchronous tasks (e.g. a long-running
 *       operation which reports its progress) to a request: Qt signal/slot
 *       connections then automatically marshal emissions from worker
 *       threads to the GUI thread, and are dropped when the request is
 *       destroyed. Since the request is created and also destroyed on the
 *       GUI thread, Qt's thread affinity rules are fulfilled.
 */

/*******************************************************************************
 *  Class RpcRequest
 ******************************************************************************/

class RpcRequest final : public QObject {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    Notification,
    Reply,
  };
  Signal<RpcRequest, Event, uint32_t, uint32_t, uint32_t, const QVariant&>
      onEvent;
  typedef Slot<RpcRequest, Event, uint32_t, uint32_t, uint32_t, const QVariant&>
      OnEventSlot;

  // Constructors / Destructor
  RpcRequest() = delete;
  RpcRequest(const RpcRequest& other) = delete;
  RpcRequest& operator=(const RpcRequest& rhs) = delete;
  RpcRequest(uint32_t connId, uint32_t msgId, const QString& method,
             const QVariantMap& args) noexcept;

  // Getters
  const QString& getMethod() const noexcept { return mMethod; }
  const QVariantMap& getArgs() const noexcept { return mArgs; }
  QVariant getArg(const QString& key,
                  const QVariant& defaultValue = QVariant()) const noexcept {
    return mArgs.value(key, defaultValue);
  }

  /**
   * @brief Check whether the client cancelled this request
   *
   * @note Cancellation is cooperative: The handler may poll this flag
   *       during long-running work and stop early (replying with the
   *       "cancelled" error). If it doesn't, the normal result is sent
   *       as usual — the client must handle both cases.
   */
  bool isCancelled() const noexcept { return mCancelled; }

  bool isFinished() const noexcept { return mReplied; }

  bool isAsync() const noexcept { return mAsync; }

  // General Methods

  /**
   * @brief Mark the request as cancelled
   *
   * Called by the #RpcServer when the client cancelled this request. See
   * #isCancelled() for the cooperative cancellation semantics.
   */
  void cancel() noexcept;

  void makeAsync(QObject& supervisor) noexcept;

  /**
   * @brief Send a notification to the client (e.g. progress)
   *
   * May be called any number of times while the request is pending, i.e.
   * before it is replied. Never blocks, thus safe to call from the GUI
   * thread.
   */
  void send(const RpcNotification& notification) noexcept;

  /**
   * @brief Reply with success and no result (finishes the request)
   */
  void send() noexcept;

  /**
   * @brief Reply with a result (finishes the request)
   *
   * @param result  The result map, with the fields documented for the
   *                method in the specification.
   */
  void send(const QVariantMap& result) noexcept;

  /**
   * @brief Reply with an error thrown by a command implementation
   *
   * @param error  The error which was thrown (see rpcerrors.h).
   */
  void send(const RpcError& error) noexcept;

signals:
  void cancelled();

private:  // Methods
  /// Send the reply event (status 0 on success, otherwise the error code;
  /// the payload carries the result or the error map respectively).
  void reply(uint32_t status, const QVariant& payload) noexcept;

private:  // Data
  const uint32_t mConnId;
  const uint32_t mMsgid;
  const QString mMethod;
  const QVariantMap mArgs;
  bool mAsync;
  bool mReplied;
  bool mCancelled;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

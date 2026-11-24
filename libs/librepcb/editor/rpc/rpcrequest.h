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
 *
 * @note The QObject base class does not serve the RPC internals, but
 *       allows connecting asynchronous tasks (e.g. a long-running
 *       operation which reports its progress) to a request: Qt signal/slot
 *       connections then automatically marshal emissions from worker
 *       threads to the GUI thread, and are dropped when the request is
 *       destroyed. Since the request is created and also destroyed on the
 *       GUI thread, Qt's thread affinity rules are fulfilled.
 */
namespace rpc {

// Traits for enum types, generated in rpc/api/rpctypes.h (specialized
// per enum of the specification). See getArg<E>()/tryGetArg<E>() below.
template <typename T>
struct RpcEnumTraits;

}  // namespace rpc

/*******************************************************************************
 *  Class RpcRequest
 ******************************************************************************/

class RpcRequest final : public QObject {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    Notification,
    Succeeded,
    Failed,
  };
  Signal<RpcRequest, Event, const QVariant&> onEvent;
  typedef Slot<RpcRequest, Event, const QVariant&> OnEventSlot;

  // Constructors / Destructor
  RpcRequest() = delete;
  RpcRequest(const RpcRequest& other) = delete;
  RpcRequest& operator=(const RpcRequest& rhs) = delete;
  RpcRequest(uint32_t connId, uint32_t msgId, const QString& method,
             const QVariantMap& args) noexcept;

  // Getters
  uint32_t getConnId() const noexcept { return mConnId; }
  uint32_t getMsgId() const noexcept { return mMsgid; }
  const QString& getMethod() const noexcept { return mMethod; }
  const QVariantMap& getArgs() const noexcept { return mArgs; }
  QVariant getArg(const QString& key,
                  const QVariant& defaultValue = QVariant()) const noexcept {
    return mArgs.value(key, defaultValue);
  }

  /**
   * @brief Read an enum value from a request argument
   *
   * Returns the default value of the enum (from the specification) if the
   * argument is not provided. Enums without a default value are mandatory
   * arguments.
   *
   * @throws RpcInvalidArgs  If the argument is provided but is not nil or
   *                         a valid enum value, or if it is not provided
   *                         and the enum has no default value.
   */
  template <typename E>
  E getArg(const QString& argName) const {
    if (auto value = tryGetArg<E>(argName)) {
      return *value;
    } else if (auto fallback = rpc::RpcEnumTraits<E>::DEFAULT) {
      return *fallback;
    } else {
      throwMissingArgs(argName, rpc::RpcEnumTraits<E>::NAME);
    }
  }

  /**
   * @brief Read an optional enum value from a request argument
   *
   * @return std::nullopt if the argument is not provided at all.
   *
   * @throws RpcInvalidArgs  If the argument is provided but is nil or not
   *                         a valid enum value.
   */
  template <typename E>
  std::optional<E> tryGetArg(const QString& argName) const {
    auto argIt = mArgs.find(argName);
    if (argIt == mArgs.constEnd()) {
      return std::nullopt;  // Argument not provided.
    }
    const QString value = argIt->toString();
    const auto& map = rpc::RpcEnumTraits<E>::mapFromString();
    const auto mapIt = map.constFind(value);
    if (mapIt != map.constEnd()) {
      return *mapIt;
    }
    throwInvalidArgs(value, argName, rpc::RpcEnumTraits<E>::NAME, map.keys());
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

  void sendProgress(int percent) noexcept;

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
   * @brief Reply with an error thrown by a command implementation
   *
   * @param error  The error which was thrown (see rpcerrors.h).
   */
  void sendError(const RpcError& error) noexcept;

signals:
  void cancelled();

private:  // Methods
  /// Throw an InvalidArgs error for an invalid enum argument value.
  /// Defined in rpcrequest.cpp (the only place allowed to include
  /// the generated error classes).
  [[noreturn]] static void throwInvalidArgs(const QString& value,
                                            const QString& argName,
                                            const char* enumName,
                                            const QStringList& values);

  /// Throw an InvalidArgs error for a missing mandatory enum argument.
  [[noreturn]] static void throwMissingArgs(const QString& argName,
                                            const char* enumName);

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

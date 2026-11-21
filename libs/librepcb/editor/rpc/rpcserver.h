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

#ifndef LIBREPCB_EDITOR_RPCSERVER_H
#define LIBREPCB_EDITOR_RPCSERVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/utils/rusthandle.h>
#include <librepcb/rust-core/ffi.h>
#include <librepcb/core/utils/signalslot.h>
#include <QtCore>

#include <functional>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace editor {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class RpcServer
 ******************************************************************************/

/**
 * @brief The RPC server to control the application with external tools
 *
 * This class integrates the Rust RPC server (rust-core module `rpc_server`)
 * into the LibrePCB application:
 *
 * - It starts and stops the (Rust implemented) server, which listens on
 *   127.0.0.1 for msgpack-encoded RPC commands (see the specification
 *   in `dev/plans/rpc-server.md` and `share/librepcb/rpc/rpc.yaml`).
 * - It provides the command dispatcher which marshals incoming commands to
 *   the GUI thread (via `Qt::QueuedConnection`, asynchronously!) and
 *   dispatches them to the command handlers registered in `commands.cpp`.
 * - Commands may reply synchronously (from within the handler) or later
 *   (e.g. after a long-running task finished), optionally emitting
 *   request-scoped messages (e.g. progress) in the meantime.
 * - It pushes application events (e.g. "project_opened") to all clients.
 *
 * @note The token is currently hardcoded (development only!) and thus the
 *       server accepts any local process. To be replaced by a random token
 *       together with the instance registry, see the plan section 9.
 */
class RpcServer final : public QObject {
  Q_OBJECT

public:
  // Types

  // Note: The RPC error codes are defined by the Rust server (see
  // `RpcErrorCode` in `rust-core/src/rpc_server/protocol.rs`, exposed
  // through `ffi.h` as `rs::RpcErrorCode`) and are used directly from
  // there — no duplication.

  /**
   * @brief A command request, as handed to the command handlers
   *
   * A request object is created on the GUI thread when a command is
   * dispatched. The handler must ensure that it is replied exactly once,
   * either synchronously (by calling #replySuccess() or #replyError()) or
   * asynchronously (by keeping the object alive and calling one of these
   * methods later). As a safety net, the destructor replies with an error
   * if the handler forgot to reply.
   */
  class Request final {
  public:
    // Constructors / Destructor
    Request() = delete;
    Request(const Request& other) = delete;
    Request& operator=(const Request& rhs) = delete;
    Request(GuiApplication& app, RpcServer& server, uint32_t connId,
            uint32_t msgId, const QString& method,
            const QVariantMap& args) noexcept;
    ~Request() noexcept;

    // Getters
    GuiApplication& getApp() noexcept { return mApp; }
    uint32_t getConnId() const noexcept { return mConnId; }
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
     *                domain, usually a map). Gets normalized, i.e. plain
     *                C++ types like `int` are converted automatically.
     */
    void sendSuccess(const QVariant& result = QVariant()) noexcept;

    /**
     * @brief Reply with an error (finishes the request)
     *
     * @param code        An error code (see #ErrorCode).
     * @param message     A human-readable error message.
     * @param properties  Additional error properties, sent next to
     *                    "code" and "message" (see the error types in the
     *                    API specification).
     */
    void sendError(rs::RpcErrorCode code, const QString& message,
                   const QVariantMap& properties = QVariantMap()) noexcept;

  private:  // Methods
    /// Mark the request as cancelled (called by the server).
    void setCancelled() noexcept { mCancelled = true; }

    /// Suppress the safety-net reply in the destructor (server teardown).
    void invalidate() noexcept { mServer = nullptr; }

    /// Send the response through the FFI (does nothing if invalidated).
    void sendReply(bool ok, const QVariant& value) noexcept;

    friend class RpcServer;

  private:  // Data
    GuiApplication& mApp;
    RpcServer* mServer;  ///< Null once the server is being destroyed.
    const uint32_t mConnId;
    const uint32_t mMsgid;
    const QString mMethod;
    const QVariantMap mArgs;
    bool mReplied;
    bool mCancelled;
  };

  /// A command handler, executed on the GUI thread. It must reply the
  /// request (synchronously or asynchronously, see #Request).
  using RpcHandler = std::function<void(Request& request)>;

  // Constructors / Destructor
  RpcServer() = delete;
  RpcServer(const RpcServer& other) = delete;
  RpcServer& operator=(const RpcServer& rhs) = delete;

  /**
   * @brief Start the RPC server (listening on 127.0.0.1)
   *
   * @param app   The application to dispatch the commands to.
   */
  explicit RpcServer(GuiApplication& app) noexcept;

  /**
   * @brief Stop the RPC server (must be called while the application is
   *        still fully alive, i.e. as the first step of the teardown)
   */
  ~RpcServer() noexcept override;

  /**
   * @brief Get all registered command handlers, by method name
   *
   * @note This is a static method on purpose, i.e. it does not require an
   *       application or a running server. It allows testing the registry
   *       without launching the whole application.
   */
  static QHash<QString, RpcHandler> buildCommandRegistry() noexcept;

private:  // Methods
  /// Called on the GUI thread: Extract the "obj" key, look up the command
  /// handler and execute it (or reply with an error).
  void handleRequest(uint32_t connId, uint32_t msgId, const QString& method,
                     const QVariant& args) noexcept;

  /// Called on the GUI thread: Mark a pending request as cancelled.
  void handleCancel(uint32_t connId, uint32_t msgId) noexcept;

  /// Called on the GUI thread: Handle an `initialize` request — validate
  /// the token, mark the connection as (un)authenticated on the Rust side
  /// and reply with the application information.
  void handleInitialize(Request& request) noexcept;

  /// Key for a pending request in the registry.
  static uint64_t requestKey(uint32_t connId, uint32_t msgId) noexcept {
    return (static_cast<uint64_t>(connId) << 32) | msgId;
  }

  static void onRequestCallback(void* ctx, uint32_t connId, uint32_t msgId,
                                const QString* method, const QVariant* params);
  static void onCancelCallback(void* ctx, uint32_t connId, uint32_t msgId);

private:  // Data
  /// The application to dispatch the commands to
  GuiApplication& mApp;

  /// All registered command handlers, by method name
  QHash<QString, RpcHandler> mCommands;

  /// The TCP port the server is listening on (coming from Rust side)
  uint16_t mPort;

  /// The FFI handle of the (Rust implemented) server
  RustHandle<rs::RpcServer> mHandle;

  /// Pending (dispatched, not yet replied) requests, by connection+msgId
  QHash<uint64_t, std::shared_ptr<Request>> mPendingRequests;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

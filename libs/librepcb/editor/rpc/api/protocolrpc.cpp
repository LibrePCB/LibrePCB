/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors
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
#include "protocolrpc.h"

#include "../../guiapplication.h"
#include "../../notification.h"
#include "../../notificationsmodel.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"

#include <librepcb/core/application.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/rust-core/ffi.h>

#include <QRegularExpression>
#include <QSettings>
#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

/// The range of protocol versions supported by this server (see the
/// negotiation rules in the RPC server documentation). Raising the
/// minimum drops support for older versions (breaking very old clients,
/// detected on their side); raising the maximum adds new features. Every
/// version N is a cumulative superset of all versions < N.
static constexpr int MIN_PROTOCOL = 1;
static constexpr int MAX_PROTOCOL = 1;

/// The nonce of each connection, generated in `initialize` and consumed in
/// `authenticate`. Connections which never authenticate leak an entry,
/// which is acceptable (they are bounded by the connection counter).
static QHash<uint32_t, QString>& getConnectionNonces() noexcept {
  static QHash<uint32_t, QString> sNonces;
  return sNonces;
}

/// The public keys of external clients which the user has confirmed, as
/// 64 hex characters each (persisted per user & machine; the keys are
/// public, so no secret is stored).
static QStringList getKnownClientKeys() noexcept {
  return QSettings().value("rpc/known_clients").toStringList();
}

static void addKnownClientKey(const QString& pubkey) noexcept {
  QStringList keys = getKnownClientKeys();
  keys.append(pubkey);
  QSettings().setValue("rpc/known_clients", keys);
}

/// Verify an Ed25519 signature (hex encoded) over the given message.
static bool verifyProof(const QString& pubkeyHex, const QString& proofHex,
                        const QByteArray& message) noexcept {
  static const QRegularExpression sHex64("^[0-9a-fA-F]{64}$");
  static const QRegularExpression sHex128("^[0-9a-fA-F]{128}$");
  if ((!sHex64.match(pubkeyHex).hasMatch()) ||
      (!sHex128.match(proofHex).hasMatch())) {
    return false;
  }
  const QByteArray pubkey = QByteArray::fromHex(pubkeyHex.toLatin1());
  const QByteArray proof = QByteArray::fromHex(proofHex.toLatin1());
  return rs::ffi_ed25519_verify(
      reinterpret_cast<const uint8_t*>(pubkey.constData()),
      reinterpret_cast<const uint8_t*>(message.constData()),
      static_cast<std::size_t>(message.size()),
      reinterpret_cast<const uint8_t*>(proof.constData()));
}

/// Reply to an authenticated `authenticate` request.
static void sendAuthenticated(RpcRequest& request) noexcept {
  static const Uuid sSession = Uuid::createRandom();
  request.send({
      {"server", "librepcb"},
      {"version", Application::getVersion()},
      {"session", sSession.toStr()},
  });
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProtocolRpc::initialize(GuiApplication& app, RpcObjectRegistry& registry,
                             RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);

  // Parse and negotiate the protocol version. The server never rejects on
  // version grounds: Whatever the client offers, the connection speaks
  // the negotiated version from the response. If the offer is below the
  // oldest version still supported, the response carries that oldest
  // version, and it is up to the client to detect that it cannot speak
  // it and to close the connection.
  const qint64 clientProtocol = getArg<qint64>(request, "protocol");
  if (clientProtocol < 0) {
    throw RpcInvalidArgs(QString("Invalid value '%1' for argument "
                                 "'protocol' (expected a non-negative "
                                 "integer).")
                             .arg(clientProtocol));
  }
  const int negotiated = static_cast<int>(
      std::clamp(clientProtocol, static_cast<qint64>(MIN_PROTOCOL),
                 static_cast<qint64>(MAX_PROTOCOL)));
  qInfo().noquote() << "RPC connection (protocol" << clientProtocol << "->"
                    << negotiated << ").";

  // Issue the nonce for this connection, consumed by `authenticate`.
  const QString nonce = Uuid::createRandom().toStr();
  getConnectionNonces().insert(request.getConnId(), nonce);

  request.send({
      {"protocol", negotiated},
      {"nonce", nonce},
  });
}

void ProtocolRpc::authenticate(GuiApplication& app, RpcObjectRegistry& registry,
                               RpcRequest& request) {
  Q_UNUSED(registry);

  const QString clientName =
      getArg<QString>(request, "name").simplified().left(32);
  const auto pubkey = getOptionalArg<QString>(request, "pubkey");
  const auto proof = getOptionalArg<QString>(request, "proof");
  const auto token = getOptionalArg<QString>(request, "token");

  // Consume the nonce issued by `initialize` (rejects connections which
  // did not handshake first).
  const QString nonce = getConnectionNonces().take(request.getConnId());
  if (nonce.isEmpty()) {
    throw RpcAccessDenied("The connection was not initialized.");
  }

  if (token) {
    // TODO: The spawned-process token mechanism is not implemented yet
    // (no tokens are issued), so any token is rejected.
    throw RpcAccessDenied("Invalid token.");
  }

  if (pubkey && proof) {
    // An external client proves possession of an Ed25519 private key.
    if (!verifyProof(*pubkey, *proof, nonce.toUtf8())) {
      throw RpcAccessDenied("Invalid proof.");
    }
    if (getKnownClientKeys().contains(*pubkey)) {
      // The user confirmed this key before: Authenticate silently.
      qInfo() << "RPC client" << clientName << "authenticated (known key).";
      sendAuthenticated(request);
      return;
    }
    // Unknown key: Ask the user whether to accept the connection. If the
    // user accepts it, the key is remembered for future connections.
    auto notification = std::make_shared<Notification>(
        ui::NotificationType::Warning, tr("RPC Connection Request"),
        tr("An external application, which identifies itself as '%1', wants "
           "to connect through the RPC interface to this process for the "
           "first time. Only accept this if you trust that application and "
           "the request was triggered by yourself.")
            .arg(clientName),
        tr("Accept Connection"), QString(), true);
    QObject::connect(notification.get(), &Notification::buttonClicked, &request,
                     [pubkey = *pubkey, clientName, &request]() {
                       addKnownClientKey(pubkey);
                       qInfo() << "RPC client" << clientName
                               << "authenticated (key confirmed).";
                       sendAuthenticated(request);
                     });
    QObject::connect(notification.get(), &Notification::destroyed, &request,
                     [&request]() { request.send(RpcAccessDenied()); });
    QObject::connect(&request, &RpcRequest::cancelled, notification.get(),
                     [&request]() { request.send(RpcCancelled()); });
    QObject::connect(&request, &RpcRequest::destroyed, notification.get(),
                     &Notification::dismiss);
    app.getNotifications().push(notification);
    request.makeAsync(*qApp);
    return;
  }

  throw RpcInvalidArgs(
      "Either 'pubkey'+'proof' or 'token' must be "
      "provided.");
}

void ProtocolRpc::ping(GuiApplication& app, RpcObjectRegistry& registry,
                       RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  request.send();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

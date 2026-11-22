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

#ifndef LIBREPCB_EDITOR_RPCERROR_H
#define LIBREPCB_EDITOR_RPCERROR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <exception>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

// Defined in rpcerrors.h (a generated file).
enum class RpcErrorCode : int;

/*******************************************************************************
 *  Class RpcError
 ******************************************************************************/

/**
 * @brief The base class of all RPC error types
 *
 * Command implementations throw an instance of one of the error classes
 * (generated in rpcerrors.h) to reply an error; the server catches it and
 * sends the response accordingly. The canonical message of the error code
 * is not part of this class — it is sent by the Rust server (see the API
 * specification), `what()` only returns the application-provided detail.
 *
 * @attention RPC exceptions shall only be used in RPC command handlers!
 *            Those errors are not safe to be thrown outside of the
 *            ::librepcb::editor::RpcServer::handleRequest() call stack,
 *            since those exceptions do not inherit from ::librepcb::Exception
 *            and would therefore not be caught anywhere else in the
 *            application, causing app termination. Inheritance is here not
 *            desired by intention, since it would cause annoying logging
 *            messages for each failed RPC command.
 */
class RpcError : public std::exception {
public:
  // Constructors / Destructor
  RpcError() = delete;
  RpcError(const RpcError& other) = default;
  RpcError& operator=(const RpcError& rhs) = delete;

  // Getters
  RpcErrorCode getCode() const noexcept { return mCode; }
  const QString& getDetail() const noexcept { return mDetail; }
  const QVariantMap& getProperties() const noexcept { return mProperties; }

protected:
  RpcError(RpcErrorCode code, const QString& detail = QString(),
           const QVariantMap& properties = QVariantMap()) noexcept
    : mCode(code), mDetail(detail), mProperties(properties) {}

private:  // Data
  const RpcErrorCode mCode;
  const QString mDetail;
  const QVariantMap mProperties;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif

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

#ifndef LIBREPCB_RUST_CORE_RPCEXT_H
#define LIBREPCB_RUST_CORE_RPCEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace rs {

/*******************************************************************************
 *  Class MsgPackExt
 ******************************************************************************/

/**
 * @brief A MessagePack extension type value (see the RPC API specification)
 *
 * The C++ representation of a msgpack `ext` value: An application-specific
 * ext code plus the raw payload bytes. Such values are fully opaque to the
 * RPC transport layer — neither the Rust side nor anything else inspects
 * them, they are just passed through.
 *
 * To send an ext value in an RPC response or notification, put it into a
 * QVariant with QVariant::fromValue() anywhere in the payload. Incoming
 * ext values from clients are delivered the same way, i.e. any command
 * argument may hold a QVariant containing this class.
 */
class MsgPackExt final {
public:
  // Constructors / Destructor

  /**
   * @brief Default constructor (creates an invalid value)
   */
  MsgPackExt() noexcept : mType(0) {}

  /**
   * @brief Constructor
   *
   * @param type  The msgpack ext type code
   * @param data  The raw payload bytes
   */
  MsgPackExt(qint8 type, const QByteArray& data) noexcept
    : mType(type), mData(data) {}

  // Getters

  /**
   * @brief Get the msgpack ext type code
   *
   * @return The ext type code
   */
  qint8 getType() const noexcept { return mType; }

  /**
   * @brief Get the raw payload bytes
   *
   * @return Reference to the payload
   */
  const QByteArray& getData() const noexcept { return mData; }

private:  // Data
  qint8 mType;  ///< The msgpack ext type code
  QByteArray mData;  ///< The raw payload bytes
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rs
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::rs::MsgPackExt)

#endif

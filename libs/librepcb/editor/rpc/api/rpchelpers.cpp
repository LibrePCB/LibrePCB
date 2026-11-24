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
#include "rpchelpers.h"

#include "rpcerrors.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

void throwMissingArg(const QString& argName, const char* typeName) {
  throw RpcInvalidArgs(QString("Missing value for argument '%1' (expected %2).")
                           .arg(argName, QString::fromLatin1(typeName)));
}

void throwInvalidArg(const QString& argName, const QVariant& value,
                     const char* typeName) {
  throw RpcInvalidArgs(
      QString("Invalid value '%1' for argument '%2' "
              "(expected %3).")
          .arg(value.toString(), argName, QString::fromLatin1(typeName)));
}

void throwInvalidEnumArg(const QString& argName, const QString& value,
                         const char* enumName, const QString& values) {
  throw RpcInvalidArgs(
      QString("Unknown value '%1' for argument '%2' of enum "
              "%3. Supported values are [%4].")
          .arg(value, argName, QString::fromLatin1(enumName), values));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb

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
#include <gtest/gtest.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/editor/rpc/rpcserver.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class RpcServerTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(RpcServerTest, testCommandRegistryMatchesApiSchema) {
  // Load the OpenRPC API schema (the single source of truth).
  const FilePath schemaPath(TEST_SHARE_DIR "/rpc/api.json");
  QFile schemaFile(schemaPath.toStr());
  ASSERT_TRUE(schemaFile.open(QIODevice::ReadOnly))
      << "Could not open " << schemaPath.toNative().toStdString();
  const QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaFile.readAll());
  ASSERT_TRUE(schemaDoc.isObject());
  QSet<QString> schemaMethods;
  for (const QJsonValue& value :
       schemaDoc.object().value("methods").toArray()) {
    const QJsonObject method = value.toObject();
    ASSERT_TRUE(method.contains("name"));
    schemaMethods.insert(method.value("name").toString());
  }

  // Collect the methods of the command registry.
  const QHash<QString, RpcServer::RpcHandler> registry =
      RpcServer::buildCommandRegistry();
  QSet<QString> registryMethods;
  for (auto it = registry.constBegin(); it != registry.constEnd(); ++it) {
    registryMethods.insert(it.key());
  }

  // Both directions must match exactly, otherwise the documentation (which
  // is generated from the schema) is out of sync with the implementation.
  for (const QString& method : schemaMethods) {
    EXPECT_TRUE(registryMethods.contains(method))
        << "Method documented in api.json but not implemented: "
        << method.toStdString();
  }
  for (const QString& method : registryMethods) {
    EXPECT_TRUE(schemaMethods.contains(method))
        << "Method implemented but not documented in api.json: "
        << method.toStdString();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb

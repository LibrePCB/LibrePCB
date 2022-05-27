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
#include "../serialization/sexpressionlegacymode.h"

#include <gtest/gtest.h>
#include <librepcb/core/application.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class WorkspaceSettingsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(WorkspaceSettingsTest, testLoadFromSExpressionV01) {
  // Attention: Do NOT modify this string! It represents the freezed(!) file
  // format V0.1 and even current versions of LibrePCB must be able to load it!
  SExpression root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (user \"Foo Bar\")\n"
      " (application_locale \"de_CH\")\n"
      " (default_length_unit micrometers)\n"
      " (project_autosave_interval 120)\n"
      " (use_opengl true)\n"
      " (library_locale_order\n"
      "  (locale \"de_DE\")\n"
      " )\n"
      " (library_norm_order\n"
      "  (norm \"IEC 60617\")\n"
      " )\n"
      " (repositories\n"
      "  (repository \"https://api.librepcb.org\")\n"
      " )\n"
      ")",
      FilePath());

  WorkspaceSettings obj;
  obj.load(root, Version::fromString("0.1"));
  EXPECT_EQ("Foo Bar", obj.userName.get());
  EXPECT_EQ("de_CH", obj.applicationLocale.get());
  EXPECT_EQ(LengthUnit::micrometers(), obj.defaultLengthUnit.get());
  EXPECT_EQ(120U, obj.projectAutosaveIntervalSeconds.get());
  EXPECT_EQ(true, obj.useOpenGl.get());
  EXPECT_EQ(QStringList{"de_DE"}, obj.libraryLocaleOrder.get());
  EXPECT_EQ(QStringList{"IEC 60617"}, obj.libraryNormOrder.get());
  EXPECT_EQ(QList<QUrl>{QUrl("https://api.librepcb.org")},
            obj.repositoryUrls.get());
}

TEST_F(WorkspaceSettingsTest, testLoadFromSExpressionCurrentVersion) {
  SExpression root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (user \"Foo Bar\")\n"
      " (application_locale \"de_CH\")\n"
      " (default_length_unit micrometers)\n"
      " (project_autosave_interval 120)\n"
      " (use_opengl true)\n"
      " (library_locale_order\n"
      "  (locale \"de_DE\")\n"
      " )\n"
      " (library_norm_order\n"
      "  (norm \"IEC 60617\")\n"
      " )\n"
      " (repositories\n"
      "  (repository \"https://api.librepcb.org\")\n"
      " )\n"
      ")",
      FilePath());

  WorkspaceSettings obj;
  obj.load(root, qApp->getFileFormatVersion());
  EXPECT_EQ("Foo Bar", obj.userName.get());
  EXPECT_EQ("de_CH", obj.applicationLocale.get());
  EXPECT_EQ(LengthUnit::micrometers(), obj.defaultLengthUnit.get());
  EXPECT_EQ(120U, obj.projectAutosaveIntervalSeconds.get());
  EXPECT_EQ(true, obj.useOpenGl.get());
  EXPECT_EQ(QStringList{"de_DE"}, obj.libraryLocaleOrder.get());
  EXPECT_EQ(QStringList{"IEC 60617"}, obj.libraryNormOrder.get());
  EXPECT_EQ(QList<QUrl>{QUrl("https://api.librepcb.org")},
            obj.repositoryUrls.get());
}

TEST_F(WorkspaceSettingsTest, testStoreAndLoad) {
  // Store
  WorkspaceSettings obj1;
  obj1.userName.set("foo bar");
  obj1.applicationLocale.set("de_CH");
  obj1.defaultLengthUnit.set(LengthUnit::nanometers());
  obj1.projectAutosaveIntervalSeconds.set(1234);
  obj1.useOpenGl.set(!obj1.useOpenGl.get());
  obj1.libraryLocaleOrder.set({"de_CH", "en_US"});
  obj1.libraryNormOrder.set({"foo", "bar"});
  obj1.repositoryUrls.set({QUrl("https://foo"), QUrl("https://bar")});
  obj1.useCustomPdfReader.set(obj1.useCustomPdfReader.get());
  obj1.pdfReaderCommand.set("my reader");
  obj1.pdfOpenBehavior.set(WorkspaceSettings::PdfOpenBehavior::NEVER);
  const SExpression root1 = obj1.serialize();

  // Load
  WorkspaceSettings obj2;
  obj2.load(root1, qApp->getFileFormatVersion());
  EXPECT_EQ(obj1.userName.get().toStdString(),
            obj2.userName.get().toStdString());
  EXPECT_EQ(obj1.applicationLocale.get().toStdString(),
            obj2.applicationLocale.get().toStdString());
  EXPECT_EQ(obj1.defaultLengthUnit.get(), obj2.defaultLengthUnit.get());
  EXPECT_EQ(obj1.projectAutosaveIntervalSeconds.get(),
            obj2.projectAutosaveIntervalSeconds.get());
  EXPECT_EQ(obj1.useOpenGl.get(), obj2.useOpenGl.get());
  EXPECT_EQ(obj1.libraryLocaleOrder.get(), obj2.libraryLocaleOrder.get());
  EXPECT_EQ(obj1.libraryNormOrder.get(), obj2.libraryNormOrder.get());
  EXPECT_EQ(obj1.repositoryUrls.get(), obj2.repositoryUrls.get());
  EXPECT_EQ(obj1.useCustomPdfReader.get(), obj2.useCustomPdfReader.get());
  EXPECT_EQ(obj1.pdfReaderCommand.get(), obj2.pdfReaderCommand.get());
  EXPECT_EQ(obj1.pdfOpenBehavior.get(), obj2.pdfOpenBehavior.get());
  const SExpression root2 = obj2.serialize();

  // Check if serialization of loaded settings leads to same file content
  EXPECT_EQ(root1.toByteArray().toStdString(),
            root2.toByteArray().toStdString());
}

// Verify that serializing does only overwrite modified settings, but keeps
// unknown file entries and does not add new entries for default settings.
// This allows to switch between different application versions without
// creating unnecessary modifications after an upgrade, or - even worse -
// loosing settings after a downgrade. This allows us to improve/extend the
// workspace settings even between minor versions (i.e. without introducing
// a new file format) without any pain for users.
//
// In addition, it ensures that the built-in default values are used unless
// the user explicitly changed the settings. This way, most users will profit
// from improved default settings automatically. If we saved all settings every
// time, users would keep the settings at the time writing the settings file
// the first time forever.
TEST_F(WorkspaceSettingsTest, testSaveOnlyModifiedSettings) {
  SExpression root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_item \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(root, qApp->getFileFormatVersion());
  EXPECT_EQ(1234U, obj.projectAutosaveIntervalSeconds.get());
  obj.projectAutosaveIntervalSeconds.set(42);
  const SExpression root2 = obj.serialize();

  QString actualContent = root2.toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 42)\n"
      " (unknown_item \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

// Addition for the previous test: Saving a default-constructed object to file
// shall create a file without any entries.
TEST_F(WorkspaceSettingsTest, testDefaultSerializeEmpty) {
  SExpressionLegacyMode legacyMode(false);  // File format v0.2+

  WorkspaceSettings obj;
  const SExpression root = obj.serialize();

  QString actualContent = root.toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

TEST_F(WorkspaceSettingsTest, testDefaultSerializeEmptyFileLegacy) {
  SExpressionLegacyMode legacyMode(true);  // File format v0.1

  WorkspaceSettings obj;
  const SExpression root = obj.serialize();

  QString actualContent = root.toByteArray();
  QString expectedContent = "(librepcb_workspace_settings)\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

// Test that restoring all default values also removes unknown entries from the
// settings file, since an empty file is the real default.
TEST_F(WorkspaceSettingsTest, testRestoreDefaultsClearsFile) {
  SExpressionLegacyMode legacyMode(false);  // File format v0.2+
  SExpression root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_value \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(root, qApp->getFileFormatVersion());
  obj.restoreDefaults();
  const SExpression root2 = obj.serialize();

  QString actualContent = root2.toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

TEST_F(WorkspaceSettingsTest, testRestoreDefaultsClearsFileLegacy) {
  SExpressionLegacyMode legacyMode(true);  // File format v0.1
  SExpression root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_value \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(root, qApp->getFileFormatVersion());
  obj.restoreDefaults();
  const SExpression root2 = obj.serialize();

  QString actualContent = root2.toByteArray();
  QString expectedContent = "(librepcb_workspace_settings)\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

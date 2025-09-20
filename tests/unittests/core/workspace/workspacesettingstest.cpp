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
#include <librepcb/core/application.h>
#include <librepcb/core/types/version.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

using ApiEndpoint = WorkspaceSettings::ApiEndpoint;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class WorkspaceSettingsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(WorkspaceSettingsTest, testLoadFromSExpression) {
  std::unique_ptr<SExpression> root = SExpression::parse(
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
      " (api_endpoints\n"
      "  (endpoint \"https://api.librepcb.org\" (libraries true) "
      "(parts false) (order true))\n"
      " )\n"
      " (external_web_browser\n"
      "  (command \"firefox \\\"{{URL}}\\\"\")\n"
      " )\n"
      " (external_file_manager\n"
      "  (command \"nautilus \\\"{{FILEPATH}}\\\"\")\n"
      " )\n"
      " (external_pdf_reader\n"
      "  (command \"evince \\\"{{FILEPATH}}\\\"\")\n"
      " )\n"
      " (dismissed_messages\n"
      "  (message \"SOME_MESSAGE: foo\")\n"
      "  (message \"SOME_MESSAGE: bar\")\n"
      " )\n"
      ")",
      FilePath());

  WorkspaceSettings obj;
  obj.load(*root, Application::getFileFormatVersion());
  EXPECT_EQ("Foo Bar", obj.userName.get());
  EXPECT_EQ("de_CH", obj.applicationLocale.get());
  EXPECT_EQ(LengthUnit::micrometers(), obj.defaultLengthUnit.get());
  EXPECT_EQ(120U, obj.projectAutosaveIntervalSeconds.get());
  EXPECT_EQ(true, obj.useOpenGl.get());
  EXPECT_EQ(QStringList{"de_DE"}, obj.libraryLocaleOrder.get());
  EXPECT_EQ(QStringList{"IEC 60617"}, obj.libraryNormOrder.get());
  EXPECT_EQ(
      (QList<ApiEndpoint>{
          ApiEndpoint{QUrl("https://api.librepcb.org"), true, false, true},
      }),
      obj.apiEndpoints.get());
  EXPECT_EQ(QStringList{"firefox \"{{URL}}\""},
            obj.externalWebBrowserCommands.get());
  EXPECT_EQ(QStringList{"nautilus \"{{FILEPATH}}\""},
            obj.externalFileManagerCommands.get());
  EXPECT_EQ(QStringList{"evince \"{{FILEPATH}}\""},
            obj.externalPdfReaderCommands.get());
  EXPECT_EQ((QSet<QString>{"SOME_MESSAGE: foo", "SOME_MESSAGE: bar"}),
            obj.dismissedMessages.get());
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
  obj1.apiEndpoints.set({
      ApiEndpoint{QUrl("https://foo"), true, false, true},
      ApiEndpoint{QUrl("https://bar"), false, true, false},
  });
  obj1.externalWebBrowserCommands.set({"foo", "bar"});
  obj1.externalFileManagerCommands.set({"file", "manager"});
  obj1.externalPdfReaderCommands.set({"pdf", "reader"});
  obj1.dismissedMessages.set({"foo", "bar"});
  const std::unique_ptr<const SExpression> root1 = obj1.serialize();

  // Load
  WorkspaceSettings obj2;
  obj2.load(*root1, Application::getFileFormatVersion());
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
  EXPECT_EQ(obj1.apiEndpoints.get(), obj2.apiEndpoints.get());
  EXPECT_EQ(obj1.externalWebBrowserCommands.get(),
            obj2.externalWebBrowserCommands.get());
  EXPECT_EQ(obj1.externalFileManagerCommands.get(),
            obj2.externalFileManagerCommands.get());
  EXPECT_EQ(obj1.externalPdfReaderCommands.get(),
            obj2.externalPdfReaderCommands.get());
  EXPECT_EQ(obj1.dismissedMessages.get(), obj2.dismissedMessages.get());
  const std::unique_ptr<const SExpression> root2 = obj2.serialize();

  // Check if serialization of loaded settings leads to same file content
  EXPECT_EQ(root1->toByteArray().toStdString(),
            root2->toByteArray().toStdString());
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
  std::unique_ptr<SExpression> root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_item \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(*root, Application::getFileFormatVersion());
  EXPECT_EQ(1234U, obj.projectAutosaveIntervalSeconds.get());
  obj.projectAutosaveIntervalSeconds.set(42);
  const std::unique_ptr<const SExpression> root2 = obj.serialize();

  QString actualContent = root2->toByteArray();
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
  WorkspaceSettings obj;
  const std::unique_ptr<const SExpression> root = obj.serialize();

  QString actualContent = root->toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

// Test that restoring all default values also removes unknown entries from the
// settings file, since an empty file is the real default.
TEST_F(WorkspaceSettingsTest, testRestoreDefaultsClearsFile) {
  std::unique_ptr<SExpression> root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_value \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(*root, Application::getFileFormatVersion());
  obj.restoreDefaults();
  const std::unique_ptr<const SExpression> root2 = obj.serialize();

  QString actualContent = root2->toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

// Verify that unknown (obsolete) settings are removed from the file when
// upgrading the file format. Otherwise we have no clue what file format
// an entry has if it was removed in some LibrePCB version, and then re-added
// some day later.
TEST_F(WorkspaceSettingsTest, testUpgradeFileFormat) {
  std::unique_ptr<SExpression> root = SExpression::parse(
      "(librepcb_workspace_settings\n"
      " (dismissed_messages\n"
      "  (message \"SOME_MESSAGE: foo\")\n"
      "  (message \"SOME_MESSAGE: bar\")\n"
      " )\n"
      " (external_pdf_reader\n"
      "  (command \"evince \\\"{{FILEPATH}}\\\"\")\n"
      " )\n"
      " (keyboard_shortcuts\n"
      "  (shortcut file_manager \"F1\")\n"
      "  (shortcut foo_bar \"F2\")\n"
      " )\n"
      " (project_autosave_interval 1234)\n"
      " (unknown_item \"Foo Bar\")\n"
      " (unknown_list\n"
      "  (unknown_list_item 42)\n"
      " )\n"
      ")\n",
      FilePath());

  WorkspaceSettings obj;
  obj.load(*root, Version::fromString("0.1"));
  EXPECT_EQ(1234U, obj.projectAutosaveIntervalSeconds.get());
  obj.projectAutosaveIntervalSeconds.set(42);
  const std::unique_ptr<const SExpression> root2 = obj.serialize();

  QString actualContent = root2->toByteArray();
  QString expectedContent =
      "(librepcb_workspace_settings\n"
      " (dismissed_messages\n"
      "  (message \"SOME_MESSAGE: bar\")\n"
      "  (message \"SOME_MESSAGE: foo\")\n"
      " )\n"
      " (external_pdf_reader\n"
      "  (command \"evince \\\"{{FILEPATH}}\\\"\")\n"
      " )\n"
      " (keyboard_shortcuts\n"
      "  (shortcut file_manager \"F1\")\n"
      "  (shortcut foo_bar \"F2\")\n"
      " )\n"
      " (project_autosave_interval 42)\n"
      ")\n";
  EXPECT_EQ(expectedContent.toStdString(), actualContent.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

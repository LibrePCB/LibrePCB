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

#include "../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/orderpcbdialog.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using ::librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class OrderPcbDialogTest : public ::testing::Test {
protected:
  OrderPcbDialogTest() { QSettings().clear(); }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(OrderPcbDialogTest, testAutoOpenBrowser) {
  WorkspaceSettings settings;
  settings.apiEndpoints.set(QList<QUrl>());  // Avoid API calls during test!
  const bool defaultValue = true;
  const bool newValue = false;

  {
    OrderPcbDialog dialog(settings, nullptr);

    // Check the default value.
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dialog, "cbxOpenBrowser");
    EXPECT_EQ(defaultValue, cbx.isChecked());

    // Check if the value can be changed.
    cbx.setChecked(newValue);
  }

  // Check if the setting is saved and restored automatically.
  {
    OrderPcbDialog dialog(settings, nullptr);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dialog, "cbxOpenBrowser");
    EXPECT_EQ(newValue, cbx.isChecked());
  }
}

TEST_F(OrderPcbDialogTest, testTabOrder) {
  WorkspaceSettings settings;
  settings.apiEndpoints.set(QList<QUrl>());  // Avoid API calls during test!
  OrderPcbDialog dialog(settings, nullptr);
  TestHelpers::testTabOrder(dialog);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
